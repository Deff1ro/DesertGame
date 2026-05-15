// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/RecipeTypes.h"
#include "RecipeDataAsset.generated.h"

class UItemDataAsset;
class UTexture2D;

UCLASS(BlueprintType)
class DESERTGAME_API URecipeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TObjectPtr<UTexture2D> RecipeIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe|Result")
	TObjectPtr<UItemDataAsset> ResultItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe|Result", meta = (ClampMin = "1"))
	int32 ResultQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe|Ingredients")
	TArray<FRecipeIngredient> Ingredients;
};
