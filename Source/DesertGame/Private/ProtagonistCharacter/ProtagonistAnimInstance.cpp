// Fill out your copyright notice in the Description page of Project Settings.

#include "ProtagonistCharacter/ProtagonistAnimInstance.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Traversal/TraversalComponent.h"

void UProtagonistAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn) return;

	OwningCharacter = Cast<AProtagonistCharacter>(Pawn);
	if (OwningCharacter)
	{
		MovementComponent = OwningCharacter->GetCharacterMovement();
		TraversalComponent = OwningCharacter->FindComponentByClass<UTraversalComponent>();
	}
}

void UProtagonistAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !MovementComponent) return;

	const FVector Velocity = OwningCharacter->GetVelocity();

	Speed = Velocity.Size();
	GroundSpeed = Velocity.Size2D();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwningCharacter->GetActorRotation());

	bIsFalling = MovementComponent->IsFalling();
	bIsCrouching = MovementComponent->IsCrouching();
	bShouldMove = GroundSpeed > 3.f && MovementComponent->GetCurrentAcceleration().Size() > 0.f;
	bIsJumping = bIsFalling && Velocity.Z > 0.f;

	GaitState = OwningCharacter->CurrentGaitState;
	StanceState = OwningCharacter->CurrentStanceState;

	// Traversal
	if (TraversalComponent)
	{
		bIsPerformingTraversal = TraversalComponent->bIsPerformingTraversal;
		CurrentTraversalAction = TraversalComponent->LastCheckResult.Action;
	}
}
