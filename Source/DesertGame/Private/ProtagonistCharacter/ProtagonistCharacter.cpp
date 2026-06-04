// Fill out your copyright notice in the Description page of Project Settings.

#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Traversal/TraversalComponent.h"
#include "MotionWarpingComponent.h"
#include "Combat/CombatComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Core/DesertGameInstance.h"
#include "SaveSystem/DesertSaveGame.h"
#include "Attributes/AttributeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Tools/ToolUseComponent.h"
#include "UI/InventoryHubWidget.h"
#include "UI/DeathMenuWidget.h"
#include "UI/PauseMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerSpringArmComponent.h"
#include "Environment/DayNightCycleManager.h"
#include "Environment/CampfireActor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AProtagonistCharacter::AProtagonistCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate character to controller yaw — let movement orient instead
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Movement settings
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->bOrientRotationToMovement = true;
	CMC->RotationRate = FRotator(0.f, 540.f, 0.f);
	CMC->JumpZVelocity = 600.f;
	CMC->AirControl = 0.2f;
	CMC->NavAgentProps.bCanCrouch = true;

	// Camera boom — custom subclass that ignores enemies/items in its collision probe
	CameraBoom = CreateDefaultSubobject<UPlayerSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 0.f); // Slight right offset for exploration

	// Collision test stays on so static walls still push the camera, but our
	// UPlayerSpringArmComponent override ensures enemies/items/resource nodes
	// can't push it (they're forced to ignore ECC_Camera every tick).
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Traversal
	TraversalComponent = CreateDefaultSubobject<UTraversalComponent>(TEXT("TraversalComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// Combat
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// Attributes
	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));

	// Inventory
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Tool use
	ToolUseComponent = CreateDefaultSubobject<UToolUseComponent>(TEXT("ToolUseComponent"));

	// Equipped tool mesh — socket attachment is deferred to BeginPlay
	EquippedToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquippedToolMesh"));
	EquippedToolMesh->SetupAttachment(GetMesh());
	EquippedToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Cloak / cape overlay. Driven by the body skeleton via Leader Pose (set in
	// BeginPlay), so it automatically inherits all animations without its own AnimBP.
	// Assign the skeletal mesh in BP_ProtagonistCharacter (must share the body skeleton).
	CloakMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CloakMesh"));
	CloakMesh->SetupAttachment(GetMesh());
	CloakMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Don't run a separate animation pipeline — pose comes from the body.
	CloakMesh->SetAnimationMode(EAnimationMode::AnimationCustomMode);

	// AI Perception — make this character visible to enemy AI
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComp"));
	StimuliSourceComponent->RegisterForSense(TSubclassOf<UAISense>(UAISense_Sight::StaticClass()));
	StimuliSourceComponent->bAutoRegister = true;
}

void AProtagonistCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Subscribe to attribute events
	if (AttributeComponent)
	{
		AttributeComponent->OnStaminaDepleted.AddDynamic(this, &AProtagonistCharacter::HandleStaminaDepleted);
		AttributeComponent->OnDeath.AddDynamic(this, &AProtagonistCharacter::HandlePlayerDeath);
	}

	// Attach equipped tool mesh to the correct socket — must happen after Super::BeginPlay
	// because the skeletal mesh component resolves socket names only once the skeleton is loaded.
	if (EquippedToolMesh && GetMesh())
	{
		EquippedToolMesh->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ToolAttachSocket);
	}

	// Drive the cloak's skeleton from the body's skeleton. The cloak mesh asset
	// must use the same skeleton as the body for this to work.
	if (CloakMesh && GetMesh())
	{
		CloakMesh->SetLeaderPoseComponent(GetMesh());
	}

	// Cache the default body mesh so we can restore it when a body-swap outfit
	// (e.g. jacket) is removed from the cloth slot.
	if (GetMesh())
	{
		DefaultBodyMesh = GetMesh()->GetSkeletalMeshAsset();
	}

	// Subscribe to hotbar selection changes to update equipped mesh
	if (InventoryComponent)
	{
		InventoryComponent->OnSelectedHotbarChanged.AddDynamic(this, &AProtagonistCharacter::HandleSelectedHotbarChanged);
		InventoryComponent->OnClothChanged.AddDynamic(this, &AProtagonistCharacter::HandleClothChanged);

		// Apply the current cloth slot in case the inventory was loaded with one
		// already equipped (save game).
		ApplyClothFromInventory();
	}

	// Apply pending save load (if requested via main menu "Continue")
	if (UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		if (GI->ShouldApplyLoadedSave())
		{
			if (UDesertSaveGame* SaveData = GI->LoadSaveGameData())
			{
				SetActorLocation(SaveData->PlayerLocation);
				SetActorRotation(SaveData->PlayerRotation);
				// TODO: apply StanceState through Crouch()/UnCrouch() once stance restoration flow is finalized

				// Restore picked-up world items set so AItemActor::BeginPlay can self-destroy
				GI->RestorePickedUpItemsFrom(SaveData->PickedUpItemIds);

				// Restore inventory contents
				if (InventoryComponent)
				{
					InventoryComponent->ApplyLoadedSlots(SaveData->InventorySlots, SaveData->HotbarSlots);
				}
			}
			GI->ConsumeLoadRequest();
		}
	}
}

void AProtagonistCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentSpeed = GetVelocity().Size();

	UpdateMovementState();
	UpdateGaitState();
	UpdateMaxSpeed();

	TickEnvironmentEffects(DeltaTime);
	TickFootsteps(DeltaTime);
}

void AProtagonistCharacter::TickEnvironmentEffects(float DeltaTime)
{
	if (!AttributeComponent)
	{
		return;
	}

	ADayNightCycleManager* Cycle = ADayNightCycleManager::Get(this);
	if (!Cycle)
	{
		// No day/night manager on the map — reset to neutral values.
		AttributeComponent->SetEnvironmentMultipliers(1.f, 1.f);
		if (bSufferingHeat) { bSufferingHeat = false; OnHeatSufferChanged(false); }
		if (bSufferingCold) { bSufferingCold = false; OnColdSufferChanged(false); }
		return;
	}

	// What is the player wearing (cloth slot)?
	EClothProtection Protection = EClothProtection::None;
	if (InventoryComponent)
	{
		const FInventorySlot& Cloth = InventoryComponent->GetClothSlot();
		if (!Cloth.IsEmpty() && Cloth.ItemData)
		{
			Protection = Cloth.ItemData->Protection;
		}
	}

	const bool bIsDay = Cycle->IsDay();

	// 1) Drain multipliers — apply unconditionally based on phase, even when
	//    the player is properly dressed. Day = thirst x mult, Night = hunger x mult.
	if (bIsDay)
	{
		AttributeComponent->SetEnvironmentMultipliers(Cycle->DayThirstMultiplier, 1.f);
	}
	else
	{
		AttributeComponent->SetEnvironmentMultipliers(1.f, Cycle->NightHungerMultiplier);
	}

	// 2) Suffering check — only when the player is alive and lacks the matching cloth.
	const bool bWantsHeatProt = bIsDay;
	const bool bWantsColdProt = !bIsDay;

	// A nearby active campfire fully shelters the player from cold. We only do
	// the lookup at night when it actually matters.
	const bool bNearCampfire = bWantsColdProt
		&& ACampfireActor::IsLocationWarmed(this, GetActorLocation());

	const bool bNewSufferingHeat = bWantsHeatProt && Protection != EClothProtection::Heat && AttributeComponent->IsAlive();
	const bool bNewSufferingCold = bWantsColdProt && Protection != EClothProtection::Cold && !bNearCampfire && AttributeComponent->IsAlive();

	if (bNewSufferingHeat != bSufferingHeat)
	{
		bSufferingHeat = bNewSufferingHeat;
		OnHeatSufferChanged(bSufferingHeat);
	}
	if (bNewSufferingCold != bSufferingCold)
	{
		bSufferingCold = bNewSufferingCold;
		OnColdSufferChanged(bSufferingCold);
	}

	// 3) HP drain when suffering.
	if (bSufferingHeat && Cycle->DayUnprotectedHealthDrainPerSecond > 0.f)
	{
		AttributeComponent->ApplyDamage(Cycle->DayUnprotectedHealthDrainPerSecond * DeltaTime);
	}
	else if (bSufferingCold && Cycle->NightUnprotectedHealthDrainPerSecond > 0.f)
	{
		AttributeComponent->ApplyDamage(Cycle->NightUnprotectedHealthDrainPerSecond * DeltaTime);
	}
}

void AProtagonistCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput) return;

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Move);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Look);
	}

	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AProtagonistCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::StopJump);
	}

	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AProtagonistCharacter::StartSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::StopSprint);
	}

	if (CrouchAction)
	{
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &AProtagonistCharacter::ToggleCrouch);
	}

	if (AttackAction)
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnAttackInput);
	}

	if (BlockAction)
	{
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnBlockStart);
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::OnBlockStop);
	}

	if (DodgeAction)
	{
		EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnDodgeInput);
	}

	if (SaveGameAction)
	{
		EnhancedInput->BindAction(SaveGameAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnSaveGameInput);
	}

	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnInteractInput);
	}

	if (ToggleInventoryAction)
	{
		EnhancedInput->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnToggleInventoryInput);
	}

	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnPauseInput);
	}

	if (HotbarSlot1Action)
	{
		EnhancedInput->BindAction(HotbarSlot1Action, ETriggerEvent::Started, this, &AProtagonistCharacter::OnHotbarSlot1);
	}
	if (HotbarSlot2Action)
	{
		EnhancedInput->BindAction(HotbarSlot2Action, ETriggerEvent::Started, this, &AProtagonistCharacter::OnHotbarSlot2);
	}
	if (HotbarSlot3Action)
	{
		EnhancedInput->BindAction(HotbarSlot3Action, ETriggerEvent::Started, this, &AProtagonistCharacter::OnHotbarSlot3);
	}
	if (HotbarSlot4Action)
	{
		EnhancedInput->BindAction(HotbarSlot4Action, ETriggerEvent::Started, this, &AProtagonistCharacter::OnHotbarSlot4);
	}
}

// ============================================================
// Input Handlers
// ============================================================

void AProtagonistCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (Controller && !Input.IsNearlyZero())
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, Input.Y);
		AddMovementInput(Right, Input.X);
	}
}

void AProtagonistCharacter::Look(const FInputActionValue& Value)
{
	if (!bLookEnabled) return;

	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(-Input.Y);
}

void AProtagonistCharacter::EnableLookInput()
{
	bLookEnabled = true;
}

void AProtagonistCharacter::DisableLookInput()
{
	bLookEnabled = false;
}

void AProtagonistCharacter::StartJump()
{
	// Try traversal first — if obstacle detected, traverse instead of jumping
	if (TraversalComponent && TraversalComponent->CanPerformTraversal())
	{
		const FTraversalCheckResult Result = TraversalComponent->PerformTraversalCheck();
		if (Result.bIsValid())
		{
			TraversalComponent->ExecuteTraversal(Result);
			return;
		}
	}

	// No obstacle — normal jump
	Jump();
}

void AProtagonistCharacter::StopJump()
{
	StopJumping();
}

void AProtagonistCharacter::StartSprint()
{
	bWantsToSprint = true;
}

void AProtagonistCharacter::StopSprint()
{
	bWantsToSprint = false;
}

void AProtagonistCharacter::ToggleCrouch()
{
	if (CurrentStanceState == EStanceState::Standing)
	{
		Crouch();
		CurrentStanceState = EStanceState::Crouching;
	}
	else
	{
		UnCrouch();
		CurrentStanceState = EStanceState::Standing;
	}
}

void AProtagonistCharacter::OnAttackInput()
{
	if (!InventoryComponent)
	{
		// No inventory means we have no way of knowing what's equipped — bail.
		return;
	}

	UItemDataAsset* Selected = InventoryComponent->GetSelectedItem();

	// Consumable takes priority over tool: a consumable in the selected hotbar slot
	// is "used" on LMB, restoring stats and decrementing the stack by 1.
	if (Selected && Selected->bIsConsumable)
	{
		if (AttributeComponent)
		{
			if (Selected->HungerRestore > 0.f) AttributeComponent->RestoreHunger(Selected->HungerRestore);
			if (Selected->ThirstRestore > 0.f) AttributeComponent->RestoreThirst(Selected->ThirstRestore);
			if (Selected->HealthRestore > 0.f) AttributeComponent->Heal(Selected->HealthRestore);
		}
		InventoryComponent->ConsumeSelectedItem();
		return;
	}

	// LMB only does something when a tool is in hand:
	//   Sword           → combo attack via CombatComponent
	//   Axe / Pickaxe   → ToolUseComponent
	//   anything else   → no-op (no fists/punches without a weapon)
	if (!Selected || Selected->ToolType == EToolType::None)
	{
		return;
	}

	if (Selected->ToolType == EToolType::Sword)
	{
		if (ToolUseComponent) ToolUseComponent->ArmTool();
		if (CombatComponent) CombatComponent->RequestAttack();
	}
	else
	{
		if (ToolUseComponent) ToolUseComponent->RequestUseTool();
	}
}

