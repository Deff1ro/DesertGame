// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DesertGameInstance.generated.h"

class UDesertSaveGame;
class AProtagonistCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertSave, Log, All);

UCLASS()
class DESERTGAME_API UDesertGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool HasSaveGame() const;

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool SaveGame(AProtagonistCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	UDesertSaveGame* LoadSaveGameData() const;

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool DeleteSaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void RequestLoadOnNextLevel();

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	bool ShouldApplyLoadedSave() const;

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void ConsumeLoadRequest();

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	FString GetSavedLevelName() const;

	// ============================================================
	// World-item pickup tracking (consumed by AItemActor::BeginPlay)
	// ============================================================

	// Called by AItemActor when it is fully picked up. Records its FName so the
	// item won't respawn on next load (until the player explicitly starts a new game).
	UFUNCTION(BlueprintCallable, Category = "SaveSystem|Items")
	void RegisterPickedUpItem(FName ItemId);

	UFUNCTION(BlueprintPure, Category = "SaveSystem|Items")
	bool IsItemPickedUp(FName ItemId) const;

	// Reset the in-memory picked-up set (used by "New Game")
	UFUNCTION(BlueprintCallable, Category = "SaveSystem|Items")
	void ResetPickedUpItems();

	// Replace the in-memory picked-up set from a saved array (used after loading)
	void RestorePickedUpItemsFrom(const TArray<FName>& Saved);

private:
	bool bShouldLoadSaveOnBeginPlay = false;

	UPROPERTY()
	TSet<FName> PickedUpItemIds;
};
