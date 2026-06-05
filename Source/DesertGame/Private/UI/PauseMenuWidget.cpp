// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PauseMenuWidget.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// SpawnSound2D produces a UI-flagged audio component that survives a
	// global pause — exactly what we want for pause-screen music.
	if (BackgroundMusic && !BackgroundMusicComponent)
	{
		BackgroundMusicComponent = UGameplayStatics::SpawnSound2D(
			this, BackgroundMusic, /*VolumeMultiplier*/ 1.f, /*PitchMultiplier*/ 1.f,
			/*StartTime*/ 0.f, /*ConcurrencySettings*/ nullptr, /*bPersistAcrossLevelTransition*/ false,
			/*bAutoDestroy*/ false);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	if (BackgroundMusicComponent)
	{
		BackgroundMusicComponent->Stop();
		BackgroundMusicComponent = nullptr;
	}

	Super::NativeDestruct();
}

void UPauseMenuWidget::OnResumeClicked()
{
	// Hand control back to the character — it owns the pause widget and knows
	// how to tear it down + restore input/cursor.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(PC->GetPawn()))
		{
			Player->TogglePauseMenu();
			return;
		}
	}

	// Fallback (no player pawn — shouldn't happen in normal flow): just unpause
	// and remove ourselves so the widget doesn't get stuck on screen.
	UGameplayStatics::SetGamePaused(this, false);
	RemoveFromParent();
}

void UPauseMenuWidget::OnExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
