// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/AnimNotify_EnemyAttackHit.h"
#include "Enemy/EnemyAttackComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_EnemyAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UEnemyAttackComponent* AttackComp = Owner->FindComponentByClass<UEnemyAttackComponent>())
	{
		AttackComp->PerformAttackHit();
	}
}
