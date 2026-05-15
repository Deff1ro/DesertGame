// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MainHUDWidget.h"
#include "UI/InventoryCellWidget.h"
#include "Attributes/AttributeComponent.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"

DEFINE_LOG_CATEGORY(LogDesertHUD);

void UMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindToPlayer();
	TryBindHotbar();
}

void UMainHUDWidget::NativeDestruct()
{
	UnbindHotbar();
	UnbindFromPlayer();
	Super::NativeDestruct();
}

void UMainHUDWidget::TryBindToPlayer()
{
	APawn* Pawn = GetOwningPlayerPawn();
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn);
	if (!Player)
	{
		UE_LOG(LogDesertHUD, Warning, TEXT("MainHUDWidget: owning pawn is not AProtagonistCharacter — HUD will not bind"));
		return;
	}

	UAttributeComponent* Attr = Player->GetAttributeComponent();
	if (!Attr)
	{
		UE_LOG(LogDesertHUD, Warning, TEXT("MainHUDWidget: AProtagonistCharacter has no AttributeComponent"));
		return;
	}

	CachedAttributes = Attr;

	Attr->OnHealthChanged.AddDynamic(this, &UMainHUDWidget::HandleHealthChanged);
	Attr->OnStaminaChanged.AddDynamic(this, &UMainHUDWidget::HandleStaminaChanged);
	Attr->OnHungerChanged.AddDynamic(this, &UMainHUDWidget::HandleHungerChanged);
	Attr->OnThirstChanged.AddDynamic(this, &UMainHUDWidget::HandleThirstChanged);

	// Initial paint with current values
	HandleHealthChanged(Attr->GetHealth(), Attr->GetMaxHealth());
	HandleStaminaChanged(Attr->GetStamina(), Attr->GetMaxStamina());
	HandleHungerChanged(Attr->GetHunger(), Attr->GetMaxHunger());
	HandleThirstChanged(Attr->GetThirst(), Attr->GetMaxThirst());

	UE_LOG(LogDesertHUD, Log, TEXT("MainHUDWidget bound to player attributes"));
}

void UMainHUDWidget::UnbindFromPlayer()
{
	if (!CachedAttributes)
	{
		return;
	}

	CachedAttributes->OnHealthChanged.RemoveDynamic(this, &UMainHUDWidget::HandleHealthChanged);
	CachedAttributes->OnStaminaChanged.RemoveDynamic(this, &UMainHUDWidget::HandleStaminaChanged);
	CachedAttributes->OnHungerChanged.RemoveDynamic(this, &UMainHUDWidget::HandleHungerChanged);
	CachedAttributes->OnThirstChanged.RemoveDynamic(this, &UMainHUDWidget::HandleThirstChanged);

	CachedAttributes = nullptr;

	UE_LOG(LogDesertHUD, Log, TEXT("MainHUDWidget unbound from player attributes"));
}

void UMainHUDWidget::HandleHealthChanged(float NewValue, float MaxValue)
{
	const float Percent = (MaxValue > 0.0f) ? (NewValue / MaxValue) : 0.0f;
	UpdateHealthBar(Percent);
}

void UMainHUDWidget::HandleStaminaChanged(float NewValue, float MaxValue)
{
	const float Percent = (MaxValue > 0.0f) ? (NewValue / MaxValue) : 0.0f;
	UpdateStaminaBar(Percent);
}

void UMainHUDWidget::HandleHungerChanged(float NewValue, float MaxValue)
{
	const float Percent = (MaxValue > 0.0f) ? (NewValue / MaxValue) : 0.0f;
	UpdateHungerBar(Percent);
}

void UMainHUDWidget::HandleThirstChanged(float NewValue, float MaxValue)
{
	const float Percent = (MaxValue > 0.0f) ? (NewValue / MaxValue) : 0.0f;
	UpdateThirstBar(Percent);
}

// ============================================================
// Hotbar
// ============================================================

void UMainHUDWidget::TryBindHotbar()
{
	GatherHotbarCells();

	APawn* Pawn = GetOwningPlayerPawn();
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn);
	if (!Player)
	{
		return;
	}

	UInventoryComponent* Inv = Player->GetInventoryComponent();
	if (!Inv)
	{
		return;
	}

	CachedInventory = Inv;

	for (int32 i = 0; i < HotbarCells.Num(); ++i)
	{
		if (UInventoryCellWidget* Cell = HotbarCells[i])
		{
			Cell->ConfigureSlotReference(ESlotContainer::Hotbar, i);
			Cell->bIsDraggable = false;
			Cell->bAcceptsDrop = false;
		}
	}

	Inv->OnHotbarChanged.AddDynamic(this, &UMainHUDWidget::HandleHotbarChangedHUD);
	Inv->OnHotbarSelectionChanged.AddDynamic(this, &UMainHUDWidget::HandleHotbarSelectionChanged);

	RefreshHotbar();
	ApplyHotbarSelection(Inv->GetSelectedHotbarIndex());
}

void UMainHUDWidget::UnbindHotbar()
{
	if (!CachedInventory)
	{
		return;
	}

	CachedInventory->OnHotbarChanged.RemoveDynamic(this, &UMainHUDWidget::HandleHotbarChangedHUD);
	CachedInventory->OnHotbarSelectionChanged.RemoveDynamic(this, &UMainHUDWidget::HandleHotbarSelectionChanged);
	CachedInventory = nullptr;
}

void UMainHUDWidget::HandleHotbarSelectionChanged(int32 OldIndex, int32 NewIndex)
{
	if (HotbarCells.IsValidIndex(OldIndex) && HotbarCells[OldIndex])
	{
		HotbarCells[OldIndex]->SetSelected(false);
	}
	if (HotbarCells.IsValidIndex(NewIndex) && HotbarCells[NewIndex])
	{
		HotbarCells[NewIndex]->SetSelected(true);
	}
}

void UMainHUDWidget::ApplyHotbarSelection(int32 Index)
{
	for (int32 i = 0; i < HotbarCells.Num(); ++i)
	{
		if (HotbarCells[i])
		{
			HotbarCells[i]->SetSelected(i == Index);
		}
	}
}

void UMainHUDWidget::HandleHotbarChangedHUD(int32 SlotIndex)
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

void UMainHUDWidget::RefreshHotbar()
{
	if (!CachedInventory)
	{
		return;
	}

	const TArray<FInventorySlot>& Slots = CachedInventory->GetHotbarSlots();
	for (int32 i = 0; i < HotbarCells.Num() && i < Slots.Num(); ++i)
	{
		if (HotbarCells[i])
		{
			HotbarCells[i]->SetSlotData(Slots[i]);
		}
	}
}
