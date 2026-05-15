// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryComponent.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"
#include "Core/DesertGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupZone = CreateDefaultSubobject<USphereComponent>(TEXT("PickupZone"));
	PickupZone->SetupAttachment(MeshComponent);
	PickupZone->SetSphereRadius(150.0f);
	PickupZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupZone->OnComponentBeginOverlap.AddDynamic(this, &AItemActor::OnPickupZoneBeginOverlap);
	PickupZone->OnComponentEndOverlap.AddDynamic(this, &AItemActor::OnPickupZoneEndOverlap);
}

void AItemActor::Initialize(UItemDataAsset* InData, int32 InQuantity)
{
	ItemData = InData;
	Quantity = FMath::Max(1, InQuantity);

	if (ItemData && ItemData->WorldMesh && MeshComponent)
	{
		MeshComponent->SetStaticMesh(ItemData->WorldMesh);
	}
}

void AItemActor::LaunchAsDrop(const FVector& Impulse)
{
	if (!MeshComponent)
	{
		return;
	}

	// Pawn must overlap so the player can still walk into the pickup zone, but world
	// geometry must block to let the drop rest on the ground.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// Don't push the player's spring-arm if the drop flies past the camera.
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->AddImpulse(Impulse, NAME_None, /*bVelChange*/ true);
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply at runtime so this overrides any Blueprint collision-preset override.
	// Without this, falling drops can briefly intersect the camera probe and yank
	// the spring-arm inward, exposing the inside of the player mesh.
	if (MeshComponent)
	{
		MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// If this placement-actor was already picked up in the loaded save, remove it
	if (UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
	{
		if (GI->IsItemPickedUp(GetFName()))
		{
			UE_LOG(LogDesertInventory, Log, TEXT("AItemActor '%s' was picked up in save — destroying"), *GetName());
			Destroy();
			return;
		}
	}

	if (!ItemData)
	{
		UE_LOG(LogDesertInventory, Warning, TEXT("AItemActor '%s' has no ItemData assigned"), *GetName());
		return;
	}

	if (ItemData->WorldMesh && MeshComponent)
	{
		MeshComponent->SetStaticMesh(ItemData->WorldMesh);
	}
}

void AItemActor::OnPickupZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);
	}
}

void AItemActor::OnPickupZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(OtherActor))
	{
		Player->ClearInteractableIfMatches(this);
	}
}

void AItemActor::TryPickup(AProtagonistCharacter* Picker)
{
	if (!ItemData || !Picker)
	{
		return;
	}

	UInventoryComponent* Inv = Picker->GetInventoryComponent();
	if (!Inv)
	{
		return;
	}

	const int32 Remaining = Inv->AddItem(ItemData, Quantity);

	if (Remaining < Quantity && PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	if (Remaining == 0)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("Pickup: %s x%d fully taken — destroying actor"), *ItemData->GetName(), Quantity);
		Picker->ClearInteractableIfMatches(this);

		if (UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>())
		{
			GI->RegisterPickedUpItem(GetFName());
		}

		Destroy();
	}
	else if (Remaining < Quantity)
	{
		UE_LOG(LogDesertInventory, Log, TEXT("Pickup: %s — partial pickup, %d remain in world"), *ItemData->GetName(), Remaining);
		Quantity = Remaining;
	}
	else
	{
		UE_LOG(LogDesertInventory, Log, TEXT("Pickup: %s — inventory full, nothing taken"), *ItemData->GetName());
	}
}
