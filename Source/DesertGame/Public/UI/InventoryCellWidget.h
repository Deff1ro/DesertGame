// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryTypes.h"
#include "Inventory/ItemDataAsset.h"
#include "InventoryCellWidget.generated.h"

class UTexture2D;
class UDragDropOperation;

UCLASS()
class DESERTGAME_API UInventoryCellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ============================================================
	// Legacy icon API (kept for compatibility)
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "InventoryCell")
	void SetItemIcon(UTexture2D* NewIcon);

	UFUNCTION(BlueprintCallable, Category = "InventoryCell")
	void ClearCell();

	UFUNCTION(BlueprintPure, Category = "InventoryCell")
	bool IsEmpty() const { return CurrentSlot.IsEmpty(); }

	// Convenience — same as CurrentSlot.IsEmpty(), exposed explicitly for Blueprint clarity
	UFUNCTION(BlueprintPure, Category = "InventoryCell")
	bool IsSlotEmpty() const { return CurrentSlot.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "InventoryCell")
	UItemDataAsset* GetCurrentItemData() const { return CurrentSlot.ItemData; }

	UFUNCTION(BlueprintPure, Category = "InventoryCell")
	int32 GetCurrentQuantity() const { return CurrentSlot.Quantity; }

	// ============================================================
	// Slot API
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "InventoryCell")
	void SetSlotData(const FInventorySlot& InSlot);

	UFUNCTION(BlueprintCallable, Category = "InventoryCell")
	void ConfigureSlotReference(ESlotContainer Container, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "InventoryCell")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "InventoryCell")
	bool IsSelected() const { return bSelected; }

	// ============================================================
	// Drag & Drop config
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "InventoryCell|DragDrop")
	TSubclassOf<UUserWidget> DragVisualClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventoryCell|DragDrop")
	bool bIsDraggable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventoryCell|DragDrop")
	bool bAcceptsDrop = true;

	// When true the cell only accepts items whose ItemDataAsset.Category matches
	// AcceptedCategory. Other drops are rejected. Use for cloth-only / armor-only
	// equipment slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventoryCell|DragDrop")
	bool bRestrictByCategory = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventoryCell|DragDrop",
		meta = (EditCondition = "bRestrictByCategory"))
	EItemCategory AcceptedCategory = EItemCategory::Cloth;

	// ============================================================
	// UUserWidget overrides (drag & drop)
	// ============================================================

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

protected:
	// Legacy
	UFUNCTION(BlueprintImplementableEvent, Category = "InventoryCell")
	void OnIconUpdated(UTexture2D* Icon);

	// Extended — receives full slot data
	UFUNCTION(BlueprintImplementableEvent, Category = "InventoryCell")
	void OnSlotDataUpdated(UTexture2D* Icon, int32 Quantity, bool bIsEmpty);

	// Implemented in BP — show/hide a highlight border, change color, etc.
	UFUNCTION(BlueprintImplementableEvent, Category = "InventoryCell")
	void OnSelectionChanged(bool bIsSelected);

	UPROPERTY(BlueprintReadOnly, Category = "InventoryCell")
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(BlueprintReadOnly, Category = "InventoryCell")
	FInventorySlotReference SlotReference;

	UPROPERTY(BlueprintReadOnly, Category = "InventoryCell")
	bool bSelected = false;

	// VisibleAnywhere — Blueprint может Break-ать структуру и читать ItemData / Quantity
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryCell")
	FInventorySlot CurrentSlot;
};
