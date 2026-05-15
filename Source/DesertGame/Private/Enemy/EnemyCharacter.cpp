// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyCharacter.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/EnemyAttackComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "TimerManager.h"

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

	if (DeathLingerTime > 0.f)
	{
		// Give a moment for a death montage / fade. Adjust per BP if needed.
		SetLifeSpan(DeathLingerTime);
	}
	else
	{
		// No linger time configured — vanish immediately.
		Destroy();
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
