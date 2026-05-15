// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inventory/InventoryTypes.h"
#include "EnemyCharacter.generated.h"

class UAnimMontage;
class UBehaviorTree;
class UEnemyAttackComponent;
class AItemActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDied);

UCLASS()
class DESERTGAME_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void BeginPlay() override;

	// ============================================================
	// Health
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Health")
	float CurrentHealth = 100.f;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Health")
	FOnEnemyDied OnDied;

	UFUNCTION(BlueprintPure, Category = "Enemy|Health")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	// Apply damage to this enemy. Returns true if the hit actually landed (i.e.
	// the enemy was alive at the time of the call).
	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	bool TakeDamageAmount(float Amount);

	// Seconds to keep the corpse around after dying — allow time for a death
	// montage. 0 = destroy on the same frame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float DeathLingerTime = 0.f;

	// ============================================================
	// Loot drops
	// ============================================================

	// Items that scatter out of this enemy when it dies.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Loot")
	TArray<FEnemyLootEntry> LootTable;

	// How far (cm) items are scattered sideways from the death location.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Loot", meta = (ClampMin = "0.0"))
	float DropScatterRadius = 100.f;

	// Height above the actor origin at which drops are spawned.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Loot", meta = (ClampMin = "0.0"))
	float DropSpawnHeight = 50.f;

	// Class used to spawn loot actors. Assign AItemActor subclass in BP if needed.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Loot")
	TSubclassOf<AItemActor> DropActorClass;

	// ============================================================
	// Patrol
	// ============================================================

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "AI|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float PatrolWaitTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float PatrolSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float ChaseSpeed = 450.f;

	FVector GetNextPatrolPoint();

	// ============================================================
	// Combat
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat")
	TObjectPtr<UEnemyAttackComponent> AttackComponent;

	// ============================================================
	// Behavior Tree (assign asset in BP)
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

protected:
	// Called once when health drops to zero. Default implementation disables AI,
	// disables collision and destroys the actor after a short delay so death
	// montages (if any) can play.
	virtual void HandleDeath();

private:
	int32 CurrentPatrolIndex = 0;

	void SpawnLootDrops();
};
