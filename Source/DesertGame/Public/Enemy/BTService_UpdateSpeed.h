// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateSpeed.generated.h"

UCLASS()
class DESERTGAME_API UBTService_UpdateSpeed : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateSpeed();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;
};
