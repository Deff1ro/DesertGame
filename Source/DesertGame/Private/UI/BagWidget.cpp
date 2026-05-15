// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/BagWidget.h"
#include "UI/InventoryCellWidget.h"
#include "UI/InventoryHubWidget.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"

void UBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GatherCells();

	for (int32 i = 0; i < InventoryCells.Num(); ++i)
	{
		if (InventoryCells[i])
		{
			InventoryCells[i]->ConfigureSlotReference(ESlotContainer::Inventory, i);
		}
	}

	for (int32 i = 0; i < HotbarCells.Num(); ++i)
	{
		if (HotbarCells[i])
		{
			HotbarCells[i]->ConfigureSlotReference(ESlotContainer::Hotbar, i);
		}
	}

	TryBindToPlayer();
}

void UBagWidget::NativeDestruct()
{
	UnbindFromPlayer();
	Super::NativeDestruct();
}

void UBagWidget::TryBindToPlayer()
{
	APawn* Pawn = GetOwningPlayerPawn();
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn);
	if (!Player)
	{
		UE_LOG(LogDesertInventory, Warning, TEXT("BagWidget: owning pawn is not AProtagonistCharacter"));
		return;
	}

	UInventoryComponent* Inv = Player->GetInventoryComponent();
	if (!Inv)
	{
		UE_LOG(LogDesertInventory, Warning, TEXT("BagWidget: player has no InventoryComponent"));
		return;
	}

	CachedInventory = Inv;

	Inv->OnInventoryChanged.AddDynamic(this, &UBagWidget::HandleInventoryChanged);
	Inv->OnHotbarChanged.AddDynamic(this, &UBagWidget::HandleHotbarChanged);

	RefreshAllCells();
}

void UBagWidget::UnbindFromPlayer()
{
	if (!CachedInventory)
	{
		return;
	}

	CachedInventory->OnInventoryChanged.RemoveDynamic(this, &UBagWidget::HandleInventoryChanged);
	CachedInventory->OnHotbarChanged.RemoveDynamic(this, &UBagWidget::HandleHotbarChanged);
	CachedInventory = nullptr;
}

void UBagWidget::HandleInventoryChanged(int32 SlotIndex)
{
	if (!CachedInventory || !InventoryCells.IsValidIndex(SlotIndex) || !InventoryCells[SlotIndex])
	{
		return;
	}

	const TArray<FInventorySlot>& Slots = CachedInventory->GetInventorySlots();
	if (Slots.IsValidIndex(SlotIndex))
	{
		InventoryCells[SlotIndex]->SetSlotData(Slots[SlotIndex]);
	}
}

void UBagWidget::HandleHotbarChanged(int32 SlotIndex)
{
	if (!CachedInventory || !HotbarCells.IsValidIndex(SlotIndex) || !HotbarCells[SlotIndex])
	{
		return;
	}

	const TArray<FInventorySlot>& Slots = CachedInventory->GetHotbarSlots();
	if (Slots.IsValidIndex(SlotIndex))
	{
		HotbarCells[SlotIndex]->SetSlotData(Slots[SlotIndex]);
	}
}

void UBagWidget::RefreshAllCells()
{
	if (!CachedInventory)
	{
		return;
	}

	const TArray<FInventorySlot>& InvSlots = CachedInventory->GetInventorySlots();
	for (int32 i = 0; i < InventoryCells.Num() && i < InvSlots.Num(); ++i)
	{
		if (InventoryCells[i])
		{
			InventoryCells[i]->SetSlotData(InvSlots[i]);
		}
	}

	const TArray<FInventorySlot>& HotSlots = CachedInventory->GetHotbarSlots();
	for (int32 i = 0; i < HotbarCells.Num() && i < HotSlots.Num(); ++i)
	{
		if (HotbarCells[i])
		{
			HotbarCells[i]->SetSlotData(HotSlots[i]);
		}
	}
}

void UBagWidget::SetOwnerHub(UInventoryHubWidget* InHub)
{
	OwnerHub = InHub;
}

void UBagWidget::OnSwitchToCraftingClicked()
{
	if (OwnerHub)
	{
		OwnerHub->SwitchToTab(EHubTab::Crafting);
	}
}
