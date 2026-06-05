// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyAIController.h"
#include "Enemy/EnemyCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

AEnemyAIController::AEnemyAIController()
{
	BehaviorTreeComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));

	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1800.f;
	SightConfig->LoseSightRadius = 2200.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	// MaxAge=0 — stimulus expires the moment we lose sight. Combined with the
	// give-up timer in OnTargetPerceptionUpdated this produces a clean "lost him"
	// transition back to patrol.
	SightConfig->SetMaxAge(0.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = -1.f;
	// Trace channel used for the line-of-sight check. Walls/landscape must
	// BLOCK this channel for occlusion to work.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
			this, &AEnemyAIController::OnTargetPerceptionUpdated);
	}
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickProximitySense();

#if ENABLE_DRAW_DEBUG
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !SightConfig) return;

	const FVector EyeLocation = ControlledPawn->GetActorLocation() + FVector(0, 0, 50.f);
	const FVector ForwardDir = ControlledPawn->GetActorForwardVector();
	const float HalfAngleRad = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);
	const float Radius = SightConfig->SightRadius;

	// Check if we currently see the target
	const bool bHasTarget = BlackboardComp && BlackboardComp->GetValueAsObject(FName("TargetActor")) != nullptr;
	const FColor ConeColor = bHasTarget ? FColor::Red : FColor::Green;

	// Draw sight cone lines
	const int32 NumSegments = 16;
	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float AngleFraction = (float)i / (float)NumSegments;
		const float CurrentAngle = -HalfAngleRad + (2.f * HalfAngleRad * AngleFraction);
		const FVector Dir = ForwardDir.RotateAngleAxis(FMath::RadiansToDegrees(CurrentAngle), FVector::UpVector);
		DrawDebugLine(GetWorld(), EyeLocation, EyeLocation + Dir * Radius, ConeColor, false, -1.f, 0, 1.f);
	}

	// Draw arc at the end
	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float Angle1 = -HalfAngleRad + (2.f * HalfAngleRad * ((float)i / (float)NumSegments));
		const float Angle2 = -HalfAngleRad + (2.f * HalfAngleRad * ((float)(i + 1) / (float)NumSegments));
		const FVector P1 = EyeLocation + ForwardDir.RotateAngleAxis(FMath::RadiansToDegrees(Angle1), FVector::UpVector) * Radius;
		const FVector P2 = EyeLocation + ForwardDir.RotateAngleAxis(FMath::RadiansToDegrees(Angle2), FVector::UpVector) * Radius;
		DrawDebugLine(GetWorld(), P1, P2, ConeColor, false, -1.f, 0, 1.f);
	}

	// Draw proximity-sense circle (always-detect zone, regardless of facing)
	if (ProximityRadius > 0.f)
	{
		DrawDebugCircle(GetWorld(), EyeLocation, ProximityRadius, 32,
			FColor::Yellow, false, -1.f, 0, 1.f, FVector(1, 0, 0), FVector(0, 1, 0));
	}
#endif
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn);
	if (!EnemyChar || !EnemyChar->BehaviorTree) return;

	if (EnemyChar->BehaviorTree->BlackboardAsset)
	{
		BlackboardComp->InitializeBlackboard(*EnemyChar->BehaviorTree->BlackboardAsset);
	}

	RunBehaviorTree(EnemyChar->BehaviorTree);
}

void AEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	const APawn* PlayerPawn = Cast<APawn>(Actor);
	if (!PlayerPawn || !PlayerPawn->IsPlayerControlled()) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bWasAggroed = BlackboardComp->GetValueAsObject(FName("TargetActor")) != nullptr;

		// Увидели игрока — запоминаем цель, сбрасываем последнюю позицию.
		BlackboardComp->SetValueAsObject(FName("TargetActor"), Actor);
		BlackboardComp->ClearValue(FName("LastKnownLocation"));

		// Cancel any pending give-up timer — we have a live target again.
		GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);

		// First-time aggro this chase: bark + combat music.
		if (!bWasAggroed)
		{
			if (AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(GetPawn()))
			{
				if (EnemyChar->SpotPlayerSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, EnemyChar->SpotPlayerSound, EnemyChar->GetActorLocation());
				}
			}
			if (CombatMusic && !CombatMusicComponent)
			{
				CombatMusicComponent = UGameplayStatics::SpawnSound2D(this, CombatMusic);
			}
		}
	}
	else
	{
		// Потеряли из виду — запоминаем последнюю позицию для расследования,
		// сбрасываем активную цель.
		BlackboardComp->SetValueAsVector(FName("LastKnownLocation"), Stimulus.StimulusLocation);
		BlackboardComp->ClearValue(FName("TargetActor"));

		// Start the give-up countdown. If perception doesn't reacquire the
		// player within InvestigationTimeout seconds, drop LastKnownLocation
		// so the BT falls back to patrol.
		GetWorldTimerManager().SetTimer(
			InvestigationTimerHandle,
			this,
			&AEnemyAIController::OnInvestigationTimeout,
			FMath::Max(0.5f, InvestigationTimeout),
			false);
	}
}

void AEnemyAIController::OnInvestigationTimeout()
{
	if (!BlackboardComp) return;

	// Only clear if we still have no active target; if we re-spotted the player
	// in the meantime, OnTargetPerceptionUpdated already cleared the timer.
	if (BlackboardComp->GetValueAsObject(FName("TargetActor")) == nullptr)
	{
		BlackboardComp->ClearValue(FName("LastKnownLocation"));

		// Stop combat music — the chase is fully over.
		if (CombatMusicComponent)
		{
			CombatMusicComponent->FadeOut(1.0f, 0.f);
			CombatMusicComponent = nullptr;
		}
	}
}

void AEnemyAIController::TickProximitySense()
{
	if (ProximityRadius <= 0.f || !BlackboardComp)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	// Cheap distance check to the local player. If they're close enough,
	// treat them as spotted regardless of facing.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistSq > ProximityRadius * ProximityRadius)
	{
		return;
	}

	// Player is in our personal-space bubble — make sure they're the target.
	if (BlackboardComp->GetValueAsObject(FName("TargetActor")) != PlayerPawn)
	{
		BlackboardComp->SetValueAsObject(FName("TargetActor"), PlayerPawn);
		BlackboardComp->ClearValue(FName("LastKnownLocation"));
		GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);
	}
}
