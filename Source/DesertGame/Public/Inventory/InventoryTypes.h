// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UItemDataAsset;

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None     UMETA(DisplayName = "None"),
	Wood     UMETA(DisplayName = "Wood"),
	Metal    UMETA(DisplayName = "Metal"),
	Stone    UMETA(DisplayName = "Stone")
};

UENUM(BlueprintType)
enum class EToolType : uint8
{
	None     UMETA(DisplayName = "None"),
	Sword    UMETA(DisplayName = "Sword"),
	Axe      UMETA(DisplayName = "Axe"),
	Pickaxe  UMETA(DisplayName = "Pickaxe")
};

// Broad equipment category — used by inventory slots to restrict what can be
// dropped into them (e.g. a "cloth slot" only accepts EItemCategory::Cloth).
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	Cloth   UMETA(DisplayName = "Cloth")
};

// How a piece of cloth equipment is applied to the character mesh.
UENUM(BlueprintType)
enum class EClothEquipMode : uint8
{
	// Worn on top of the body. Goes into CloakMesh, body stays as-is.
	Overlay  UMETA(DisplayName = "Overlay (Cloak)"),
	// Replaces the body mesh entirely while equipped (e.g. full outfit / jacket).
	BodySwap UMETA(DisplayName = "Body Swap (Jacket)")
};

// What kind of environmental damage a piece of cloth protects against.
UENUM(BlueprintType)
enum class EClothProtection : uint8
{
	None UMETA(DisplayName = "None"),
	Heat UMETA(DisplayName = "Heat (Day)"),   // Cloak / sun protection
	Cold UMETA(DisplayName = "Cold (Night)")  // Jacket / warm clothes
};

UENUM(BlueprintType)
enum class ESlotContainer : uint8
{
	Inventory UMETA(DisplayName = "Main Inventory"),
	Hotbar    UMETA(DisplayName = "Hotbar"),
	Cloth     UMETA(DisplayName = "Cloth")
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemDataAsset> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;

	bool IsEmpty() const { return ItemData == nullptr || Quantity <= 0; }
	void Clear() { ItemData = nullptr; Quantity = 0; }
};

// One entry in an enemy loot table: a specific item that drops in a random quantity.
USTRUCT(BlueprintType)
struct FEnemyLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemDataAsset> Item = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 DropMin = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 DropMax = 1;
};

USTRUCT(BlueprintType)
struct FInventorySlotReference
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	ESlotContainer Container = ESlotContainer::Inventory;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 Index = INDEX_NONE;

	bool IsValid() const { return Index >= 0; }
	bool operator==(const FInventorySlotReference& Other) const
	{
		return Container == Other.Container && Index == Other.Index;
	}
};
