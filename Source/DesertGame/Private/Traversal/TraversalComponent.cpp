// Fill out your copyright notice in the Description page of Project Settings.

#include "Traversal/TraversalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogTraversal, Log, All);

UTraversalComponent::UTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureCachedPointers();
}

void UTraversalComponent::EnsureCachedPointers()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}
	if (!OwnerCharacter)
	{
		return;
	}

	if (!MovementComponent)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}

	if (!MotionWarpingComponent)
	{
		MotionWarpingComponent = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();
	}

	if (!AnimInstance && OwnerCharacter->GetMesh())
	{
		AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	}

	if (!CameraBoom)
	{
		CameraBoom = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
	}
}

// ============================================================
// Public API
// ============================================================

FTraversalCheckResult UTraversalComponent::PerformTraversalCheck()
{
	LastCheckResult = FTraversalCheckResult();

	// Lazy init — in case BeginPlay didn't have valid owner yet
	EnsureCachedPointers();

	if (!CanPerformTraversal())
	{
		return LastCheckResult;
	}

	const float FeetZ = OwnerCharacter->GetActorLocation().Z - OwnerCharacter->GetSimpleCollisionHalfHeight();
	UE_LOG(LogTraversal, Log, TEXT("PerformTraversalCheck: CharLocation=%s, FeetZ=%.1f, Forward=%s"),
		*OwnerCharacter->GetActorLocation().ToString(), FeetZ, *OwnerCharacter->GetActorForwardVector().ToString());

	// Step 1: Forward trace to find wall
	FHitResult WallHit;
	if (!ForwardTrace(WallHit))
	{
		UE_LOG(LogTraversal, Log, TEXT("ForwardTrace: No wall hit"));
		return LastCheckResult;
	}

	LastCheckResult.WallHitLocation = WallHit.ImpactPoint;
	LastCheckResult.WallHitNormal = WallHit.ImpactNormal;
	LastCheckResult.HitActor = WallHit.GetActor();
	UE_LOG(LogTraversal, Log, TEXT("ForwardTrace: Hit at (%.1f, %.1f, %.1f), HeightFromFeet=%.1f"),
		WallHit.ImpactPoint.X, WallHit.ImpactPoint.Y, WallHit.ImpactPoint.Z, WallHit.ImpactPoint.Z - FeetZ);

	// Step 2: Height trace to find ledge
	FVector LedgeLocation;
	if (!HeightTrace(WallHit, LedgeLocation))
	{
		UE_LOG(LogTraversal, Log, TEXT("HeightTrace: No ledge found"));
		return LastCheckResult;
	}

	LastCheckResult.LedgeLocation = LedgeLocation;
	const float CharacterFeetZ = OwnerCharacter->GetActorLocation().Z - OwnerCharacter->GetSimpleCollisionHalfHeight();
	LastCheckResult.ObstacleHeight = LedgeLocation.Z - CharacterFeetZ;
	UE_LOG(LogTraversal, Log, TEXT("Height calc: LedgeZ=%.1f, FeetZ=%.1f, ObstacleHeight=%.1f"),
		LedgeLocation.Z, CharacterFeetZ, LastCheckResult.ObstacleHeight);

	// Skip if height is out of range
	if (LastCheckResult.ObstacleHeight < TraceSettings.MinObstacleHeight ||
		LastCheckResult.ObstacleHeight > TraceSettings.MaxObstacleHeight)
	{
		UE_LOG(LogTraversal, Log, TEXT("Height out of range: %.1f (min=%.1f, max=%.1f)"),
			LastCheckResult.ObstacleHeight, TraceSettings.MinObstacleHeight, TraceSettings.MaxObstacleHeight);
		return LastCheckResult;
	}

	// Step 3: Depth trace to determine obstacle thickness
	float Depth = 0.f;
	DepthTrace(LedgeLocation, WallHit.ImpactNormal, Depth);
	LastCheckResult.ObstacleDepth = Depth;
	UE_LOG(LogTraversal, Log, TEXT("DepthTrace: ObstacleDepth=%.1f"), Depth);

	// Step 4: Room check above ledge
	const bool bHasRoom = RoomCheck(LedgeLocation);
	LastCheckResult.bHasRoom = bHasRoom;
	UE_LOG(LogTraversal, Log, TEXT("RoomCheck: bHasRoom=%s"), bHasRoom ? TEXT("true") : TEXT("false"));

	// Step 5: Classify
	LastCheckResult.Action = ClassifyObstacle(
		LastCheckResult.ObstacleHeight,
		LastCheckResult.ObstacleDepth,
		bHasRoom);
	UE_LOG(LogTraversal, Log, TEXT("ClassifyObstacle: Action=%d (Height=%.1f, Depth=%.1f, Room=%s)"),
		static_cast<uint8>(LastCheckResult.Action), LastCheckResult.ObstacleHeight, LastCheckResult.ObstacleDepth,
		bHasRoom ? TEXT("true") : TEXT("false"));

	// Debug: visualize final result
	if (bDebugDraw && LastCheckResult.bIsValid())
	{
		DrawDebugSphere(GetWorld(), LastCheckResult.LedgeLocation, 15.f, 12, FColor::Magenta, false, DebugDrawDuration * 2);
		DrawDebugString(GetWorld(), LastCheckResult.LedgeLocation + FVector(0.f, 0.f, 30.f),
			*UEnum::GetValueAsString(LastCheckResult.Action), nullptr, FColor::White, DebugDrawDuration * 2);
	}

	return LastCheckResult;
}

