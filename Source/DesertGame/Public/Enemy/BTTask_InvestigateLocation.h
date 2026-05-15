// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_InvestigateLocation.generated.h"

/**
 * Moves to LastKnownLocation and waits there briefly, then clears it.
 * Used when the enemy loses sight of the player.
 */
UCLASS()
class DESERTGAME_API UBTTask_InvestigateLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_InvestigateLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;

private:
	float WaitTimer = 0.f;
	bool bReachedLocation = false;

	UPROPERTY(EditAnywhere, Category = "Investigation")
	float WaitDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Investigation")
	float AcceptanceRadius = 100.f;
};
