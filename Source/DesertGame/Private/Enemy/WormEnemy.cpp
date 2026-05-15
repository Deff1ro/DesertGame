// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/WormEnemy.h"
#include "Enemy/EnemyCharacter.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Attributes/AttributeComponent.h"
#include "Inventory/ItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY(LogDesertWorm);

AWormEnemy::AWormEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	// Block dynamic actors (player, enemies) but ignore world geometry so the mesh
	// can rise through the terrain. Camera channel ignored so the spring-arm
	// doesn't snap when the worm erupts close to the player.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	DetectionTrigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DetectionTrigger"));
	DetectionTrigger->SetupAttachment(MeshComponent);
	DetectionTrigger->SetCapsuleRadius(200.f);
	DetectionTrigger->SetCapsuleHalfHeight(150.f);
	DetectionTrigger->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	DetectionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	KillCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("KillCapsule"));
	KillCapsule->SetupAttachment(MeshComponent);
	KillCapsule->SetCapsuleRadius(150.f);
	KillCapsule->SetCapsuleHalfHeight(200.f);
	KillCapsule->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	KillCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KillCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	KillCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AWormEnemy::BeginPlay()
{
	Super::BeginPlay();

	BuriedLocation = GetActorLocation();
	State = EWormState::Hidden;
	CurrentHealth = MaxHealth;

	if (DetectionTrigger)
	{
		DetectionTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWormEnemy::OnDetectionBeginOverlap);
	}
}

bool AWormEnemy::TakeDamageAmount(float Amount)
{
	if (!IsAlive() || Amount <= 0.f)
	{
		return false;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — took %.1f damage, HP %.1f/%.1f"),
		*GetName(), Amount, CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.f)
	{
		HandleDeath();
	}
	return true;
}

void AWormEnemy::HandleDeath()
{
	OnDied.Broadcast();

	// Stop any pending state transitions so the corpse doesn't keep ticking.
	GetWorldTimerManager().ClearTimer(TelegraphTimer);
	GetWorldTimerManager().ClearTimer(AttackTimer);
	SetActorTickEnabled(false);

	// Cancel any active animation loop and camera shake.
	StopRiseMontageLoop();
	StopCameraShake();

	// Disable all triggers/collision so the body can't ambush, kill, or be hit again.
	if (DetectionTrigger)
	{
		DetectionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (KillCapsule)
	{
		KillCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SpawnLootDrops();

	if (DeathLingerTime > 0.f)
	{
		SetLifeSpan(DeathLingerTime);
	}
	else
	{
		Destroy();
	}
}

void AWormEnemy::SpawnLootDrops()
{
	if (LootTable.Num() == 0 || !DropActorClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, DropSpawnHeight);

	for (const FEnemyLootEntry& Entry : LootTable)
	{
		if (!Entry.Item)
		{
			continue;
		}

		const int32 Count = FMath::RandRange(Entry.DropMin, FMath::Max(Entry.DropMin, Entry.DropMax));

		for (int32 i = 0; i < Count; ++i)
		{
			const float Angle = FMath::FRandRange(0.f, 2.f * PI);
			const FVector LateralDir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);

			const float SpawnOffsetDistance = FMath::FRandRange(0.f, FMath::Min(DropScatterRadius * 0.25f, 25.f));
			const FRotator SpawnRotation = FMath::VRand().Rotation();
			const FVector SpawnLocation = Origin + LateralDir * SpawnOffsetDistance;

			AItemActor* Item = World->SpawnActor<AItemActor>(DropActorClass, SpawnLocation, SpawnRotation, Params);
			if (!Item)
			{
				continue;
			}

			Item->Initialize(Entry.Item, 1);

			const float LateralStrength = FMath::FRandRange(150.f, 300.f);
			const float UpwardStrength = FMath::FRandRange(250.f, 400.f);
			const FVector Impulse = LateralDir * LateralStrength + FVector(0.f, 0.f, UpwardStrength);

			Item->LaunchAsDrop(Impulse);
		}
	}
}

void AWormEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State != EWormState::Rising && State != EWormState::Burrowing)
	{
		return;
	}

	MotionElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(MotionElapsed / FMath::Max(0.001f, MotionDuration), 0.f, 1.f);
	// Smoothstep for a softer ease in/out
	const float Smoothed = Alpha * Alpha * (3.f - 2.f * Alpha);
	const FVector NewLoc = FMath::Lerp(MotionStart, MotionEnd, Smoothed);
	SetActorLocation(NewLoc);

	if (Alpha >= 1.f)
	{
		if (State == EWormState::Rising)
		{
			OnRiseComplete();
		}
		else
		{
			OnBurrowComplete();
		}
	}
}

