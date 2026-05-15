// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BagWidget.generated.h"

class UInventoryCellWidget;
class UInventoryComponent;
class UInventoryHubWidget;

UCLASS()
class DESERTGAME_API UBagWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Bag")
	void SetOwnerHub(UInventoryHubWidget* InHub);

	UFUNCTION(BlueprintCallable, Category = "Bag")
	void OnSwitchToCraftingClicked();

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Bag")
	TArray<TObjectPtr<UInventoryCellWidget>> InventoryCells;

	UPROPERTY(BlueprintReadWrite, Category = "Bag")
	TArray<TObjectPtr<UInventoryCellWidget>> HotbarCells;

	UFUNCTION(BlueprintImplementableEvent, Category = "Bag")
	void GatherCells();

private:
	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventory;

	UPROPERTY()
	TObjectPtr<UInventoryHubWidget> OwnerHub;

	UFUNCTION()
	void HandleInventoryChanged(int32 SlotIndex);

	UFUNCTION()
	void HandleHotbarChanged(int32 SlotIndex);

	void RefreshAllCells();
	void TryBindToPlayer();
	void UnbindFromPlayer();
};
