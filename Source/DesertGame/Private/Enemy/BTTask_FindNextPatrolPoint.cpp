// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/BTTask_FindNextPatrolPoint.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindNextPatrolPoint::UBTTask_FindNextPatrolPoint()
{
	NodeName = TEXT("Find Next Patrol Point");
}

EBTNodeResult::Type UBTTask_FindNextPatrolPoint::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	const FVector NextPoint = Enemy->GetNextPatrolPoint();

	OwnerComp.GetBlackboardComponent()->SetValueAsVector(
		FName("PatrolLocation"), NextPoint);

	return EBTNodeResult::Succeeded;
}
