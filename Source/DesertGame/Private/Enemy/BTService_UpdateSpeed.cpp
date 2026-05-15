// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/BTService_UpdateSpeed.h"
#include "Enemy/EnemyCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_UpdateSpeed::UBTService_UpdateSpeed()
{
	NodeName = TEXT("Update Speed");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateSpeed::TickNode(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AIController->GetPawn());
	if (!Enemy) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	if (!Movement) return;

	const UObject* Target = BB->GetValueAsObject(FName("TargetActor"));
	Movement->MaxWalkSpeed = Target ? Enemy->ChaseSpeed : Enemy->PatrolSpeed;
}