void AProtagonistCharacter::HandleSelectedHotbarChanged(int32 NewIndex)
{
	if (!EquippedToolMesh || !InventoryComponent)
	{
		return;
	}

	UItemDataAsset* Selected = InventoryComponent->GetSelectedItem();
	if (!Selected)
	{
		EquippedToolMesh->SetStaticMesh(nullptr);
		return;
	}

	UStaticMesh* MeshToShow = Selected->EquipMesh ? Selected->EquipMesh : Selected->WorldMesh;
	EquippedToolMesh->SetStaticMesh(MeshToShow);
}

void AProtagonistCharacter::HandleClothChanged()
{
	ApplyClothFromInventory();
}

void AProtagonistCharacter::PlayFootstepSound()
{
	if (FootstepSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation());
	}
}

void AProtagonistCharacter::TickFootsteps(float DeltaTime)
{
	if (!FootstepSound)
	{
		return;
	}

	// Only on the ground — no footstep sounds while falling/jumping.
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC || !CMC->IsMovingOnGround())
	{
		DistanceSinceLastFootstep = 0.f;
		return;
	}

	// Use horizontal speed so vertical bobs don't add to the stride budget.
	const FVector Velocity = GetVelocity();
	const float HorizontalSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();

	// Idle / barely moving: reset so the first step after resuming movement
	// fires shortly, not "however much travel was banked before the stop".
	if (HorizontalSpeed < 10.f)
	{
		DistanceSinceLastFootstep = 0.f;
		return;
	}

	DistanceSinceLastFootstep += HorizontalSpeed * DeltaTime;

	const float Stride = (CurrentGaitState == EGaitState::Sprint)
		? FootstepSprintStrideCm
		: FootstepStrideCm;

	if (Stride > 0.f && DistanceSinceLastFootstep >= Stride)
	{
		DistanceSinceLastFootstep = 0.f;
		PlayFootstepSound();
	}
}

void AProtagonistCharacter::ApplyClothFromInventory()
{
	if (!InventoryComponent || !GetMesh())
	{
		return;
	}

	const FInventorySlot& Cloth = InventoryComponent->GetClothSlot();
	UItemDataAsset* Item = Cloth.IsEmpty() ? nullptr : Cloth.ItemData.Get();

	// Default state: nothing equipped — restore body, hide cloak.
	if (!Item || Item->Category != EItemCategory::Cloth)
	{
		if (DefaultBodyMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(DefaultBodyMesh);
		}
		if (CloakMesh)
		{
			CloakMesh->SetSkeletalMeshAsset(nullptr);
		}
		return;
	}

	switch (Item->ClothEquipMode)
	{
	case EClothEquipMode::Overlay:
		// Cloak overlay: body stays default, cloak gets the item's mesh.
		if (DefaultBodyMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(DefaultBodyMesh);
		}
		if (CloakMesh)
		{
			CloakMesh->SetSkeletalMeshAsset(Item->ClothSkeletalMesh);
			// Re-link leader pose in case the mesh swap reset the relationship.
			CloakMesh->SetLeaderPoseComponent(GetMesh());
		}
		break;

	case EClothEquipMode::BodySwap:
		// Full outfit replaces the body; clear any cloak so we don't double-up.
		if (Item->ClothSkeletalMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(Item->ClothSkeletalMesh);
		}
		if (CloakMesh)
		{
			CloakMesh->SetSkeletalMeshAsset(nullptr);
		}
		break;
	}
}

