// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/RecipeCellWidget.h"
#include "UI/CraftingWidget.h"
#include "Inventory/RecipeDataAsset.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"

void URecipeCellWidget::SetRecipe(URecipeDataAsset* InRecipe)
{
	Recipe = InRecipe;
	if (!Recipe)
	{
		OnRecipeCleared();
		return;
	}
	RefreshAvailability();
}

void URecipeCellWidget::SetOwnerCraftingWidget(UCraftingWidget* InOwner)
{
	OwnerCraftingWidget = InOwner;
}

void URecipeCellWidget::RefreshAvailability()
{
	if (!Recipe)
	{
		OnRecipeCleared();
		return;
	}

	bool bAvailable = false;
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn))
		{
			if (UInventoryComponent* Inv = Player->GetInventoryComponent())
			{
				bAvailable = Inv->CanCraft(Recipe);
			}
		}
	}

	OnRecipeUpdated(Recipe->RecipeIcon, bAvailable);
}

FReply URecipeCellWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Recipe)
	{
		if (APawn* Pawn = GetOwningPlayerPawn())
		{
			if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn))
			{
				if (UInventoryComponent* Inv = Player->GetInventoryComponent())
				{
					Inv->TryCraft(Recipe);
				}
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void URecipeCellWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (Recipe && OwnerCraftingWidget)
	{
		OwnerCraftingWidget->ShowRecipeDetails(Recipe, this);
	}
}

void URecipeCellWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (OwnerCraftingWidget)
	{
		OwnerCraftingWidget->HideRecipeDetails();
	}
}
