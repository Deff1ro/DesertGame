// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/RecipeDataAsset.h"

DEFINE_LOG_CATEGORY(LogDesertInventory);

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InventorySlots.SetNum(InventorySize);
	HotbarSlots.SetNum(HotbarSize);
}

void UInventoryComponent::SelectHotbarSlot(int32 NewIndex)
{
	if (NewIndex < 0 || NewIndex >= HotbarSlots.Num())
	{
		return;
	}

	const int32 OldIndex = SelectedHotbarIndex;

	// Pressing the same key again deselects (toggle-off)
	if (NewIndex == SelectedHotbarIndex)
	{
		SelectedHotbarIndex = INDEX_NONE;
		OnHotbarSelectionChanged.Broadcast(OldIndex, INDEX_NONE);
		OnSelectedHotbarChanged.Broadcast(INDEX_NONE);
		return;
	}

	SelectedHotbarIndex = NewIndex;
	OnHotbarSelectionChanged.Broadcast(OldIndex, NewIndex);
	OnSelectedHotbarChanged.Broadcast(NewIndex);
}

UItemDataAsset* UInventoryComponent::GetSelectedItem() const
{
	if (HotbarSlots.IsValidIndex(SelectedHotbarIndex))
	{
		return HotbarSlots[SelectedHotbarIndex].ItemData;
	}
	return nullptr;
}

bool UInventoryComponent::ConsumeSelectedItem()
{
	if (!HotbarSlots.IsValidIndex(SelectedHotbarIndex))
	{
		return false;
	}

	FInventorySlot& Slot = HotbarSlots[SelectedHotbarIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	Slot.Quantity -= 1;
	if (Slot.Quantity <= 0)
	{
		Slot.Clear();
	}

	// Reuse the standard slot-changed broadcast so HUD + equipped-mesh both refresh.
	FInventorySlotReference Ref;
	Ref.Container = ESlotContainer::Hotbar;
	Ref.Index = SelectedHotbarIndex;
	BroadcastSlotChanged(Ref);

	return true;
}

// ============================================================
// AddItem
// ============================================================

int32 UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
	if (!ItemData || Quantity <= 0)
	{
		return Quantity;
	}

	int32 Remaining = Quantity;
	const int32 MaxStack = ItemData->MaxStackSize;

	// Phase 1: top up existing stacks (Inventory first, then Hotbar)
	auto TopUpStacks = [&](TArray<FInventorySlot>& Slots, ESlotContainer Container)
	{
		for (int32 i = 0; i < Slots.Num() && Remaining > 0; ++i)
		{
			FInventorySlot& Slot = Slots[i];
			if (!Slot.IsEmpty() && Slot.ItemData == ItemData && Slot.Quantity < MaxStack)
			{
				const int32 CanAdd = MaxStack - Slot.Quantity;
				const int32 ToAdd = FMath::Min(CanAdd, Remaining);
				Slot.Quantity += ToAdd;
				Remaining -= ToAdd;

				FInventorySlotReference Ref;
				Ref.Container = Container;
				Ref.Index = i;
				BroadcastSlotChanged(Ref);
			}
		}
	};

	TopUpStacks(InventorySlots, ESlotContainer::Inventory);
	if (Remaining == 0)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("AddItem: %s x%d fully stacked"), *ItemData->GetName(), Quantity);
		return 0;
	}

	TopUpStacks(HotbarSlots, ESlotContainer::Hotbar);
	if (Remaining == 0)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("AddItem: %s x%d fully stacked"), *ItemData->GetName(), Quantity);
		return 0;
	}

	// Phase 2: fill empty slots (Inventory first, then Hotbar)
	auto FillEmpty = [&](TArray<FInventorySlot>& Slots, ESlotContainer Container)
	{
		for (int32 i = 0; i < Slots.Num() && Remaining > 0; ++i)
		{
			FInventorySlot& Slot = Slots[i];
			if (Slot.IsEmpty())
			{
				const int32 ToAdd = FMath::Min(MaxStack, Remaining);
				Slot.ItemData = ItemData;
				Slot.Quantity = ToAdd;
				Remaining -= ToAdd;

				FInventorySlotReference Ref;
				Ref.Container = Container;
				Ref.Index = i;
				BroadcastSlotChanged(Ref);
			}
		}
	};

	FillEmpty(InventorySlots, ESlotContainer::Inventory);
	if (Remaining == 0)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("AddItem: %s x%d placed into empty inventory slots"), *ItemData->GetName(), Quantity);
		return 0;
	}

	FillEmpty(HotbarSlots, ESlotContainer::Hotbar);

	if (Remaining > 0)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("AddItem: %s — %d unit(s) did not fit"), *ItemData->GetName(), Remaining);
	}
	else
	{
		UE_LOG(LogDesertInventory, Log, TEXT("AddItem: %s x%d fully placed"), *ItemData->GetName(), Quantity);
	}

	return Remaining;
}

