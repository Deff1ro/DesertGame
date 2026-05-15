// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AnimNotify_SwordHit.h"
#include "Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SwordHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
	{
		Combat->PerformSwordHit();
	}
}