void AProtagonistCharacter::OnBlockStart()
{
	if (CombatComponent)
	{
		CombatComponent->StartBlock();
	}
}

void AProtagonistCharacter::OnBlockStop()
{
	if (CombatComponent)
	{
		CombatComponent->StopBlock();
	}
}

void AProtagonistCharacter::OnDodgeInput(const FInputActionValue& Value)
{
	if (!CombatComponent || !Controller) return;

	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero()) return;

	// Convert WASD input into a world-space direction using the camera's yaw,
	// so that "W" always means "away from the camera" regardless of where the
	// character is currently facing.
	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector CamForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector CamRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	FVector WorldDir = CamForward * Input.Y + CamRight * Input.X;
	WorldDir.Z = 0.f;
	if (!WorldDir.Normalize())
	{
		return;
	}

	// Pick which dodge animation to play by projecting the world dodge direction
	// onto the body's own forward/right axes. This keeps the animation correct
	// for the character's current facing (e.g. dodging "into the camera" while
	// facing the camera plays the backward animation).
	const FVector BodyForward = GetActorForwardVector();
	const FVector BodyRight = GetActorRightVector();
	const float ForwardDot = FVector::DotProduct(WorldDir, BodyForward);
	const float RightDot = FVector::DotProduct(WorldDir, BodyRight);

	EDodgeDirection Direction;
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		Direction = (ForwardDot >= 0.f) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
	}
	else
	{
		Direction = (RightDot >= 0.f) ? EDodgeDirection::Right : EDodgeDirection::Left;
	}

	CombatComponent->RequestDodge(Direction, WorldDir);
}

void AProtagonistCharacter::HandleStaminaDepleted()
{
	if (CurrentGaitState == EGaitState::Sprint)
	{
		CurrentGaitState = EGaitState::Run;
		UpdateMaxSpeed();
	}
}

void AProtagonistCharacter::OnSaveGameInput()
{
	if (UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		GI->SaveGame(this);
	}
}

void AProtagonistCharacter::OnInteractInput()
{
	if (CurrentInteractable && InventoryComponent)
	{
		CurrentInteractable->TryPickup(this);
	}
}

void AProtagonistCharacter::OnToggleInventoryInput()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bIsInventoryOpen)
	{
		if (!HubWidgetClass)
		{
			UE_LOG(LogDesertInventory, Warning, TEXT("HubWidgetClass not set on ProtagonistCharacter"));
			return;
		}

		if (!HubWidgetInstance)
		{
			HubWidgetInstance = CreateWidget<UInventoryHubWidget>(PC, HubWidgetClass);
		}

		if (HubWidgetInstance)
		{
			HubWidgetInstance->AddToViewport();

			FInputModeGameAndUI Mode;
			Mode.SetWidgetToFocus(HubWidgetInstance->TakeWidget());
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->SetShowMouseCursor(true);

			bIsInventoryOpen = true;
		}
	}
	else
	{
		if (HubWidgetInstance)
		{
			HubWidgetInstance->RemoveFromParent();
		}

		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(false);

		bIsInventoryOpen = false;
	}
}

void AProtagonistCharacter::OnPauseInput()
{
	// Ignore pause while dead — the death menu owns the screen.
	if (bIsDead)
	{
		return;
	}
	TogglePauseMenu();
}

void AProtagonistCharacter::TogglePauseMenu()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bIsPauseMenuOpen)
	{
		if (!PauseMenuWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PauseMenuWidgetClass not set on ProtagonistCharacter"));
			return;
		}

		if (!PauseMenuInstance)
		{
			PauseMenuInstance = CreateWidget<UPauseMenuWidget>(PC, PauseMenuWidgetClass);
		}
		if (!PauseMenuInstance)
		{
			return;
		}

		PauseMenuInstance->AddToViewport(100);

		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);

		UGameplayStatics::SetGamePaused(this, true);
		bIsPauseMenuOpen = true;
	}
	else
	{
		if (PauseMenuInstance)
		{
			PauseMenuInstance->RemoveFromParent();
		}

		UGameplayStatics::SetGamePaused(this, false);

		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(false);

		bIsPauseMenuOpen = false;
	}
}