void UTraversalComponent::ExecuteTraversal(const FTraversalCheckResult& Result)
{
	if (!Result.bIsValid() || bIsPerformingTraversal)
	{
		return;
	}

	// Find montage for this action
	const TObjectPtr<UAnimMontage>* MontagePtr = TraversalMontages.Find(Result.Action);
	if (!MontagePtr || !*MontagePtr)
	{
		UE_LOG(LogTraversal, Warning, TEXT("TraversalComponent: No montage assigned for action %d"), static_cast<uint8>(Result.Action));
		return;
	}

	if (!AnimInstance)
	{
		return;
	}

	// Set motion warping target
	if (MotionWarpingComponent)
	{
		const FRotator WarpRotation = (-Result.WallHitNormal).Rotation();
		FVector WarpLocation;

		switch (Result.Action)
		{
		case ETraversalAction::Vault:
		case ETraversalAction::ClimbOver:
			// Landing point BEHIND the obstacle
			WarpLocation = Result.LedgeLocation + (-Result.WallHitNormal * TraceSettings.ClimbOverLandingOffset);
			WarpLocation.Z = Result.LedgeLocation.Z;
			break;

		case ETraversalAction::MantleLow:
			// On top of the obstacle, slightly past the edge
			WarpLocation = Result.LedgeLocation + (-Result.WallHitNormal * 30.f);
			WarpLocation.Z = Result.LedgeLocation.Z;
			break;

		case ETraversalAction::MantleHigh:
			// On top of the obstacle, slightly past the edge + vertical offset
			WarpLocation = Result.LedgeLocation + (-Result.WallHitNormal * 30.f);
			WarpLocation.Z = Result.LedgeLocation.Z + TraceSettings.MantleHighZOffset;
			break;

		case ETraversalAction::LedgeGrab:
			// Grab the ledge edge
			WarpLocation = Result.LedgeLocation;
			break;

		default:
			WarpLocation = Result.LedgeLocation;
			break;
		}

		UE_LOG(LogTraversal, Log, TEXT("WarpTarget: %s (Action=%d)"), *WarpLocation.ToString(), static_cast<int32>(Result.Action));

		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
			WarpTargetName,
			WarpLocation,
			WarpRotation);
	}

	// Play montage
	UAnimMontage* Montage = *MontagePtr;
	const float PlayRate = AnimInstance->Montage_Play(Montage);
	if (PlayRate > 0.f)
	{
		bIsPerformingTraversal = true;

		// Ignore the specific obstacle so capsule passes through it
		if (Result.HitActor.IsValid())
		{
			CurrentTraversalObstacle = Result.HitActor;
			OwnerCharacter->MoveIgnoreActorAdd(Result.HitActor.Get());
			UE_LOG(LogTraversal, Log, TEXT("ExecuteTraversal: Ignoring obstacle: %s"), *Result.HitActor->GetName());
		}

		// Disable camera collision so SpringArm doesn't clip into character
		if (CameraBoom)
		{
			CameraBoom->bDoCollisionTest = false;
		}

		// Switch to flying so root motion moves the capsule freely
		MovementComponent->SetMovementMode(MOVE_Flying);

		// Bind end delegate
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UTraversalComponent::OnTraversalMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
}

bool UTraversalComponent::CanPerformTraversal() const
{
	if (!OwnerCharacter || !MovementComponent || !AnimInstance)
	{
		UE_LOG(LogTraversal, Warning, TEXT("CanPerformTraversal: BLOCKED - missing ptr (Char=%s, CMC=%s, Anim=%s)"),
			OwnerCharacter ? TEXT("OK") : TEXT("NULL"),
			MovementComponent ? TEXT("OK") : TEXT("NULL"),
			AnimInstance ? TEXT("OK") : TEXT("NULL"));
		return false;
	}

	if (bIsPerformingTraversal)
	{
		UE_LOG(LogTraversal, Warning, TEXT("CanPerformTraversal: BLOCKED - already performing traversal"));
		return false;
	}

	if (!MovementComponent->IsMovingOnGround())
	{
		UE_LOG(LogTraversal, Warning, TEXT("CanPerformTraversal: BLOCKED - not on ground (MovementMode=%d)"),
			static_cast<uint8>(MovementComponent->MovementMode));
		return false;
	}

	return true;
}

