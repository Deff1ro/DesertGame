// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventoryHubWidget.h"
#include "UI/BagWidget.h"
#include "UI/CraftingWidget.h"
#include "Components/WidgetSwitcher.h"

void UInventoryHubWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BagPage)
	{
		BagPage->SetOwnerHub(this);
	}

	if (CraftingPage)
	{
		CraftingPage->SetOwnerHub(this);
	}

	SwitchToTab(EHubTab::Inventory);
}

void UInventoryHubWidget::SwitchToTab(EHubTab Tab)
{
	CurrentTab = Tab;

	if (!TabSwitcher)
	{
		return;
	}

	const int32 Index = (Tab == EHubTab::Inventory) ? 0 : 1;
	TabSwitcher->SetActiveWidgetIndex(Index);
}