void AProtagonistCharacter::HandlePlayerDeath()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	// If the pause menu happened to be open at the moment of death, close it
	// first so we don't stack two modal widgets.
	if (bIsPauseMenuOpen && PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		bIsPauseMenuOpen = false;
	}

	// Stop the world so the player can't keep getting hit / moved while the
	// death screen is up.
	UGameplayStatics::SetGamePaused(this, true);

	ShowDeathMenu();
}

void AProtagonistCharacter::ShowDeathMenu()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !DeathMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeathMenuWidgetClass not set on ProtagonistCharacter"));
		return;
	}

	if (!DeathMenuInstance)
	{
		DeathMenuInstance = CreateWidget<UDeathMenuWidget>(PC, DeathMenuWidgetClass);
	}
	if (!DeathMenuInstance)
	{
		return;
	}

	DeathMenuInstance->AddToViewport(200);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(DeathMenuInstance->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
}

void AProtagonistCharacter::SetCurrentInteractable(AItemActor* Item)
{
	CurrentInteractable = Item;
}

void AProtagonistCharacter::ClearInteractableIfMatches(AItemActor* Item)
{
	if (CurrentInteractable == Item)
	{
		CurrentInteractable = nullptr;
	}
}

void AProtagonistCharacter::OnHotbarSlot1() { SelectHotbarIndex(0); }
void AProtagonistCharacter::OnHotbarSlot2() { SelectHotbarIndex(1); }
void AProtagonistCharacter::OnHotbarSlot3() { SelectHotbarIndex(2); }
void AProtagonistCharacter::OnHotbarSlot4() { SelectHotbarIndex(3); }

void AProtagonistCharacter::SelectHotbarIndex(int32 Index)
{
	if (InventoryComponent)
	{
		InventoryComponent->SelectHotbarSlot(Index);
	}
}

// ============================================================
// State Updates
// ============================================================

void AProtagonistCharacter::UpdateMovementState()
{
	const UCharacterMovementComponent* CMC = GetCharacterMovement();

	if (CMC->IsFalling())
	{
		CurrentMovementState = (GetVelocity().Z > 0.f)
			? EMovementState::Jumping
			: EMovementState::Falling;
	}
	else
	{
		CurrentMovementState = EMovementState::Grounded;
	}
}

void AProtagonistCharacter::UpdateGaitState()
{
	const float GroundSpeed = GetVelocity().Size2D();

	if (GroundSpeed < 5.f)
	{
		// Essentially idle — keep current gait but don't force walk
		return;
	}

	const bool bWasSprinting = (CurrentGaitState == EGaitState::Sprint);

	// Sprint requires stamina: gate entry by CanStartSprint, but allow continuing
	// while there is any stamina left so the player isn't kicked out at the threshold.
	const bool bHasStaminaForSprint = AttributeComponent
		? (bWasSprinting ? AttributeComponent->GetStamina() > 0.0f : AttributeComponent->CanStartSprint())
		: true;

	if (bWantsToSprint && bHasStaminaForSprint && GroundSpeed > RunSpeed * 0.5f)
	{
		CurrentGaitState = EGaitState::Sprint;
	}
	else if (GroundSpeed > WalkSpeed * 0.8f)
	{
		CurrentGaitState = EGaitState::Run;
	}
	else
	{
		CurrentGaitState = EGaitState::Walk;
	}

	// Sync stamina draining with actual sprint state
	if (AttributeComponent)
	{
		const bool bIsSprintingNow = (CurrentGaitState == EGaitState::Sprint);
		if (bIsSprintingNow != bWasSprinting)
		{
			AttributeComponent->SetSprintingState(bIsSprintingNow);
		}
	}
}

void AProtagonistCharacter::UpdateMaxSpeed()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();

	float TargetSpeed = RunSpeed;

	switch (CurrentGaitState)
	{
	case EGaitState::Walk:
		TargetSpeed = WalkSpeed;
		break;
	case EGaitState::Run:
		TargetSpeed = RunSpeed;
		break;
	case EGaitState::Sprint:
		TargetSpeed = SprintSpeed;
		break;
	}

	if (CurrentStanceState == EStanceState::Crouching)
	{
		TargetSpeed *= CrouchSpeedMultiplier;
	}

	CMC->MaxWalkSpeed = TargetSpeed;
}