// ============================================================
// Trace Methods
// ============================================================

bool UTraversalComponent::ForwardTrace(FHitResult& OutHit)
{
	const FVector ActorLocation = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const TArray<AActor*> ActorsToIgnore = { OwnerCharacter };

	const EDrawDebugTrace::Type DebugType = bDebugDraw
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	// Trace at multiple heights: 50, 90, 130 cm from character FEET (not capsule center)
	const float FeetZ = ActorLocation.Z - OwnerCharacter->GetSimpleCollisionHalfHeight();
	const float TraceHeights[] = { 50.f, 90.f, 130.f };

	for (const float Height : TraceHeights)
	{
		const FVector Start = FVector(ActorLocation.X, ActorLocation.Y, FeetZ + Height);
		const FVector End = Start + Forward * TraceSettings.MaxForwardDistance;

		UE_LOG(LogTraversal, Log, TEXT("ForwardTrace [Height %.1f]: Start=%s End=%s"), Height, *Start.ToString(), *End.ToString());

		bool bHit = UKismetSystemLibrary::SphereTraceSingle(
			OwnerCharacter,
			Start,
			End,
			TraceSettings.ForwardTraceRadius,
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false,
			ActorsToIgnore,
			DebugType,
			OutHit,
			true,
			FLinearColor::Red,
			FLinearColor::Green,
			DebugDrawDuration);

		// Manual debug draw for guaranteed visibility
		if (bDebugDraw)
		{
			DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Green, false, DebugDrawDuration, 0, 2.f);
			DrawDebugSphere(GetWorld(), Start, TraceSettings.ForwardTraceRadius, 8, FColor::Yellow, false, DebugDrawDuration);
			if (bHit)
			{
				DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 10.f, 8, FColor::Red, false, DebugDrawDuration);
				DrawDebugDirectionalArrow(GetWorld(), OutHit.ImpactPoint,
					OutHit.ImpactPoint + OutHit.ImpactNormal * 50.f, 10.f, FColor::Blue, false, DebugDrawDuration);
			}
		}

		if (bHit)
		{
			UE_LOG(LogTraversal, Log, TEXT("ForwardTrace HIT: %s at %s"),
				OutHit.GetActor() ? *OutHit.GetActor()->GetName() : TEXT("None"), *OutHit.ImpactPoint.ToString());
			return true;
		}
		else
		{
			UE_LOG(LogTraversal, Log, TEXT("ForwardTrace [Height %.1f]: MISS"), Height);
		}
	}

	return false;
}

bool UTraversalComponent::HeightTrace(const FHitResult& WallHit, FVector& OutLedgeLocation)
{
	// Trace down from above the wall hit point
	const FVector WallPoint = WallHit.ImpactPoint;
	const FVector WallNormal = WallHit.ImpactNormal;

	// Offset slightly into the wall surface to ensure we're above the obstacle
	const FVector TraceXY = WallPoint - WallNormal * 5.f;
	const float FeetZ = OwnerCharacter->GetActorLocation().Z - OwnerCharacter->GetSimpleCollisionHalfHeight();

	const FVector Start = FVector(TraceXY.X, TraceXY.Y, FeetZ + TraceSettings.TopTraceStartHeight);
	const FVector End = FVector(TraceXY.X, TraceXY.Y, FeetZ);

	const TArray<AActor*> ActorsToIgnore = { OwnerCharacter };
	const EDrawDebugTrace::Type DebugType = bDebugDraw
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		OwnerCharacter,
		Start,
		End,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		DebugType,
		Hit,
		true,
		FLinearColor::Blue,
		FLinearColor::Yellow,
		DebugDrawDuration);

	// Manual debug draw
	if (bDebugDraw)
	{
		DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Blue : FColor::Yellow, false, DebugDrawDuration, 0, 2.f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 8, FColor::Orange, false, DebugDrawDuration);
		}
	}

	if (bHit)
	{
		OutLedgeLocation = Hit.ImpactPoint;
		return true;
	}

	return false;
}

