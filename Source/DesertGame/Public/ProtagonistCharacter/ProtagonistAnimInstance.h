// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ProtagonistCharacterTypes.h"
#include "Traversal/TraversalTypes.h"
#include "ProtagonistAnimInstance.generated.h"

class AProtagonistCharacter;
class UCharacterMovementComponent;
class UTraversalComponent;

UCLASS()
class DESERTGAME_API UProtagonistAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ============================================================
	// Locomotion
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsJumping = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	EGaitState GaitState = EGaitState::Run;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	EStanceState StanceState = EStanceState::Standing;

	// ============================================================
	// Traversal
	// ============================================================

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bIsPerformingTraversal = false;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	ETraversalAction CurrentTraversalAction = ETraversalAction::None;

private:
	UPROPERTY()
	TObjectPtr<AProtagonistCharacter> OwningCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY()
	TObjectPtr<UTraversalComponent> TraversalComponent;
};
