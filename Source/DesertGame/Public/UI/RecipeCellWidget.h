// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RecipeCellWidget.generated.h"

class URecipeDataAsset;
class UCraftingWidget;

UCLASS()
class DESERTGAME_API URecipeCellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RecipeCell")
	void SetRecipe(URecipeDataAsset* InRecipe);

	UFUNCTION(BlueprintCallable, Category = "RecipeCell")
	void SetOwnerCraftingWidget(UCraftingWidget* InOwner);

	UFUNCTION(BlueprintPure, Category = "RecipeCell")
	URecipeDataAsset* GetRecipe() const { return Recipe; }

	UFUNCTION(BlueprintCallable, Category = "RecipeCell")
	void RefreshAvailability();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "RecipeCell")
	void OnRecipeUpdated(UTexture2D* Icon, bool bIsAvailable);

	UFUNCTION(BlueprintImplementableEvent, Category = "RecipeCell")
	void OnRecipeCleared();

	UPROPERTY(BlueprintReadOnly, Category = "RecipeCell")
	TObjectPtr<URecipeDataAsset> Recipe;

	UPROPERTY(BlueprintReadOnly, Category = "RecipeCell")
	TObjectPtr<UCraftingWidget> OwnerCraftingWidget;
};
