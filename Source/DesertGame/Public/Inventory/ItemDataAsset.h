// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "ItemDataAsset.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;

UCLASS(BlueprintType)
class DESERTGAME_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemType ItemType = EItemType::None;

	// Broad category used for slot restrictions. Set to Cloth on cloak/armor items
	// so they can be placed in cloth-only slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::Generic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxStackSize = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool")
	EToolType ToolType = EToolType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool")
	TObjectPtr<UStaticMesh> EquipMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Tool", meta = (ClampMin = "1"))
	int32 ToolDamage = 1;

	// ============================================================
	// Consumable
	// ============================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable")
	bool bIsConsumable = false;

	// Hunger restored on use. Set 0 if this consumable doesn't affect hunger.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable",
		meta = (ClampMin = "0.0", EditCondition = "bIsConsumable"))
	float HungerRestore = 0.0f;

	// Thirst restored on use. Set 0 if this consumable doesn't affect thirst.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable",
		meta = (ClampMin = "0.0", EditCondition = "bIsConsumable"))
	float ThirstRestore = 0.0f;

	// Optional health restore (e.g. healing food). 0 = none.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable",
		meta = (ClampMin = "0.0", EditCondition = "bIsConsumable"))
	float HealthRestore = 0.0f;

	// ============================================================
	// Cloth equipment (only used when Category == Cloth)
	// ============================================================

	// How this piece of cloth is applied to the character when equipped.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Cloth")
	EClothEquipMode ClothEquipMode = EClothEquipMode::Overlay;

	// Skeletal mesh used to visually represent the cloth on the character.
	// Must use the same skeleton as the body mesh (Leader Pose).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Cloth")
	TObjectPtr<USkeletalMesh> ClothSkeletalMesh;

	// What environmental hazard this garment protects from. None means it's
	// cosmetic and provides no protection.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Cloth")
	EClothProtection Protection = EClothProtection::None;
};