void AWormEnemy::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (State != EWormState::Hidden)
	{
		return;
	}
	if (!IsValidPrey(OtherActor))
	{
		return;
	}

	UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — prey '%s' entered, telegraphing"),
		*GetName(), *OtherActor->GetName());

	StartTelegraph();
}

bool AWormEnemy::IsValidPrey(AActor* Other) const
{
	if (!Other || Other == this)
	{
		return false;
	}
	if (Cast<AProtagonistCharacter>(Other))
	{
		return true;
	}
	if (AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(Other))
	{
		return EnemyChar->IsAlive();
	}
	return false;
}

void AWormEnemy::StartTelegraph()
{
	State = EWormState::Telegraphing;

	StartCameraShake();

	GetWorldTimerManager().SetTimer(
		TelegraphTimer,
		this,
		&AWormEnemy::StartRise,
		FMath::Max(0.01f, TelegraphDelay),
		false);
}

void AWormEnemy::StartRise()
{
	State = EWormState::Rising;

	MotionStart = BuriedLocation;
	MotionEnd = BuriedLocation + FVector(0.f, 0.f, RiseDistance);
	MotionDuration = RiseDuration;
	MotionElapsed = 0.f;
	SetActorTickEnabled(true);

	if (EmergeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EmergeSound, GetActorLocation());
	}

	// Start the rise/attack animation loop. It keeps playing through Rising,
	// Attacking and Burrowing — i.e. the entire active lifetime.
	StartRiseMontageLoop();
}

void AWormEnemy::OnRiseComplete()
{
	State = EWormState::Attacking;
	SetActorTickEnabled(false);

	// Camera shake stops once the worm is fully out of the ground
	StopCameraShake();

	if (KillCapsule)
	{
		KillCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// Geometry-based overlap query: doesn't depend on the kill capsule having
	// accumulated overlaps yet (it was just enabled this frame). We sweep the
	// world directly with the kill capsule's current shape and transform.
	UWorld* World = GetWorld();
	if (World && KillCapsule)
	{
		const FVector CapsuleLoc = KillCapsule->GetComponentLocation();
		const FQuat CapsuleRot = KillCapsule->GetComponentQuat();
		const float Radius = KillCapsule->GetScaledCapsuleRadius();
		const float HalfHeight = KillCapsule->GetScaledCapsuleHalfHeight();

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(
			Overlaps,
			CapsuleLoc,
			CapsuleRot,
			ECC_Pawn,
			FCollisionShape::MakeCapsule(Radius, HalfHeight),
			Params);

		if (bDebugDrawKillCapsule)
		{
			UKismetSystemLibrary::DrawDebugCapsule(
				World,
				CapsuleLoc,
				HalfHeight,
				Radius,
				CapsuleRot.Rotator(),
				Overlaps.Num() > 0 ? FLinearColor::Red : FLinearColor::Yellow,
				2.0f);
		}

		TSet<AActor*> Hit;
		for (const FOverlapResult& O : Overlaps)
		{
			AActor* Victim = O.GetActor();
			if (!IsValidPrey(Victim) || Hit.Contains(Victim))
			{
				continue;
			}
			Hit.Add(Victim);

			if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Victim))
			{
				if (UAttributeComponent* Attr = Player->GetAttributeComponent())
				{
					Attr->ApplyDamage(StrikeDamage);
					UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — killed player"), *GetName());
				}
			}
			else if (AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(Victim))
			{
				EnemyChar->TakeDamageAmount(StrikeDamage);
				UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — devoured enemy '%s'"),
					*GetName(), *EnemyChar->GetName());
			}

			if (StrikeHitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, StrikeHitSound, Victim->GetActorLocation());
			}
		}
	}

	// Schedule burrow at the end of AttackDuration
	GetWorldTimerManager().SetTimer(
		AttackTimer,
		this,
		&AWormEnemy::StartBurrow,
		FMath::Max(0.1f, AttackDuration),
		false);

	UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — strike active"), *GetName());
}

