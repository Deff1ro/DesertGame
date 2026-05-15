// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class DESERTGAME_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
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
};
