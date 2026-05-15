// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "ResourceNodeData.generated.h"

class UStaticMesh;
class UItemDataAsset;

UCLASS(BlueprintType)
class DESERTGAME_API UResourceNodeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Tool")
	EToolType RequiredTool = EToolType::Pickaxe;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Health", meta = (ClampMin = "1"))
	int32 MaxHealth = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Drop")
	TObjectPtr<UItemDataAsset> DropItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Drop", meta = (ClampMin = "1"))
	int32 DropMin = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Drop", meta = (ClampMin = "1"))
	int32 DropMax = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Drop", meta = (ClampMin = "0.0"))
	float DropScatterRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ResourceNode|Drop")
	float DropSpawnHeight = 50.0f;
};
