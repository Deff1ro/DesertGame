// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MainMenuWidget.h"
#include "Core/DesertGameInstance.h"
#include "SaveSystem/DesertSaveGame.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackgroundMusic && !BackgroundMusicComponent)
	{
		BackgroundMusicComponent = UGameplayStatics::SpawnSound2D(
			this, BackgroundMusic, /*VolumeMultiplier*/ 1.f, /*PitchMultiplier*/ 1.f,
			/*StartTime*/ 0.f, /*ConcurrencySettings*/ nullptr, /*bPersistAcrossLevelTransition*/ false,
			/*bAutoDestroy*/ false);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	if (BackgroundMusicComponent)
	{
		BackgroundMusicComponent->Stop();
		BackgroundMusicComponent = nullptr;
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::OnNewGameClicked()
{
	if (UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		GI->DeleteSaveGame();
	}

	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UMainMenuWidget::OnContinueClicked()
{
	UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>();
	if (!GI)
	{
		return;
	}

	if (!GI->HasSaveGame())
	{
		return;
	}

	UDesertSaveGame* SaveData = GI->LoadSaveGameData();
	FString SavedLevelName;
	if (SaveData)
	{
		SavedLevelName = SaveData->LevelName;
		// Restore picked-up items set BEFORE OpenLevel so AItemActor::BeginPlay
		// on the next map can self-destroy already-collected placement actors.
		GI->RestorePickedUpItemsFrom(SaveData->PickedUpItemIds);
	}

	if (SavedLevelName.IsEmpty())
	{
		SavedLevelName = GameLevelName.ToString();
	}

	GI->RequestLoadOnNextLevel();
	UGameplayStatics::OpenLevel(this, FName(*SavedLevelName));
}

void UMainMenuWidget::OnExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

bool UMainMenuWidget::HasSaveGame() const
{
	if (const UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		return GI->HasSaveGame();
	}
	return false;
}
