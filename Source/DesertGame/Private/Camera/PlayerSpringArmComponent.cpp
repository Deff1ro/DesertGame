// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/PlayerSpringArmComponent.h"
#include "Enemy/EnemyCharacter.h"
#include "Inventory/ItemActor.h"
#include "Resources/ResourceNode.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"

UPlayerSpringArmComponent::UPlayerSpringArmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerSpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	// Before the engine probe runs, force every dynamic actor that should not block
	// the camera to ignore the Camera trace channel. Doing this here (every frame)
	// guarantees the setting overrides any Blueprint-level collision preset that
	// might have been re-applied since BeginPlay.
	if (UWorld* World = GetWorld())
	{
		auto IgnoreCameraOnAllPrimitives = [](AActor* Actor)
		{
			if (!Actor)
			{
				return;
			}
			TArray<UPrimitiveComponent*> Primitives;
			Actor->GetComponents<UPrimitiveComponent>(Primitives);
			for (UPrimitiveComponent* Prim : Primitives)
			{
				if (Prim && Prim->GetCollisionResponseToChannel(ECC_Camera) != ECR_Ignore)
				{
					Prim->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
				}
			}
		};

		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			IgnoreCameraOnAllPrimitives(*It);
		}
		for (TActorIterator<AItemActor> It(World); It; ++It)
		{
			IgnoreCameraOnAllPrimitives(*It);
		}
		for (TActorIterator<AResourceNode> It(World); It; ++It)
		{
			IgnoreCameraOnAllPrimitives(*It);
		}
	}

	Super::UpdateDesiredArmLocation(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
}
