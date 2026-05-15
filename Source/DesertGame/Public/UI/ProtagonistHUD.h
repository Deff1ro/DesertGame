// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ProtagonistHUD.generated.h"

class UMainHUDWidget;

UCLASS()
class DESERTGAME_API AProtagonistHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UMainHUDWidget> MainHUDWidget;
};