// ============================================================
// MoveItem
// ============================================================

bool UInventoryComponent::MoveItem(const FInventorySlotReference& From, const FInventorySlotReference& To)
{
	if (!From.IsValid() || !To.IsValid())
	{
		return false;
	}
	if (From == To)
	{
		return false;
	}

	FInventorySlot* FromSlot = GetMutableSlot(From);
	FInventorySlot* ToSlot = GetMutableSlot(To);
	if (!FromSlot || !ToSlot)
	{
		return false;
	}

	if (FromSlot->IsEmpty())
	{
		return false;
	}

	// Cloth slot only accepts items in the Cloth category. Reject any move that
	// would put a non-cloth item into a cloth container, and reject swaps where
	// the item already in the cloth slot is non-cloth (shouldn't happen, but
	// defensive). Moves OUT of the cloth slot have no restriction.
	if (To.Container == ESlotContainer::Cloth && FromSlot->ItemData
		&& FromSlot->ItemData->Category != EItemCategory::Cloth)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("MoveItem rejected: cloth slot only accepts cloth items"));
		return false;
	}

	if (ToSlot->IsEmpty())
	{
		// Case A: target is empty — full move
		*ToSlot = *FromSlot;
		FromSlot->Clear();
	}
	else if (ToSlot->ItemData == FromSlot->ItemData)
	{
		// Case B: same item type — stack
		const int32 MaxStack = ToSlot->ItemData->MaxStackSize;
		const int32 CanAdd = MaxStack - ToSlot->Quantity;
		if (CanAdd <= 0)
		{
			// destination already full — fall back to swap
			Swap(*FromSlot, *ToSlot);
		}
		else
		{
			const int32 ToTransfer = FMath::Min(CanAdd, FromSlot->Quantity);
			ToSlot->Quantity += ToTransfer;
			FromSlot->Quantity -= ToTransfer;
			if (FromSlot->Quantity <= 0)
			{
				FromSlot->Clear();
			}
		}
	}
	else
	{
		// Case C: different items — swap
		Swap(*FromSlot, *ToSlot);
	}

	BroadcastSlotChanged(From);
	BroadcastSlotChanged(To);

	UE_LOG(LogDesertInventory, Log, TEXT("MoveItem: [%d:%d] -> [%d:%d]"),
		(int32)From.Container, From.Index, (int32)To.Container, To.Index);

	return true;
}

// ============================================================
// RemoveFromSlot
// ============================================================

bool UInventoryComponent::RemoveFromSlot(const FInventorySlotReference& SlotRef, int32 Amount)
{
	if (!SlotRef.IsValid() || Amount <= 0)
	{
		return false;
	}

	FInventorySlot* Slot = GetMutableSlot(SlotRef);
	if (!Slot || Slot->IsEmpty())
	{
		return false;
	}

	Slot->Quantity -= Amount;
	if (Slot->Quantity <= 0)
	{
		Slot->Clear();
	}

	BroadcastSlotChanged(SlotRef);
	return true;
}

// ============================================================
// Getters
// ============================================================

FInventorySlot UInventoryComponent::GetSlot(const FInventorySlotReference& SlotRef) const
{
	if (!SlotRef.IsValid())
	{
		return FInventorySlot();
	}

	if (SlotRef.Container == ESlotContainer::Cloth)
	{
		return ClothSlot;
	}

	const TArray<FInventorySlot>& Slots = (SlotRef.Container == ESlotContainer::Inventory) ? InventorySlots : HotbarSlots;
	if (!Slots.IsValidIndex(SlotRef.Index))
	{
		return FInventorySlot();
	}
	return Slots[SlotRef.Index];
}

// ============================================================
// Private helpers
// ============================================================

FInventorySlot* UInventoryComponent::GetMutableSlot(const FInventorySlotReference& Ref)
{
	if (!Ref.IsValid())
	{
		return nullptr;
	}

	if (Ref.Container == ESlotContainer::Cloth)
	{
		return &ClothSlot;
	}

	TArray<FInventorySlot>& Slots = (Ref.Container == ESlotContainer::Inventory) ? InventorySlots : HotbarSlots;
	if (!Slots.IsValidIndex(Ref.Index))
	{
		return nullptr;
	}
	return &Slots[Ref.Index];
}

void UInventoryComponent::BroadcastSlotChanged(const FInventorySlotReference& Ref)
{
	switch (Ref.Container)
	{
	case ESlotContainer::Inventory:
		OnInventoryChanged.Broadcast(Ref.Index);
		break;
	case ESlotContainer::Hotbar:
		OnHotbarChanged.Broadcast(Ref.Index);
		if (SelectedHotbarIndex != INDEX_NONE && Ref.Index == SelectedHotbarIndex)
		{
			OnSelectedHotbarChanged.Broadcast(SelectedHotbarIndex);
		}
		break;
	case ESlotContainer::Cloth:
		OnClothChanged.Broadcast();
		break;
	}
}

