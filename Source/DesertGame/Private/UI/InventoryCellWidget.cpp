// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventoryCellWidget.h"
#include "UI/ItemDragDropOperation.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"

void UInventoryCellWidget::SetItemIcon(UTexture2D* NewIcon)
{
	ItemIcon = NewIcon;
	OnIconUpdated(NewIcon);
}

void UInventoryCellWidget::ClearCell()
{
	SetItemIcon(nullptr);
	CurrentSlot.Clear();
	OnSlotDataUpdated(nullptr, 0, true);
}

void UInventoryCellWidget::SetSlotData(const FInventorySlot& InSlot)
{
	CurrentSlot = InSlot;
	UTexture2D* Icon = (InSlot.ItemData) ? InSlot.ItemData->Icon : nullptr;
	ItemIcon = Icon;
	OnSlotDataUpdated(Icon, InSlot.Quantity, InSlot.IsEmpty());
}

void UInventoryCellWidget::ConfigureSlotReference(ESlotContainer Container, int32 Index)
{
	SlotReference.Container = Container;
	SlotReference.Index = Index;
}

void UInventoryCellWidget::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}
	bSelected = bInSelected;
	OnSelectionChanged(bSelected);
}

FReply UInventoryCellWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggable && !CurrentSlot.IsEmpty() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryCellWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (CurrentSlot.IsEmpty())
	{
		return;
	}

	UItemDragDropOperation* Op = NewObject<UItemDragDropOperation>(this);
	Op->SourceSlot = SlotReference;
	Op->DraggedItemData = CurrentSlot.ItemData;
	Op->DraggedQuantity = CurrentSlot.Quantity;
	Op->Pivot = EDragPivot::CenterCenter;

	if (DragVisualClass)
	{
		if (UUserWidget* Visual = CreateWidget<UUserWidget>(this, DragVisualClass))
		{
			Op->DefaultDragVisual = Visual;
		}
	}

	OutOperation = Op;
}

bool UInventoryCellWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!bAcceptsDrop)
	{
		return false;
	}

	UItemDragDropOperation* DragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!DragOp)
	{
		return false;
	}

	// Category filter — restricted slots reject anything that doesn't match.
	if (bRestrictByCategory)
	{
		if (!DragOp->DraggedItemData || DragOp->DraggedItemData->Category != AcceptedCategory)
		{
			return false;
		}
	}

	APawn* Pawn = GetOwningPlayerPawn();
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(Pawn);
	if (!Player)
	{
		return false;
	}

	UInventoryComponent* Inv = Player->GetInventoryComponent();
	if (!Inv)
	{
		return false;
	}

	return Inv->MoveItem(DragOp->SourceSlot, SlotReference);
}
