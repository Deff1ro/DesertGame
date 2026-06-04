// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS()
class DESERTGAME_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnNewGameClicked();

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnContinueClicked();

	UFUNCTION(BlueprintCallable, Category = "MainMenu")
	void OnExitClicked();

	UFUNCTION(BlueprintPure, Category = "MainMenu")
	bool HasSaveGame() const;

	UPROPERTY(EditDefaultsOnly, Category = "MainMenu")
	FName GameLevelName = TEXT("L_Desert");

	// Looping ambient track for the main menu screen. Stopped when the widget
	// is removed (e.g. New Game / Continue → level change).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainMenu|Audio")
	TObjectPtr<USoundBase> BackgroundMusic;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> BackgroundMusicComponent;
};
