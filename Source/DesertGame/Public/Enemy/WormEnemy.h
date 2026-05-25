// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryTypes.h"
#include "WormEnemy.generated.h"

class USkeletalMeshComponent;
class UCapsuleComponent;
class UBoxComponent;
class UAnimMontage;
class UPrimitiveComponent;
class UCameraShakeBase;
class AItemActor;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertWorm, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWormDied);

UENUM(BlueprintType)
enum class EWormState : uint8
{
	Hidden       UMETA(DisplayName = "Hidden"),     // Buried, waiting for prey to enter trigger
	Telegraphing UMETA(DisplayName = "Telegraphing"),// Prey detected, waiting before strike
	Rising       UMETA(DisplayName = "Rising"),     // Animating up out of the ground
	Attacking    UMETA(DisplayName = "Attacking"),  // Fully risen, kill capsule active
	Burrowing    UMETA(DisplayName = "Burrowing")   // Animating back down
};

// An ambush enemy that hides under the surface and erupts upward when something
// living enters its trigger zone. Anything caught in its kill capsule during
// the strike is killed instantly.
UCLASS()
class DESERTGAME_API AWormEnemy : public AActor
{
	GENERATED_BODY()

public:
	AWormEnemy();

	// ============================================================
	// Tunable parameters (BP)
	// ============================================================

	// Delay between detecting prey and rising to attack. Worm tracks the prey's
	// XY position throughout this window — see StrikeLeadTime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Timing", meta = (ClampMin = "0.0"))
	float TelegraphDelay = 2.f;

	// How "stale" the captured prey position should be when the worm erupts.
	// In other words: the worm strikes where the prey was StrikeLeadTime seconds
	// ago, giving the player exactly that much time to dodge by running.
	// Must be <= TelegraphDelay (otherwise we'd need a position from before
	// detection started — falls back to the earliest sample available).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Timing", meta = (ClampMin = "0.0"))
	float StrikeLeadTime = 1.f;

	// How long the worm stays risen and dangerous.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Timing", meta = (ClampMin = "0.1"))
	float AttackDuration = 5.f;

	// How far (cm) the mesh moves up when attacking, then back down when re-burrowing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Motion", meta = (ClampMin = "0.0"))
	float RiseDistance = 200.f;

	// Seconds it takes to smoothly rise from buried to risen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Motion", meta = (ClampMin = "0.05"))
	float RiseDuration = 0.6f;

	// Seconds it takes to smoothly burrow back down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Motion", meta = (ClampMin = "0.05"))
	float BurrowDuration = 0.6f;

	// Damage applied to anything inside the kill capsule on the strike. A very
	// large default ensures even high-HP targets die instantly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Combat", meta = (ClampMin = "1.0"))
	float StrikeDamage = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Played as the worm rises out of the ground (looping or one-shot, ends naturally).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Combat")
	TObjectPtr<UAnimMontage> RiseMontage;

	// Camera shake played on the local player from telegraph start until the worm
	// is fully risen, then stopped. Optional — leave null to skip.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|FX")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|FX", meta = (ClampMin = "0.0"))
	float CameraShakeScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Debug")
	bool bDebugDrawKillCapsule = false;

	// Played the moment the worm starts rising (warning roar).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Audio")
	TObjectPtr<class USoundBase> EmergeSound;

	// Played when the worm's strike actually catches a victim.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Audio")
	TObjectPtr<class USoundBase> StrikeHitSound;

	// ============================================================
	// Health
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Worm|Health")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Worm|Health", meta = (ClampMin = "0.0"))
	float DeathLingerTime = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "Worm|Health")
	FOnWormDied OnDied;

	// ============================================================
	// Loot drops
	// ============================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Worm|Loot")
	TArray<FEnemyLootEntry> LootTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Worm|Loot", meta = (ClampMin = "0.0"))
	float DropScatterRadius = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Worm|Loot", meta = (ClampMin = "0.0"))
	float DropSpawnHeight = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Worm|Loot")
	TSubclassOf<AItemActor> DropActorClass;

	UFUNCTION(BlueprintPure, Category = "Worm|Health")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	// Same signature as AEnemyCharacter::TakeDamageAmount so the player's tool/sword
	// hit-handlers can damage worms with the same code path.
	UFUNCTION(BlueprintCallable, Category = "Worm|Health")
	bool TakeDamageAmount(float Amount);

	// ============================================================
	// Components
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// Detection zone — a flat-ish box covering the area the worm patrols. The
	// worm can erupt anywhere inside it (at BuriedLocation.Z). Size it in BP
	// to match the patrol area.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> DetectionBox;

	// Kill capsule activated only during the Attacking state. Covers the worm's
	// risen body. Adjustable in BP to tune to the mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> KillCapsule;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void HandleDeath();

	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void StartTelegraph();
	void StartRise();
	void OnRiseComplete();
	void StartBurrow();
	void OnBurrowComplete();

	// Sample current prey XY into the tracking buffer and trim old entries.
	void SampleTrackedPosition(float WorldTimeSeconds);

	// Returns the prey XY captured StrikeLeadTime seconds ago (or the oldest
	// available sample if we don't have one that old). bOutHasSample = false
	// if the buffer is empty.
	FVector ResolveStrikePoint(float WorldTimeSeconds, bool& bOutHasSample) const;

	// Picks any valid prey currently overlapping the detection box.
	AActor* FindPreyInDetectionBox() const;

	void StartCameraShake();
	void StopCameraShake();

	// Plays RiseMontage and arms an end-delegate that re-plays it as long as we're
	// still in an active state. Treated as a loop until StopRiseMontage() is called.
	void StartRiseMontageLoop();
	void StopRiseMontageLoop();

	UFUNCTION()
	void OnRiseMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Returns true for any pawn we should ambush (player + AEnemyCharacter, but not
	// other worms). Does NOT consider whether the prey is alive — caller should.
	bool IsValidPrey(AActor* Other) const;

	UPROPERTY(VisibleInstanceOnly, Category = "Worm|State")
	EWormState State = EWormState::Hidden;

	// Initial buried position recorded at BeginPlay so we can rise/fall by RiseDistance.
	FVector BuriedLocation = FVector::ZeroVector;

	FTimerHandle TelegraphTimer;
	FTimerHandle AttackTimer;

	// Per-frame motion interpolation
	float MotionElapsed = 0.f;
	FVector MotionStart = FVector::ZeroVector;
	FVector MotionEnd = FVector::ZeroVector;
	float MotionDuration = 0.f;

	// One captured prey snapshot in the tracking buffer.
	struct FTrackedSample
	{
		FVector Position = FVector::ZeroVector;
		float TimeSeconds = 0.f;
	};

	// Ring-ish buffer of recent prey XY positions. Trimmed every tick so the
	// oldest sample is always slightly older than StrikeLeadTime.
	TArray<FTrackedSample> TrackedSamples;

	// The actor we're currently tracking. Refreshed each tick from the box's
	// overlap list. Cleared when no valid prey remains in the zone.
	UPROPERTY()
	TWeakObjectPtr<AActor> TrackedPrey;

	// World location chosen as the strike point, captured at the moment we
	// commit to the rise. Used to drive the rise interpolation.
	FVector StrikeLocation = FVector::ZeroVector;

	void SpawnLootDrops();
};
