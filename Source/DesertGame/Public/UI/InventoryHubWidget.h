// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryHubWidget.generated.h"

class UBagWidget;
class UCraftingWidget;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class EHubTab : uint8
{
	Inventory UMETA(DisplayName = "Inventory"),
	Crafting  UMETA(DisplayName = "Crafting")
};

UCLASS()
class DESERTGAME_API UInventoryHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Hub")
	void SwitchToTab(EHubTab Tab);

	UFUNCTION(BlueprintPure, Category = "Hub")
	EHubTab GetCurrentTab() const { return CurrentTab; }

protected:
	// Must match widget names in WBP_InventoryHub exactly
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBagWidget> BagPage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCraftingWidget> CraftingPage;

private:
	EHubTab CurrentTab = EHubTab::Inventory;
};
