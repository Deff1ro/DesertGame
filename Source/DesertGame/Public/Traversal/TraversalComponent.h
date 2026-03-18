// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TraversalTypes.h"
#include "TraversalComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UMotionWarpingComponent;
class UAnimInstance;
class UAnimMontage;
class USpringArmComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTraversalComponent();

	virtual void BeginPlay() override;

	// ============================================================
	// Public API
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FTraversalCheckResult PerformTraversalCheck();

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void ExecuteTraversal(const FTraversalCheckResult& Result);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Traversal")
	bool CanPerformTraversal() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Traversal")
	FTraversalCheckResult GetLastCheckResult() const { return LastCheckResult; }

	// ============================================================
	// Settings
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Settings")
	FTraversalTraceSettings TraceSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Animations")
	TMap<ETraversalAction, TObjectPtr<UAnimMontage>> TraversalMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Traversal|Settings")
	FName WarpTargetName = TEXT("TraversalTarget");

	// ============================================================
	// Debug
	// ============================================================

	UPROPERTY(EditAnywhere, Category = "Traversal|Debug")
	bool bDebugDraw = false;

	UPROPERTY(EditAnywhere, Category = "Traversal|Debug")
	float DebugDrawDuration = 2.f;

	// ============================================================
	// Runtime State
	// ============================================================

	UPROPERTY(BlueprintReadOnly, Category = "Traversal|State")
	FTraversalCheckResult LastCheckResult;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal|State")
	bool bIsPerformingTraversal = false;

private:
	// ============================================================
	// Trace Methods
	// ============================================================

	bool ForwardTrace(FHitResult& OutHit);
	bool HeightTrace(const FHitResult& WallHit, FVector& OutLedgeLocation);
	bool DepthTrace(const FVector& LedgeLocation, const FVector& WallNormal, float& OutDepth);
	bool RoomCheck(const FVector& LedgeLocation) const;
	ETraversalAction ClassifyObstacle(float Height, float Depth, bool bHasRoom) const;

	UFUNCTION()
	void OnTraversalMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void EnsureCachedPointers();

	// ============================================================
	// Cached Pointers
	// ============================================================

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY()
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY()
	TObjectPtr<UAnimInstance> AnimInstance;

	TWeakObjectPtr<AActor> CurrentTraversalObstacle;

	UPROPERTY()
	TObjectPtr<USpringArmComponent> CameraBoom;
};
