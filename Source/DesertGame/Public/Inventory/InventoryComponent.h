// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "SaveSystem/DesertSaveGame.h"
#include "InventoryComponent.generated.h"

class UItemDataAsset;
class URecipeDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertInventory, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHotbarChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHotbarSelectionChanged, int32, OldIndex, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedHotbarChanged, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClothChanged);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void BeginPlay() override;

	// ============================================================
	// Configuration
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InventorySize = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 HotbarSize = 4;

	// ============================================================
	// State
	// ============================================================

	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	TArray<FInventorySlot> InventorySlots;

	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	TArray<FInventorySlot> HotbarSlots;

	// Single equipment slot dedicated to cloth-category items (cloak/armor).
	UPROPERTY(VisibleInstanceOnly, Category = "Inventory")
	FInventorySlot ClothSlot;

	// ============================================================
	// Events
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnHotbarChanged OnHotbarChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnHotbarSelectionChanged OnHotbarSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnSelectedHotbarChanged OnSelectedHotbarChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnClothChanged OnClothChanged;

	// ============================================================
	// API
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDataAsset* ItemData, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(const FInventorySlotReference& From, const FInventorySlotReference& To);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveFromSlot(const FInventorySlotReference& SlotRef, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventorySlot GetSlot(const FInventorySlotReference& SlotRef) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventorySlot>& GetInventorySlots() const { return InventorySlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventorySlot>& GetHotbarSlots() const { return HotbarSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Cloth")
	const FInventorySlot& GetClothSlot() const { return ClothSlot; }

	// ============================================================
	// Crafting API
	// ============================================================

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemTotalCount(UItemDataAsset* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItems(UItemDataAsset* Item, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Inventory|Crafting")
	bool CanCraft(URecipeDataAsset* Recipe) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Crafting")
	bool TryCraft(URecipeDataAsset* Recipe);

	// Replace all slots from the save data. Broadcasts change events for every slot
	// so any UI subscribed at this point repaints. Pass arrays of equal length to
	// inventory/hotbar size; missing entries are treated as empty.
	void ApplyLoadedSlots(const TArray<FSavedInventorySlot>& SavedInventory, const TArray<FSavedInventorySlot>& SavedHotbar);

	// ============================================================
	// Hotbar selection
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectHotbarSlot(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory|Hotbar")
	int32 GetSelectedHotbarIndex() const { return SelectedHotbarIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Hotbar")
	UItemDataAsset* GetSelectedItem() const;

	// Removes 1 unit from the currently selected hotbar slot. Returns false if no
	// slot is selected or it's already empty. UI repaints via OnHotbarChanged +
	// OnSelectedHotbarChanged (the latter so the hand-equipped mesh refreshes).
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	bool ConsumeSelectedItem();

private:
	FInventorySlot* GetMutableSlot(const FInventorySlotReference& Ref);
	void BroadcastSlotChanged(const FInventorySlotReference& Ref);

	UPROPERTY(VisibleInstanceOnly, Category = "Inventory|Hotbar")
	int32 SelectedHotbarIndex = INDEX_NONE;
};
