// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/DesertGameInstance.h"
#include "SaveSystem/DesertSaveGame.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogDesertSave);

bool UDesertGameInstance::HasSaveGame() const
{
	return UGameplayStatics::DoesSaveGameExist(UDesertSaveGame::SaveSlotName, UDesertSaveGame::UserIndex);
}

bool UDesertGameInstance::SaveGame(AProtagonistCharacter* Player)
{
	if (!Player)
	{
		UE_LOG(LogDesertSave, Warning, TEXT("SaveGame failed: Player is null"));
		return false;
	}

	USaveGame* SaveObject = UGameplayStatics::CreateSaveGameObject(UDesertSaveGame::StaticClass());
	UDesertSaveGame* DesertSave = Cast<UDesertSaveGame>(SaveObject);
	if (!DesertSave)
	{
		UE_LOG(LogDesertSave, Error, TEXT("SaveGame failed: could not create UDesertSaveGame instance"));
		return false;
	}

	DesertSave->LevelName = UGameplayStatics::GetCurrentLevelName(this);
	DesertSave->PlayerLocation = Player->GetActorLocation();
	DesertSave->PlayerRotation = Player->GetActorRotation();
	DesertSave->StanceState = Player->GetStanceState();
	DesertSave->SaveTimestamp = FDateTime::Now();

	// Inventory snapshot
	if (UInventoryComponent* Inv = Player->GetInventoryComponent())
	{
		auto SerializeSlots = [](const TArray<FInventorySlot>& InSlots, TArray<FSavedInventorySlot>& OutSaved)
		{
			OutSaved.Reset(InSlots.Num());
			for (const FInventorySlot& Slot : InSlots)
			{
				FSavedInventorySlot Saved;
				Saved.ItemData = Slot.ItemData;
				Saved.Quantity = Slot.IsEmpty() ? 0 : Slot.Quantity;
				OutSaved.Add(Saved);
			}
		};

		SerializeSlots(Inv->GetInventorySlots(), DesertSave->InventorySlots);
		SerializeSlots(Inv->GetHotbarSlots(), DesertSave->HotbarSlots);
	}

	// World items pickup snapshot
	DesertSave->PickedUpItemIds = PickedUpItemIds.Array();

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(DesertSave, UDesertSaveGame::SaveSlotName, UDesertSaveGame::UserIndex);

	if (bSuccess)
	{
		UE_LOG(LogDesertSave, Log, TEXT("SaveGame OK | Level=%s | Pos=%s"),
			*DesertSave->LevelName,
			*DesertSave->PlayerLocation.ToString());
	}
	else
	{
		UE_LOG(LogDesertSave, Error, TEXT("SaveGame failed: SaveGameToSlot returned false"));
	}

	return bSuccess;
}

UDesertSaveGame* UDesertGameInstance::LoadSaveGameData() const
{
	if (!HasSaveGame())
	{
		return nullptr;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(UDesertSaveGame::SaveSlotName, UDesertSaveGame::UserIndex);
	UDesertSaveGame* DesertSave = Cast<UDesertSaveGame>(Loaded);

	if (DesertSave)
	{
		UE_LOG(LogDesertSave, Log, TEXT("LoadSaveGameData OK | Level=%s | Pos=%s"),
			*DesertSave->LevelName,
			*DesertSave->PlayerLocation.ToString());
	}
	else
	{
		UE_LOG(LogDesertSave, Error, TEXT("LoadSaveGameData failed: cast to UDesertSaveGame returned null"));
	}

	return DesertSave;
}

bool UDesertGameInstance::DeleteSaveGame()
{
	const bool bSuccess = UGameplayStatics::DeleteGameInSlot(UDesertSaveGame::SaveSlotName, UDesertSaveGame::UserIndex);
	ResetPickedUpItems();
	UE_LOG(LogDesertSave, Log, TEXT("DeleteSaveGame: %s"), bSuccess ? TEXT("OK") : TEXT("FAILED / no save"));
	return bSuccess;
}

void UDesertGameInstance::RegisterPickedUpItem(FName ItemId)
{
	if (!ItemId.IsNone())
	{
		PickedUpItemIds.Add(ItemId);
	}
}

bool UDesertGameInstance::IsItemPickedUp(FName ItemId) const
{
	return PickedUpItemIds.Contains(ItemId);
}

void UDesertGameInstance::ResetPickedUpItems()
{
	PickedUpItemIds.Reset();
}

void UDesertGameInstance::RestorePickedUpItemsFrom(const TArray<FName>& Saved)
{
	PickedUpItemIds.Reset();
	PickedUpItemIds.Append(Saved);
}

void UDesertGameInstance::RequestLoadOnNextLevel()
{
	bShouldLoadSaveOnBeginPlay = true;
}

bool UDesertGameInstance::ShouldApplyLoadedSave() const
{
	return bShouldLoadSaveOnBeginPlay;
}

void UDesertGameInstance::ConsumeLoadRequest()
{
	bShouldLoadSaveOnBeginPlay = false;
}

FString UDesertGameInstance::GetSavedLevelName() const
{
	if (UDesertSaveGame* SaveData = LoadSaveGameData())
	{
		return SaveData->LevelName;
	}
	return FString();
}
