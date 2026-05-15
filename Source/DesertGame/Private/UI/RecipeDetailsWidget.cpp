// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/RecipeDetailsWidget.h"
#include "Inventory/RecipeDataAsset.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"

void URecipeDetailsWidget::DisplayRecipe(URecipeDataAsset* Recipe)
{
	if (!Recipe || !IngredientsList)
	{
		return;
	}

	IngredientsList->ClearChildren();

	if (TitleText)
	{
		TitleText->SetText(Recipe->DisplayName);
	}

	// Try to get inventory for "have" counts
	UInventoryComponent* Inv = nullptr;
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn))
		{
			Inv = Player->GetInventoryComponent();
		}
	}

	for (const FRecipeIngredient& Ing : Recipe->Ingredients)
	{
		if (!Ing.Item)
		{
			continue;
		}

		const int32 Have = Inv ? Inv->GetItemTotalCount(Ing.Item) : 0;
		const bool bEnough = Have >= Ing.Quantity;

		UTextBlock* Line = NewObject<UTextBlock>(this);
		const FString LineStr = FString::Printf(TEXT("%s: %d / %d"),
			*Ing.Item->DisplayName.ToString(), Have, Ing.Quantity);
		Line->SetText(FText::FromString(LineStr));
		Line->SetColorAndOpacity(FSlateColor(bEnough ? AvailableColor : MissingColor));

		IngredientsList->AddChild(Line);
	}
}
