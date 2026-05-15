// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;
class UAnimInstance;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Attacking	UMETA(DisplayName = "Attacking"),
	Blocking	UMETA(DisplayName = "Blocking"),
	Dodging		UMETA(DisplayName = "Dodging")
};

UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Forward		UMETA(DisplayName = "Forward"),
	Backward	UMETA(DisplayName = "Backward"),
	Left		UMETA(DisplayName = "Left"),
	Right		UMETA(DisplayName = "Right")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// Public API
	// ============================================================

	/** Called from character input — requests an attack or queues next combo hit */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RequestAttack();

	/** Called by AnimNotifyState_ComboWindow */
	void OpenComboWindow();
	void CloseComboWindow();

	/** Block — called from character input on RMB press/release */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopBlock();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsBlocking() const;

	/** Dodge — called from character input (Ctrl + WASD).
	 *  WorldSlideDirection: world-space direction the slide should move in. Pass
	 *  ZeroVector to fall back to deriving it from the owner's actor rotation
	 *  (legacy behaviour). Pass an explicit vector when the caller wants the
	 *  slide to follow a different basis (e.g. camera-relative input). */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RequestDodge(EDodgeDirection Direction, FVector WorldSlideDirection = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	bool IsDodging() const;

	// Called by UAnimNotify_SwordHit at the impact frame of every combo section.
	// Sweeps a capsule in front of the owner and damages enemies caught inside.
	// Damage is read from the currently equipped sword's ItemDataAsset.ToolDamage.
	UFUNCTION(BlueprintCallable, Category = "Combat|Sword")
	void PerformSwordHit();

	// ============================================================
	// Settings
	// ============================================================

	/** The montage containing all combo sections (Attack1, Attack2, ...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	TObjectPtr<UAnimMontage> ComboMontage;

	/** Section names inside the montage, in order */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	TArray<FName> ComboSectionNames;

	/** Animation Montage for block stance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	TObjectPtr<UAnimMontage> BlockMontage;

	/** Dodge montages — one per direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	TMap<EDodgeDirection, TObjectPtr<UAnimMontage>> DodgeMontages;

	/** Total extra distance (cm) to travel during dodge, on top of root motion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	float DodgeExtraDistance = 200.f;

	/** How long (seconds) the extra movement lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Settings")
	float DodgeSlideDuration = 0.3f;

	// ============================================================
	// Sword Hitbox (capsule swept in front of the player on each combo strike)
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword", meta = (ClampMin = "1.0"))
	float SwordCapsuleRadius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword", meta = (ClampMin = "1.0"))
	float SwordCapsuleHalfHeight = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword", meta = (ClampMin = "0.0"))
	float SwordForwardReach = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword")
	float SwordVerticalOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword|Debug")
	bool bDebugDrawSwordHit = false;

	// Sound played whenever the sword swings (one per combo strike notify).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword|Audio")
	TObjectPtr<class USoundBase> SwordSwingSound;

	// Sound played when the swing actually hits an enemy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sword|Audio")
	TObjectPtr<class USoundBase> SwordHitSound;

	// ============================================================
	// Runtime State (read by AnimInstance if needed)
	// ============================================================

	UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|State")
	int32 ComboIndex = 0;

private:
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void ResetCombo();

	void InterruptCurrentState();

	bool bComboWindowOpen = false;
	bool bPendingNextCombo = false;

	UPROPERTY()
	TObjectPtr<UAnimInstance> CachedAnimInstance;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	// Dodge slide state
	FVector DodgeSlideDirection = FVector::ZeroVector;
	float DodgeSlideTimeRemaining = 0.f;
	float DodgeSlideSpeed = 0.f;
};
