// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/BTTask_InvestigateLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_InvestigateLocation::UBTTask_InvestigateLocation()
{
	NodeName = TEXT("Investigate Location");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_InvestigateLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB) return EBTNodeResult::Failed;

	const FVector Location = BB->GetValueAsVector(FName("LastKnownLocation"));
	if (Location.IsZero()) return EBTNodeResult::Failed;

	WaitTimer = 0.f;
	bReachedLocation = false;

	AIController->MoveToLocation(Location, AcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UBTTask_InvestigateLocation::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// If we re-acquired the target while investigating, abort
	if (BB->GetValueAsObject(FName("TargetActor")))
	{
		BB->ClearValue(FName("LastKnownLocation"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (!bReachedLocation)
	{
		EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
		if (MoveStatus != EPathFollowingStatus::Moving)
		{
			bReachedLocation = true;
			WaitTimer = 0.f;
		}
		return;
	}

	// Wait at location
	WaitTimer += DeltaSeconds;
	if (WaitTimer >= WaitDuration)
	{
		BB->ClearValue(FName("LastKnownLocation"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
