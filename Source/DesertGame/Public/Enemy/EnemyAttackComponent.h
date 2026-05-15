// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAttackComponent.generated.h"

class AEnemyCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertEnemyAttack, Log, All);

// Performs a capsule-shaped melee hit-check in front of the owning enemy. Driven by
// UAnimNotify_EnemyAttackHit placed on the active frame of an attack montage.
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UEnemyAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAttackComponent();

	// Called by UAnimNotify_EnemyAttackHit on the impact frame of the attack montage.
	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	void PerformAttackHit();

	// Damage applied to the player on a successful hit.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	// Capsule radius (cm) for the attack sweep.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Hitbox", meta = (ClampMin = "1.0"))
	float AttackCapsuleRadius = 60.0f;

	// Capsule half-height (cm) for the attack sweep.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Hitbox", meta = (ClampMin = "1.0"))
	float AttackCapsuleHalfHeight = 90.0f;

	// Distance (cm) the capsule centre is offset forward from the enemy's location.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Hitbox", meta = (ClampMin = "0.0"))
	float AttackForwardReach = 120.0f;

	// Vertical offset (cm) for the capsule centre, e.g. raise it to chest height.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Hitbox")
	float AttackVerticalOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Debug")
	bool bDebugDraw = false;

	// Played each swing — set per enemy BP (different growl/swipe per species).
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Audio")
	TObjectPtr<class USoundBase> AttackSwingSound;

	// Played when the swing connects with the player.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Attack|Audio")
	TObjectPtr<class USoundBase> AttackHitSound;

private:
	AEnemyCharacter* GetOwnerEnemy() const;
};
