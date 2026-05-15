// Fill out your copyright notice in the Description page of Project Settings.

#include "Tools/AnimNotify_ToolHit.h"
#include "Tools/ToolUseComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_ToolHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	if (UToolUseComponent* ToolComp = Owner->FindComponentByClass<UToolUseComponent>())
	{
		ToolComp->PerformToolHit();
	}
}
