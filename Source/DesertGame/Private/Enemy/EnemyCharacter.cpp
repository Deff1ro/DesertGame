// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyCharacter.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/EnemyAttackComponent.h"
#include "Inventory/ItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	AttackComponent = CreateDefaultSubobject<UEnemyAttackComponent>(TEXT("AttackComponent"));

	// Don't push the player's spring-arm when the enemy meshes pass through it.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// Apply at runtime so this overrides any Blueprint-level collision-preset overrides
	// (constructor-time settings can be silently masked when a BP edits the component's
	// collision profile in the details panel).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

bool AEnemyCharacter::TakeDamageAmount(float Amount)
{
	if (!IsAlive() || Amount <= 0.f)
	{
		return false;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);

	if (CurrentHealth <= 0.f)
	{
		HandleDeath();
	}
	return true;
}

void AEnemyCharacter::HandleDeath()
{
	OnDied.Broadcast();

	// Stop AI brain so the corpse doesn't keep ticking the BT.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Died"));
		}
	}

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

void AEnemyCharacter::SpawnLootDrops()
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

FVector AEnemyCharacter::GetNextPatrolPoint()
{
	if (PatrolPoints.Num() == 0)
	{
		return GetActorLocation();
	}

	const FVector Point = PatrolPoints[CurrentPatrolIndex]->GetActorLocation();
	CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
	return Point;
}
