// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DeathMenuWidget.h"
#include "Core/DesertGameInstance.h"
#include "SaveSystem/DesertSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UDeathMenuWidget::OnReturnClicked()
{
	UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>();
	if (!GI || !GI->HasSaveGame())
	{
		return;
	}

	UDesertSaveGame* SaveData = GI->LoadSaveGameData();
	FString SavedLevelName;
	if (SaveData)
	{
		SavedLevelName = SaveData->LevelName;
		GI->RestorePickedUpItemsFrom(SaveData->PickedUpItemIds);
	}

	if (SavedLevelName.IsEmpty())
	{
		// No saved level recorded — fall back to current level so the load still
		// applies on the next BeginPlay.
		SavedLevelName = UGameplayStatics::GetCurrentLevelName(this);
	}

	GI->RequestLoadOnNextLevel();

	// Make sure we're not still paused before tearing the world down.
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(this, false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UGameplayStatics::OpenLevel(this, FName(*SavedLevelName));
}

void UDeathMenuWidget::OnMainMenuClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::SetGamePaused(this, false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UDeathMenuWidget::OnExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

bool UDeathMenuWidget::HasSaveGame() const
{
	if (const UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		return GI->HasSaveGame();
	}
	return false;
}
