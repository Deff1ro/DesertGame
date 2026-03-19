// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AnimNotifyState_ComboWindow.h"
#include "Combat/CombatComponent.h"

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UCombatComponent>())
		{
			Combat->OpenComboWindow();
		}
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UCombatComponent* Combat = MeshComp->GetOwner()->FindComponentByClass<UCombatComponent>())
		{
			Combat->CloseComboWindow();
		}
	}
}
