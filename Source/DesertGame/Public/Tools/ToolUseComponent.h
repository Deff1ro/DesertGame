// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "ToolUseComponent.generated.h"

class UAnimMontage;
class UItemDataAsset;
class AProtagonistCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertTool, Log, All);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UToolUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UToolUseComponent();

	// Запоминает активный инструмент И проигрывает монтаж. Для топора/кирки.
	UFUNCTION(BlueprintCallable, Category = "Tool")
	bool RequestUseTool();

	// Только запоминает активный инструмент без монтажа. Для меча (монтаж играет CombatComponent).
	UFUNCTION(BlueprintCallable, Category = "Tool")
	void ArmTool();

	// Вызывается UAnimNotify_ToolHit в нужный кадр анимации.
	UFUNCTION(BlueprintCallable, Category = "Tool")
	void PerformToolHit();

protected:
	virtual void BeginPlay() override;

	// Соответствие "тип инструмента → монтаж". Заполняется в редакторе.
	UPROPERTY(EditDefaultsOnly, Category = "Tool|Animations")
	TMap<EToolType, TObjectPtr<UAnimMontage>> ToolMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Tool|Trace")
	float HitTraceDistance = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Tool|Trace")
	float HitTraceRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Tool|Trace")
	bool bDebugDraw = false;

	// Sounds per tool type. Played on every notify (swing). Leave empty for None.
	UPROPERTY(EditDefaultsOnly, Category = "Tool|Audio")
	TMap<EToolType, TObjectPtr<class USoundBase>> ToolSwingSounds;

	// Sound played when the swing actually hits a resource node or enemy.
	UPROPERTY(EditDefaultsOnly, Category = "Tool|Audio")
	TObjectPtr<class USoundBase> ToolHitSound;

private:
	UPROPERTY()
	TObjectPtr<AProtagonistCharacter> OwnerCharacter;

	// Тип и данные инструмента, запомненные в момент RequestUseTool/ArmTool
	EToolType ActiveToolType = EToolType::None;

	UPROPERTY()
	TObjectPtr<UItemDataAsset> ActiveToolData;

	// True while an axe/pickaxe swing montage is still playing. Blocks new
	// swings and freezes the character in place.
	bool bIsSwinging = false;

	// Montage we locked input on; remembered so we can clean the end-delegate.
	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveMontage;

	UFUNCTION()
	void OnSwingMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void LockMovement();
	void UnlockMovement();
};
