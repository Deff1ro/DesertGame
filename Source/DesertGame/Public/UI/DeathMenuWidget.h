// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathMenuWidget.generated.h"

// Death screen — popped up by AProtagonistCharacter when the player's HP
// reaches zero. Three buttons, wired from the BP via BlueprintCallable:
//  - Return: load the most recent save (if any).
//  - Main Menu: open the main menu level.
//  - Exit: quit the game.
UCLASS()
class DESERTGAME_API UDeathMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DeathMenu")
	void OnReturnClicked();

	UFUNCTION(BlueprintCallable, Category = "DeathMenu")
	void OnMainMenuClicked();

	UFUNCTION(BlueprintCallable, Category = "DeathMenu")
	void OnExitClicked();

	UFUNCTION(BlueprintPure, Category = "DeathMenu")
	bool HasSaveGame() const;

	// Level that gets opened by the Main Menu button.
	UPROPERTY(EditDefaultsOnly, Category = "DeathMenu")
	FName MainMenuLevelName = TEXT("L_MainMenu");
};
