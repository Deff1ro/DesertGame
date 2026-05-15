// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ProtagonistHUD.h"
#include "UI/MainHUDWidget.h"
#include "Blueprint/UserWidget.h"

void AProtagonistHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!MainHUDWidgetClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	MainHUDWidget = CreateWidget<UMainHUDWidget>(PC, MainHUDWidgetClass);
	if (MainHUDWidget)
	{
		MainHUDWidget->AddToViewport();
	}
}
