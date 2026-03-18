// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TraversalTypes.generated.h"

UENUM(BlueprintType)
enum class ETraversalAction : uint8
{
	None		UMETA(DisplayName = "None"),
	Vault		UMETA(DisplayName = "Vault"),
	MantleLow	UMETA(DisplayName = "Mantle Low"),
	MantleHigh	UMETA(DisplayName = "Mantle High"),
	ClimbOver	UMETA(DisplayName = "Climb Over"),
	LedgeGrab	UMETA(DisplayName = "Ledge Grab"),
	WallClimb	UMETA(DisplayName = "Wall Climb")
};

USTRUCT(BlueprintType)
struct FTraversalCheckResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	ETraversalAction Action = ETraversalAction::None;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector WallHitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector WallHitNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector LedgeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float ObstacleHeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float ObstacleDepth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasRoom = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> HitActor;

	FORCEINLINE bool bIsValid() const { return Action != ETraversalAction::None; }
};

USTRUCT(BlueprintType)
struct FTraversalTraceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float MaxForwardDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float MaxObstacleHeight = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float MinObstacleHeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TopTraceStartHeight = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float DepthCheckDistance = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float VaultMaxDepth = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float LowObstacleMaxHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ClimbOverMinHeight = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float HighObstacleMaxHeight = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float CapsuleCheckRadius = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float CapsuleCheckHalfHeight = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ForwardTraceRadius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ClimbOverLandingOffset = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float MantleHighZOffset = 75.f;
};
