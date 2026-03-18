// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProtagonistCharacterTypes.generated.h"

UENUM(BlueprintType)
enum class EGaitState : uint8
{
	Walk	UMETA(DisplayName = "Walk"),
	Run		UMETA(DisplayName = "Run"),
	Sprint	UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Grounded	UMETA(DisplayName = "Grounded"),
	Falling		UMETA(DisplayName = "Falling"),
	Jumping		UMETA(DisplayName = "Jumping")
};

UENUM(BlueprintType)
enum class EStanceState : uint8
{
	Standing	UMETA(DisplayName = "Standing"),
	Crouching	UMETA(DisplayName = "Crouching")
};
