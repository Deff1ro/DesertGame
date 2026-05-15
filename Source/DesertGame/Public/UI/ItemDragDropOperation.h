// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/InventoryTypes.h"
#include "ItemDragDropOperation.generated.h"

class UItemDataAsset;

UCLASS()
class DESERTGAME_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	FInventorySlotReference SourceSlot;

	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	TObjectPtr<UItemDataAsset> DraggedItemData;

	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	int32 DraggedQuantity = 0;
};