bool UTraversalComponent::DepthTrace(const FVector& LedgeLocation, const FVector& WallNormal, float& OutDepth)
{
	// Trace down behind the obstacle to check its thickness
	const float FeetZ = OwnerCharacter->GetActorLocation().Z - OwnerCharacter->GetSimpleCollisionHalfHeight();
	const FVector BehindWall = LedgeLocation + (-WallNormal * TraceSettings.DepthCheckDistance) + FVector(0.f, 0.f, 10.f);
	const FVector TraceEnd = FVector(BehindWall.X, BehindWall.Y, FeetZ);

	const TArray<AActor*> ActorsToIgnore = { OwnerCharacter };
	const EDrawDebugTrace::Type DebugType = bDebugDraw
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		OwnerCharacter,
		BehindWall,
		TraceEnd,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		DebugType,
		Hit,
		true,
		FLinearColor::White,
		FLinearColor(1.f, 0.5f, 0.f),
		DebugDrawDuration);

	// Manual debug draw
	if (bDebugDraw)
	{
		DrawDebugLine(GetWorld(), BehindWall, TraceEnd, bHit ? FColor::Red : FColor::Cyan, false, DebugDrawDuration, 0, 2.f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 8, FColor::Cyan, false, DebugDrawDuration);
		}
	}

	if (bHit)
	{
		const float HeightDifference = LedgeLocation.Z - Hit.ImpactPoint.Z;
		if (HeightDifference > 50.f)
		{
			// Object is thin — the trace landed far below the ledge
			OutDepth = TraceSettings.DepthCheckDistance - FVector::Dist2D(LedgeLocation, Hit.ImpactPoint);
			OutDepth = FMath::Max(OutDepth, 0.f);
		}
		else
		{
			// Object is thick — the trace hit a surface near ledge height
			OutDepth = TraceSettings.DepthCheckDistance;
		}
	}
	else
	{
		// Nothing behind — treat as thin
		OutDepth = 0.f;
	}

	return true;
}

bool UTraversalComponent::RoomCheck(const FVector& LedgeLocation) const
{
	const FVector CheckLocation = LedgeLocation + FVector(0.f, 0.f, TraceSettings.CapsuleCheckHalfHeight + 5.f);

	const TArray<AActor*> ActorsToIgnore = { OwnerCharacter };
	const EDrawDebugTrace::Type DebugType = bDebugDraw
		? EDrawDebugTrace::ForDuration
		: EDrawDebugTrace::None;

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(
		OwnerCharacter,
		CheckLocation,
		CheckLocation, // Start == End — overlap check
		TraceSettings.CapsuleCheckRadius,
		TraceSettings.CapsuleCheckHalfHeight,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		DebugType,
		Hit,
		true,
		FLinearColor::Green,
		FLinearColor::Red,
		DebugDrawDuration);

	// Manual debug draw — green capsule outline if room, red if blocked
	if (bDebugDraw)
	{
		const FColor CapsuleColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(GetWorld(), CheckLocation, TraceSettings.CapsuleCheckHalfHeight,
			TraceSettings.CapsuleCheckRadius, FQuat::Identity, CapsuleColor, false, DebugDrawDuration, 0, 1.f);
	}

	// If nothing was hit, there IS room
	return !bHit;
}

ETraversalAction UTraversalComponent::ClassifyObstacle(float Height, float Depth, bool bHasRoom) const
{
	if (!bHasRoom)
	{
		return ETraversalAction::None;
	}

	if (Height < TraceSettings.MinObstacleHeight)
	{
		return ETraversalAction::None;
	}

	// ClimbOver takes priority from ClimbOverMinHeight for thin obstacles
	if (Height >= TraceSettings.ClimbOverMinHeight && Depth < TraceSettings.VaultMaxDepth && Height <= TraceSettings.HighObstacleMaxHeight)
	{
		return ETraversalAction::ClimbOver;
	}

	if (Height <= TraceSettings.LowObstacleMaxHeight)
	{
		return (Depth < TraceSettings.VaultMaxDepth) ? ETraversalAction::Vault : ETraversalAction::MantleLow;
	}

	if (Height <= TraceSettings.HighObstacleMaxHeight)
	{
		return ETraversalAction::MantleHigh;
	}

	if (Height <= TraceSettings.MaxObstacleHeight)
	{
		return ETraversalAction::LedgeGrab;
	}

	return ETraversalAction::None;
}

void UTraversalComponent::OnTraversalMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTraversal, Log, TEXT("OnMontageEnded: Interrupted=%d"), bInterrupted);

	bIsPerformingTraversal = false;

	// Stop ignoring the obstacle
	if (CurrentTraversalObstacle.IsValid() && OwnerCharacter)
	{
		OwnerCharacter->MoveIgnoreActorRemove(CurrentTraversalObstacle.Get());
		UE_LOG(LogTraversal, Log, TEXT("OnMontageEnded: Stopped ignoring obstacle: %s"), *CurrentTraversalObstacle->GetName());
		CurrentTraversalObstacle.Reset();
	}

	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	// Re-enable camera collision
	if (CameraBoom)
	{
		CameraBoom->bDoCollisionTest = true;
	}

	if (MotionWarpingComponent)
	{
		MotionWarpingComponent->RemoveWarpTarget(WarpTargetName);
	}
}