void AWormEnemy::StartBurrow()
{
	State = EWormState::Burrowing;

	if (KillCapsule)
	{
		KillCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	MotionStart = GetActorLocation();
	MotionEnd = BuriedLocation;
	MotionDuration = BurrowDuration;
	MotionElapsed = 0.f;
	SetActorTickEnabled(true);
}

void AWormEnemy::OnBurrowComplete()
{
	SetActorTickEnabled(false);
	State = EWormState::Hidden;

	// Worm is fully buried — kill the looping animation.
	StopRiseMontageLoop();

	UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s' — re-armed"), *GetName());
}

// ============================================================
// Rise montage loop
// ============================================================

void AWormEnemy::StartRiseMontageLoop()
{
	if (!RiseMontage)
	{
		UE_LOG(LogDesertWorm, Warning, TEXT("Worm '%s': RiseMontage not assigned"), *GetName());
		return;
	}
	if (!MeshComponent)
	{
		UE_LOG(LogDesertWorm, Warning, TEXT("Worm '%s': MeshComponent missing"), *GetName());
		return;
	}
	UAnimInstance* AnimInst = MeshComponent->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogDesertWorm, Warning, TEXT("Worm '%s': mesh has no AnimInstance — set Animation Mode to 'Use Animation Blueprint' and assign an Anim Class"), *GetName());
		return;
	}

	const float PlayLength = AnimInst->Montage_Play(RiseMontage);
	if (PlayLength <= 0.f)
	{
		UE_LOG(LogDesertWorm, Warning, TEXT("Worm '%s': Montage_Play returned 0 — montage skeleton mismatch or slot missing in AnimGraph?"), *GetName());
	}
	else
	{
		UE_LOG(LogDesertWorm, Log, TEXT("Worm '%s': playing RiseMontage (length %.2fs)"), *GetName(), PlayLength);
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AWormEnemy::OnRiseMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, RiseMontage);
}

void AWormEnemy::StopRiseMontageLoop()
{
	if (!RiseMontage || !MeshComponent)
	{
		return;
	}
	UAnimInstance* AnimInst = MeshComponent->GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	// Clear the end delegate first so the stop doesn't trigger another loop.
	FOnMontageEnded EmptyDelegate;
	AnimInst->Montage_SetEndDelegate(EmptyDelegate, RiseMontage);

	if (AnimInst->Montage_IsPlaying(RiseMontage))
	{
		AnimInst->Montage_Stop(0.2f, RiseMontage);
	}
}

void AWormEnemy::OnRiseMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Don't re-play if we were stopped on purpose, or if we've already gone
	// back to Hidden between when the delegate was bound and when it fired.
	if (bInterrupted || State == EWormState::Hidden || Montage != RiseMontage)
	{
		return;
	}
	if (!MeshComponent)
	{
		return;
	}
	UAnimInstance* AnimInst = MeshComponent->GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	AnimInst->Montage_Play(RiseMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AWormEnemy::OnRiseMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, RiseMontage);
}

// ============================================================
// Camera shake
// ============================================================

void AWormEnemy::StartCameraShake()
{
	if (!CameraShakeClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Apply to all local players (single-player practice: there's only one).
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(CameraShakeClass, CameraShakeScale);
		}
	}
}

void AWormEnemy::StopCameraShake()
{
	if (!CameraShakeClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopAllInstancesOfCameraShake(CameraShakeClass);
		}
	}
}
