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

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 0.f); // Slight right offset for exploration

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Traversal
	TraversalComponent = CreateDefaultSubobject<UTraversalComponent>(TEXT("TraversalComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// Combat
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
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
}

void AProtagonistCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentSpeed = GetVelocity().Size();

	UpdateMovementState();
	UpdateGaitState();
	UpdateMaxSpeed();
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
	if (CombatComponent)
	{
		CombatComponent->RequestAttack();
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
	if (!CombatComponent) return;

	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero()) return;

	// Determine dominant axis
	EDodgeDirection Direction;
	if (FMath::Abs(Input.Y) >= FMath::Abs(Input.X))
	{
		Direction = (Input.Y > 0.f) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
	}
	else
	{
		Direction = (Input.X > 0.f) ? EDodgeDirection::Right : EDodgeDirection::Left;
	}

	CombatComponent->RequestDodge(Direction);
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

	if (bWantsToSprint && GroundSpeed > RunSpeed * 0.5f)
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
