// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProtagonistCharacterTypes.h"
#include "ProtagonistCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UTraversalComponent;
class UMotionWarpingComponent;
class UCombatComponent;

UCLASS()
class DESERTGAME_API AProtagonistCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProtagonistCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ============================================================
	// Components
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UTraversalComponent> TraversalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCombatComponent> CombatComponent;

	// ============================================================
	// Camera / Look Control
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bLookEnabled = true;

	void EnableLookInput();
	void DisableLookInput();

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// ============================================================
	// Movement State
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	EGaitState CurrentGaitState = EGaitState::Run;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	EMovementState CurrentMovementState = EMovementState::Grounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	EStanceState CurrentStanceState = EStanceState::Standing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bWantsToSprint = false;

	// ============================================================
	// Movement Settings
	// ============================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Settings")
	float WalkSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Settings")
	float RunSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Settings")
	float SprintSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Settings")
	float CrouchSpeedMultiplier = 0.5f;

protected:
	virtual void BeginPlay() override;

	// ============================================================
	// Enhanced Input
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> BlockAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

	// ============================================================
	// Input Handlers
	// ============================================================

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void StartSprint();
	void StopSprint();
	void ToggleCrouch();
	void OnAttackInput();
	void OnBlockStart();
	void OnBlockStop();
	void OnDodgeInput(const FInputActionValue& Value);

private:
	void UpdateMovementState();
	void UpdateGaitState();
	void UpdateMaxSpeed();
};
