// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CraftingWidget.h"
#include "UI/RecipeCellWidget.h"
#include "UI/RecipeDetailsWidget.h"
#include "UI/InventoryHubWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/RecipeDataAsset.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"

DEFINE_LOG_CATEGORY(LogDesertCrafting);

void UCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GatherRecipeCells();

	for (int32 i = 0; i < RecipeCells.Num(); ++i)
	{
		URecipeCellWidget* Cell = RecipeCells[i];
		if (!Cell)
		{
			continue;
		}

		Cell->SetOwnerCraftingWidget(this);

		if (i < AvailableRecipes.Num())
		{
			Cell->SetRecipe(AvailableRecipes[i]);
		}
		else
		{
			Cell->SetRecipe(nullptr);
		}
	}

	TryBindToPlayer();
}

void UCraftingWidget::NativeDestruct()
{
	HideRecipeDetails();
	UnbindFromPlayer();
	Super::NativeDestruct();
}

void UCraftingWidget::TryBindToPlayer()
{
	APawn* Pawn = GetOwningPlayerPawn();
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn);
	if (!Player)
	{
		UE_LOG(LogDesertCrafting, Warning, TEXT("CraftingWidget: owning pawn is not AProtagonistCharacter"));
		return;
	}

	UInventoryComponent* Inv = Player->GetInventoryComponent();
	if (!Inv)
	{
		UE_LOG(LogDesertCrafting, Warning, TEXT("CraftingWidget: player has no InventoryComponent"));
		return;
	}

	CachedInventory = Inv;
	Inv->OnInventoryChanged.AddDynamic(this, &UCraftingWidget::HandleInventoryChanged);
	Inv->OnHotbarChanged.AddDynamic(this, &UCraftingWidget::HandleHotbarChanged);

	UE_LOG(LogDesertCrafting, Log, TEXT("CraftingWidget bound to player inventory"));
}

void UCraftingWidget::UnbindFromPlayer()
{
	if (!CachedInventory)
	{
		return;
	}

	CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UCraftingWidget::HandleInventoryChanged);
	CachedInventory->OnHotbarChanged.RemoveDynamic(this, &UCraftingWidget::HandleHotbarChanged);
	CachedInventory = nullptr;

	UE_LOG(LogDesertCrafting, Log, TEXT("CraftingWidget unbound from player inventory"));
}

void UCraftingWidget::HandleInventoryChanged(int32 SlotIndex)
{
	RefreshAllRecipeCells();
}

void UCraftingWidget::HandleHotbarChanged(int32 SlotIndex)
{
	RefreshAllRecipeCells();
}

void UCraftingWidget::RefreshAllRecipeCells()
{
	for (URecipeCellWidget* Cell : RecipeCells)
	{
		if (Cell)
		{
			Cell->RefreshAvailability();
		}
	}

	if (ActiveDetailsWidget && CurrentDetailsRecipe)
	{
		ActiveDetailsWidget->DisplayRecipe(CurrentDetailsRecipe);
	}
}

void UCraftingWidget::ShowRecipeDetails(URecipeDataAsset* Recipe, UUserWidget* AnchorCell)
{
	if (!Recipe || !RecipeDetailsWidgetClass)
	{
		return;
	}

	if (!ActiveDetailsWidget)
	{
		ActiveDetailsWidget = CreateWidget<URecipeDetailsWidget>(GetOwningPlayer(), RecipeDetailsWidgetClass);
	}

	if (!ActiveDetailsWidget)
	{
		return;
	}

	ActiveDetailsWidget->DisplayRecipe(Recipe);
	CurrentDetailsRecipe = Recipe;

	if (!ActiveDetailsWidget->IsInViewport())
	{
		ActiveDetailsWidget->AddToViewport(100);
	}

	// Position details widget to the right of the anchor cell
	if (AnchorCell)
	{
		const FGeometry& Geo = AnchorCell->GetCachedGeometry();
		const FVector2D AbsPos = Geo.GetAbsolutePosition();
		const FVector2D AbsSize = Geo.GetAbsoluteSize();

		FVector2D PixelPos;
		FVector2D ViewportPos;
		USlateBlueprintLibrary::AbsoluteToViewport(this, AbsPos + FVector2D(AbsSize.X + 4.f, 0.f), PixelPos, ViewportPos);
		ActiveDetailsWidget->SetPositionInViewport(ViewportPos, false);
	}

	UE_LOG(LogDesertCrafting, Log, TEXT("ShowRecipeDetails: '%s'"), *Recipe->GetName());
}

void UCraftingWidget::HideRecipeDetails()
{
	if (ActiveDetailsWidget && ActiveDetailsWidget->IsInViewport())
	{
		ActiveDetailsWidget->RemoveFromParent();
	}
	CurrentDetailsRecipe = nullptr;
}

void UCraftingWidget::SetOwnerHub(UInventoryHubWidget* InHub)
{
	OwnerHub = InHub;
}

void UCraftingWidget::OnSwitchToInventoryClicked()
{
	if (OwnerHub)
	{
		OwnerHub->SwitchToTab(EHubTab::Inventory);
		UE_LOG(LogDesertCrafting, Log, TEXT("CraftingWidget: switching to Inventory tab"));
	}
}
