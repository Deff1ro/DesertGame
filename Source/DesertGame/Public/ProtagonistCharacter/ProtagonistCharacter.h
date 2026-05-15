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
class UAIPerceptionStimuliSourceComponent;
class UAttributeComponent;
class UInventoryComponent;
class UToolUseComponent;
class AItemActor;
class UInventoryHubWidget;
class UStaticMeshComponent;
class USkeletalMesh;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttributeComponent> AttributeComponent;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	UAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UToolUseComponent> ToolUseComponent;

	UFUNCTION(BlueprintPure, Category = "Tools")
	UToolUseComponent* GetToolUseComponent() const { return ToolUseComponent; }

	// Static mesh attached to hand socket to show the equipped item.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EquippedToolMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Tools")
	FName ToolAttachSocket = TEXT("hand_r_socket");

	// Skeletal-mesh overlay parented to the body's skeleton via Leader Pose. Use it
	// for cosmetic outfit pieces (cloak/cape) that should follow the character's
	// animation without needing their own AnimBP. Assign Skeletal Mesh in BP.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> CloakMesh;

	// ============================================================
	// Interaction
	// ============================================================

	UPROPERTY()
	TObjectPtr<AItemActor> CurrentInteractable;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetCurrentInteractable(AItemActor* Item);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ClearInteractableIfMatches(AItemActor* Item);

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

	UFUNCTION(BlueprintPure, Category = "State")
	EStanceState GetStanceState() const { return CurrentStanceState; }

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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SaveGameAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	// Hotbar slot 1..4 — bind to keys 1, 2, 3, 4 in your IMC
	UPROPERTY(EditDefaultsOnly, Category = "Input|Hotbar")
	TObjectPtr<UInputAction> HotbarSlot1Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Hotbar")
	TObjectPtr<UInputAction> HotbarSlot2Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Hotbar")
	TObjectPtr<UInputAction> HotbarSlot3Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Hotbar")
	TObjectPtr<UInputAction> HotbarSlot4Action;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryHubWidget> HubWidgetClass;

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
	void OnSaveGameInput();
	void OnInteractInput();
	void OnToggleInventoryInput();
	void OnHotbarSlot1();
	void OnHotbarSlot2();
	void OnHotbarSlot3();
	void OnHotbarSlot4();
	void SelectHotbarIndex(int32 Index);

	UFUNCTION()
	void HandleStaminaDepleted();

	UFUNCTION()
	void HandleSelectedHotbarChanged(int32 NewIndex);

	UFUNCTION()
	void HandleClothChanged();

	// True when the player is currently being hurt by the environment (day without
	// heat protection, or night without cold protection). HUD filters show while
	// this is true.
	UFUNCTION(BlueprintPure, Category = "Survival")
	bool IsSufferingFromHeat() const { return bSufferingHeat; }

	UFUNCTION(BlueprintPure, Category = "Survival")
	bool IsSufferingFromCold() const { return bSufferingCold; }

	// Sound played for a single footstep. Trigger this from an AnimNotify on the
	// walk/run animations at heel-touch frames, or call from BP. Uses 2D location
	// at the character's feet.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> FootstepSound;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayFootstepSound();

protected:
	// Implemented in BP_ProtagonistCharacter (or via HUD widget) to show/hide
	// the heat-overlay PNG when the heat-suffering state changes.
	UFUNCTION(BlueprintImplementableEvent, Category = "Survival")
	void OnHeatSufferChanged(bool bSuffering);

	UFUNCTION(BlueprintImplementableEvent, Category = "Survival")
	void OnColdSufferChanged(bool bSuffering);

private:
	// Per-tick: pulls multipliers from the day/night manager and applies
	// unprotected-weather HP damage if appropriate.
	void TickEnvironmentEffects(float DeltaTime);

	bool bSufferingHeat = false;
	bool bSufferingCold = false;
	// Refreshes body mesh / cloak based on the current cloth slot contents.
	void ApplyClothFromInventory();

	void UpdateMovementState();
	void UpdateGaitState();
	void UpdateMaxSpeed();

	UPROPERTY()
	TObjectPtr<UInventoryHubWidget> HubWidgetInstance;

	bool bIsInventoryOpen = false;

	// Cached at BeginPlay so we can restore it when a body-swap outfit is removed.
	UPROPERTY()
	TObjectPtr<USkeletalMesh> DefaultBodyMesh;
};
