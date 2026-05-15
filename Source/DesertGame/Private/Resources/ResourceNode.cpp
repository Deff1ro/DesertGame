// Fill out your copyright notice in the Description page of Project Settings.

#include "Resources/ResourceNode.h"
#include "Resources/ResourceNodeData.h"
#include "Inventory/ItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogDesertResource);

AResourceNode::AResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AResourceNode::BeginPlay()
{
	Super::BeginPlay();

	if (NodeData)
	{
		CurrentHealth = NodeData->MaxHealth;
		if (NodeData->Mesh && MeshComponent)
		{
			MeshComponent->SetStaticMesh(NodeData->Mesh);
		}
	}
	else
	{
		UE_LOG(LogDesertResource, Warning, TEXT("AResourceNode '%s': no NodeData assigned"), *GetName());
		CurrentHealth = 1;
	}
}

void AResourceNode::ApplyHit(UItemDataAsset* UsedTool, int32 Damage)
{
	if (!NodeData || !UsedTool)
	{
		return;
	}

	if (UsedTool->ToolType != NodeData->RequiredTool)
	{
		UE_LOG(LogDesertResource, Log, TEXT("AResourceNode '%s': wrong tool (need %d, got %d) — no effect"),
			*GetName(), (int32)NodeData->RequiredTool, (int32)UsedTool->ToolType);
		return;
	}

	CurrentHealth -= Damage;
	UE_LOG(LogDesertResource, Log, TEXT("AResourceNode '%s': hit %d -> %d HP"),
		*GetName(), Damage, CurrentHealth);

	if (CurrentHealth <= 0)
	{
		SpawnDrops();
		Destroy();
	}
}

void AResourceNode::SpawnDrops()
{
	if (!NodeData || !NodeData->DropItem || !DropActorClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 Count = FMath::RandRange(NodeData->DropMin, FMath::Max(NodeData->DropMin, NodeData->DropMax));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, NodeData->DropSpawnHeight);

	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const FVector LateralDir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);

		// Spawn slightly offset so multiple drops don't appear inside each other
		const float SpawnOffsetDistance = FMath::FRandRange(0.f, FMath::Min(NodeData->DropScatterRadius * 0.25f, 25.f));
		const FRotator SpawnRotation = FMath::VRand().Rotation();
		const FVector SpawnLocation = Origin + LateralDir * SpawnOffsetDistance;

		AItemActor* Item = World->SpawnActor<AItemActor>(DropActorClass, SpawnLocation, SpawnRotation, Params);
		if (!Item)
		{
			continue;
		}

		Item->Initialize(NodeData->DropItem, 1);

		// Light outward + upward kick so drops fan out and tumble before landing.
		// Using bVelChange=true so the impulse is mass-independent and consistent.
		const float LateralStrength = FMath::FRandRange(150.f, 300.f);
		const float UpwardStrength = FMath::FRandRange(250.f, 400.f);
		const FVector Impulse = LateralDir * LateralStrength + FVector(0.f, 0.f, UpwardStrength);

		Item->LaunchAsDrop(Impulse);
	}

	UE_LOG(LogDesertResource, Log, TEXT("AResourceNode '%s': spawned %d drops of '%s'"),
		*GetName(), Count, *NodeData->DropItem->GetName());
}
