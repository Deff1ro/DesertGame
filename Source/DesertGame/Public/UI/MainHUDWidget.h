// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class UAttributeComponent;
class UInventoryComponent;
class UInventoryCellWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertHUD, Log, All);

UCLASS()
class DESERTGAME_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateHealthBar(float Percent);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateStaminaBar(float Percent);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateHungerBar(float Percent);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void UpdateThirstBar(float Percent);

	UPROPERTY(BlueprintReadWrite, Category = "HUD")
	TArray<TObjectPtr<UInventoryCellWidget>> HotbarCells;

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void GatherHotbarCells();

private:
	UPROPERTY()
	TObjectPtr<UAttributeComponent> CachedAttributes;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventory;

	UFUNCTION()
	void HandleHealthChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleStaminaChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleHungerChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleThirstChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleHotbarChangedHUD(int32 SlotIndex);

	UFUNCTION()
	void HandleHotbarSelectionChanged(int32 OldIndex, int32 NewIndex);

	void TryBindToPlayer();
	void UnbindFromPlayer();

	void TryBindHotbar();
	void UnbindHotbar();
	void RefreshHotbar();
	void ApplyHotbarSelection(int32 Index);
};
