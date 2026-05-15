// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RecipeDetailsWidget.generated.h"

class URecipeDataAsset;
class UVerticalBox;
class UTextBlock;

UCLASS()
class DESERTGAME_API URecipeDetailsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RecipeDetails")
	void DisplayRecipe(URecipeDataAsset* Recipe);

protected:
	// BindWidget — must match names in WBP_RecipeDetails exactly
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> IngredientsList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(EditDefaultsOnly, Category = "RecipeDetails")
	FLinearColor AvailableColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "RecipeDetails")
	FLinearColor MissingColor = FLinearColor(1.f, 0.2f, 0.2f, 1.f);
};
