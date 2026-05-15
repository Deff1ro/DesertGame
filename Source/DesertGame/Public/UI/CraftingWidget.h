// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingWidget.generated.h"

class URecipeDataAsset;
class URecipeCellWidget;
class URecipeDetailsWidget;
class UInventoryComponent;
class UInventoryHubWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertCrafting, Log, All);

UCLASS()
class DESERTGAME_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void ShowRecipeDetails(URecipeDataAsset* Recipe, UUserWidget* AnchorCell);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void HideRecipeDetails();

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetOwnerHub(UInventoryHubWidget* InHub);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void OnSwitchToInventoryClicked();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TArray<TObjectPtr<URecipeDataAsset>> AvailableRecipes;

	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<URecipeDetailsWidget> RecipeDetailsWidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "Crafting")
	TArray<TObjectPtr<URecipeCellWidget>> RecipeCells;

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void GatherRecipeCells();

private:
	UPROPERTY()
	TObjectPtr<UInventoryHubWidget> OwnerHub;

	UPROPERTY()
	TObjectPtr<URecipeDetailsWidget> ActiveDetailsWidget;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventory;

	UPROPERTY()
	TObjectPtr<URecipeDataAsset> CurrentDetailsRecipe;

	UFUNCTION()
	void HandleInventoryChanged(int32 SlotIndex);

	UFUNCTION()
	void HandleHotbarChanged(int32 SlotIndex);

	void RefreshAllRecipeCells();
	void TryBindToPlayer();
	void UnbindFromPlayer();
};
