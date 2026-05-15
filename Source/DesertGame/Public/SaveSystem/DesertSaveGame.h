// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProtagonistCharacter/ProtagonistCharacterTypes.h"
#include "DesertSaveGame.generated.h"

class UItemDataAsset;

USTRUCT()
struct FSavedInventorySlot
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY()
	int32 Quantity = 0;
};

UCLASS()
class DESERTGAME_API UDesertSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString LevelName;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY()
	EStanceState StanceState = EStanceState::Standing;

	UPROPERTY()
	FDateTime SaveTimestamp;

	// Inventory snapshot (16 main slots)
	UPROPERTY()
	TArray<FSavedInventorySlot> InventorySlots;

	// Hotbar snapshot (4 slots)
	UPROPERTY()
	TArray<FSavedInventorySlot> HotbarSlots;

	// FNames of AItemActor instances that were already picked up
	UPROPERTY()
	TArray<FName> PickedUpItemIds;

	static const FString SaveSlotName;
	static const int32 UserIndex;
};
