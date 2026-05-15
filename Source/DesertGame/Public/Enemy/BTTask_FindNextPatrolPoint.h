// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindNextPatrolPoint.generated.h"

UCLASS()
class DESERTGAME_API UBTTask_FindNextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindNextPatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
};
