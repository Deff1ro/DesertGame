// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyAttackComponent.h"
#include "Enemy/EnemyCharacter.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Attributes/AttributeComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY(LogDesertEnemyAttack);

UEnemyAttackComponent::UEnemyAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyAttackComponent::PerformAttackHit()
{
	AEnemyCharacter* Enemy = GetOwnerEnemy();
	if (!Enemy)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Build the capsule centre: in front of the enemy, optionally raised vertically.
	const FVector Forward = Enemy->GetActorForwardVector();
	const FVector Up = Enemy->GetActorUpVector();
	const FVector Centre = Enemy->GetActorLocation()
		+ Forward * AttackForwardReach
		+ Up * AttackVerticalOffset;

	// Capsule oriented along the enemy's up axis (standard upright capsule).
	const FQuat CapsuleRot = Enemy->GetActorRotation().Quaternion();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Enemy);

	if (AttackSwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSwingSound, Enemy->GetActorLocation());
	}

	TArray<FHitResult> Hits;
	const bool bAnyHit = World->SweepMultiByChannel(
		Hits,
		Centre,
		Centre,
		CapsuleRot,
		ECC_Pawn,
		FCollisionShape::MakeCapsule(AttackCapsuleRadius, AttackCapsuleHalfHeight),
		Params);

	if (bDebugDraw)
	{
		UKismetSystemLibrary::DrawDebugCapsule(
			World,
			Centre,
			AttackCapsuleHalfHeight,
			AttackCapsuleRadius,
			Enemy->GetActorRotation(),
			bAnyHit ? FLinearColor::Red : FLinearColor::Green,
			1.5f);
	}

	if (!bAnyHit)
	{
		UE_LOG(LogDesertEnemyAttack, Log, TEXT("EnemyAttack '%s' — no target in capsule"), *Enemy->GetName());
		return;
	}

	// Apply damage to the player only — and only once per swing, even if the capsule
	// catches multiple components on the same character.
	for (const FHitResult& Hit : Hits)
	{
		AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Hit.GetActor());
		if (!Player)
		{
			continue;
		}

		if (UAttributeComponent* Attr = Player->GetAttributeComponent())
		{
			Attr->ApplyDamage(AttackDamage);
			UE_LOG(LogDesertEnemyAttack, Log, TEXT("EnemyAttack '%s' — hit player for %.1f"),
				*Enemy->GetName(), AttackDamage);

			if (AttackHitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, AttackHitSound, Player->GetActorLocation());
			}
		}
		break;
	}
}

AEnemyCharacter* UEnemyAttackComponent::GetOwnerEnemy() const
{
	return Cast<AEnemyCharacter>(GetOwner());
}