// ============================================================
// Crafting helpers
// ============================================================

int32 UInventoryComponent::GetItemTotalCount(UItemDataAsset* Item) const
{
	if (!Item)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.ItemData == Item)
		{
			Total += Slot.Quantity;
		}
	}
	for (const FInventorySlot& Slot : HotbarSlots)
	{
		if (Slot.ItemData == Item)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

bool UInventoryComponent::RemoveItems(UItemDataAsset* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return false;
	}

	if (GetItemTotalCount(Item) < Amount)
	{
		return false;
	}

	int32 Remaining = Amount;

	for (int32 i = 0; i < InventorySlots.Num() && Remaining > 0; ++i)
	{
		FInventorySlot& Slot = InventorySlots[i];
		if (Slot.ItemData != Item)
		{
			continue;
		}

		const int32 Take = FMath::Min(Slot.Quantity, Remaining);
		Slot.Quantity -= Take;
		Remaining -= Take;

		if (Slot.Quantity == 0)
		{
			Slot.Clear();
		}

		FInventorySlotReference Ref;
		Ref.Container = ESlotContainer::Inventory;
		Ref.Index = i;
		BroadcastSlotChanged(Ref);
	}

	for (int32 i = 0; i < HotbarSlots.Num() && Remaining > 0; ++i)
	{
		FInventorySlot& Slot = HotbarSlots[i];
		if (Slot.ItemData != Item)
		{
			continue;
		}

		const int32 Take = FMath::Min(Slot.Quantity, Remaining);
		Slot.Quantity -= Take;
		Remaining -= Take;

		if (Slot.Quantity == 0)
		{
			Slot.Clear();
		}

		FInventorySlotReference Ref;
		Ref.Container = ESlotContainer::Hotbar;
		Ref.Index = i;
		BroadcastSlotChanged(Ref);
	}

	return true;
}

bool UInventoryComponent::CanCraft(URecipeDataAsset* Recipe) const
{
	if (!Recipe || !Recipe->ResultItem)
	{
		return false;
	}

	for (const FRecipeIngredient& Ing : Recipe->Ingredients)
	{
		if (!Ing.Item)
		{
			return false;
		}
		if (GetItemTotalCount(Ing.Item) < Ing.Quantity)
		{
			return false;
		}
	}
	return true;
}

void UInventoryComponent::ApplyLoadedSlots(const TArray<FSavedInventorySlot>& SavedInventory, const TArray<FSavedInventorySlot>& SavedHotbar)
{
	auto Apply = [](TArray<FInventorySlot>& Target, const TArray<FSavedInventorySlot>& Source)
	{
		for (int32 i = 0; i < Target.Num(); ++i)
		{
			Target[i].Clear();
			if (i < Source.Num())
			{
				const FSavedInventorySlot& Saved = Source[i];
				if (Saved.Quantity > 0)
				{
					if (UItemDataAsset* Loaded = Saved.ItemData.LoadSynchronous())
					{
						Target[i].ItemData = Loaded;
						Target[i].Quantity = Saved.Quantity;
					}
				}
			}
		}
	};

	Apply(InventorySlots, SavedInventory);
	Apply(HotbarSlots, SavedHotbar);

	// Notify subscribers about every slot so the UI repaints
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		OnInventoryChanged.Broadcast(i);
	}
	for (int32 i = 0; i < HotbarSlots.Num(); ++i)
	{
		OnHotbarChanged.Broadcast(i);
	}
}

bool UInventoryComponent::TryCraft(URecipeDataAsset* Recipe)
{
	if (!CanCraft(Recipe))
	{
		UE_LOG(LogDesertInventory, Log, TEXT("TryCraft: cannot craft '%s' — requirements not met"),
			Recipe ? *Recipe->GetName() : TEXT("null"));
		return false;
	}

	for (const FRecipeIngredient& Ing : Recipe->Ingredients)
	{
		if (!RemoveItems(Ing.Item, Ing.Quantity))
		{
			UE_LOG(LogDesertInventory, Error, TEXT("TryCraft: unexpected failure removing %s x%d"),
				*Ing.Item->GetName(), Ing.Quantity);
			return false;
		}
	}

	const int32 Remaining = AddItem(Recipe->ResultItem, Recipe->ResultQuantity);
	if (Remaining > 0)
	{
		UE_LOG(LogDesertInventory, Warning, TEXT("TryCraft: '%s' crafted but %d units didn't fit in inventory"),
			*Recipe->GetName(), Remaining);
	}
	else
	{
		UE_LOG(LogDesertInventory, Log, TEXT("TryCraft: '%s' crafted successfully"), *Recipe->GetName());
	}

	return true;
}
