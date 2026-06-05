// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class USoundBase;
class UAudioComponent;

// Pause overlay — opened/closed by AProtagonistCharacter on the Pause input
// (Esc by default). Two buttons:
//  - Resume: unpause and tear the widget down.
//  - Exit: quit the game.
UCLASS()
class DESERTGAME_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "PauseMenu")
	void OnResumeClicked();

	UFUNCTION(BlueprintCallable, Category = "PauseMenu")
	void OnExitClicked();

	// Looping ambient track for the pause screen. Plays as a 2D UI sound so it
	// keeps playing while the game world is paused. Stopped when the widget
	// is removed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PauseMenu|Audio")
	TObjectPtr<USoundBase> BackgroundMusic;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> BackgroundMusicComponent;
};
