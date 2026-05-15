// Fill out your copyright notice in the Description page of Project Settings.

#include "Tools/ToolUseComponent.h"
#include "Resources/ResourceNode.h"
#include "Enemy/EnemyCharacter.h"
#include "Enemy/WormEnemy.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY(LogDesertTool);

UToolUseComponent::UToolUseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UToolUseComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AProtagonistCharacter>(GetOwner());
}

bool UToolUseComponent::RequestUseTool()
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UInventoryComponent* Inv = OwnerCharacter->GetInventoryComponent();
	if (!Inv)
	{
		return false;
	}

	UItemDataAsset* Selected = Inv->GetSelectedItem();
	if (!Selected || Selected->ToolType == EToolType::None)
	{
		return false;
	}

	TObjectPtr<UAnimMontage>* Found = ToolMontages.Find(Selected->ToolType);
	if (!Found || !(*Found))
	{
		UE_LOG(LogDesertTool, Warning, TEXT("UToolUseComponent: no montage for tool type %d"), (int32)Selected->ToolType);
		return false;
	}

	UAnimInstance* AnimInst = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		return false;
	}

	ActiveToolType = Selected->ToolType;
	ActiveToolData = Selected;

	AnimInst->Montage_Play(*Found);
	UE_LOG(LogDesertTool, Log, TEXT("UToolUseComponent: playing montage for tool type %d"), (int32)ActiveToolType);
	return true;
}

void UToolUseComponent::ArmTool()
{
	if (!OwnerCharacter)
	{
		return;
	}

	UInventoryComponent* Inv = OwnerCharacter->GetInventoryComponent();
	if (!Inv)
	{
		return;
	}

	UItemDataAsset* Selected = Inv->GetSelectedItem();
	ActiveToolType = Selected ? Selected->ToolType : EToolType::None;
	ActiveToolData = Selected;

	UE_LOG(LogDesertTool, Log, TEXT("UToolUseComponent: armed tool type %d"), (int32)ActiveToolType);
}

void UToolUseComponent::PerformToolHit()
{
	if (ActiveToolType == EToolType::None || !OwnerCharacter)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 30.f;
	const FVector End = Start + OwnerCharacter->GetActorForwardVector() * HitTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	// Swing sound — per-tool entry in the map, played on every notify.
	if (TObjectPtr<USoundBase>* SwingSound = ToolSwingSounds.Find(ActiveToolType))
	{
		if (*SwingSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, *SwingSound, OwnerCharacter->GetActorLocation());
		}
	}

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(HitTraceRadius),
		Params);

	if (bDebugDraw)
	{
		UKismetSystemLibrary::DrawDebugCapsule(
			World,
			(Start + End) * 0.5f,
			HitTraceDistance * 0.5f,
			HitTraceRadius,
			FRotator(90.f, 0.f, 0.f),
			bHit ? FLinearColor::Green : FLinearColor::Red,
			2.0f);
	}

	if (bHit && ActiveToolData)
	{
		// Hit-confirm sound — once per swing when at least one valid target was struck.
		if (ToolHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ToolHitSound, OwnerCharacter->GetActorLocation());
		}

		// Apply damage at most once per actor per swing (capsule can register
		// multiple hits on the same character).
		TSet<AActor*> HitActors;
		for (const FHitResult& H : Hits)
		{
			AActor* HitActor = H.GetActor();
			if (!HitActor || HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			if (AResourceNode* Node = Cast<AResourceNode>(HitActor))
			{
				Node->ApplyHit(ActiveToolData, ActiveToolData->ToolDamage);
				UE_LOG(LogDesertTool, Log, TEXT("PerformToolHit — node '%s' for %d"),
					*Node->GetName(), ActiveToolData->ToolDamage);
			}
			else if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(HitActor))
			{
				Enemy->TakeDamageAmount(static_cast<float>(ActiveToolData->ToolDamage));
				UE_LOG(LogDesertTool, Log, TEXT("PerformToolHit — enemy '%s' for %d"),
					*Enemy->GetName(), ActiveToolData->ToolDamage);
			}
			else if (AWormEnemy* Worm = Cast<AWormEnemy>(HitActor))
			{
				Worm->TakeDamageAmount(static_cast<float>(ActiveToolData->ToolDamage));
				UE_LOG(LogDesertTool, Log, TEXT("PerformToolHit — worm '%s' for %d"),
					*Worm->GetName(), ActiveToolData->ToolDamage);
			}
		}
	}
	else
	{
		UE_LOG(LogDesertTool, Log, TEXT("UToolUseComponent::PerformToolHit — no target"));
	}

	ActiveToolType = EToolType::None;
	ActiveToolData = nullptr;
}
