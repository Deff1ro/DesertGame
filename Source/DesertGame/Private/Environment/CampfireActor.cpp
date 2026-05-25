// Fill out your copyright notice in the Description page of Project Settings.

#include "Environment/CampfireActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"

TArray<TWeakObjectPtr<ACampfireActor>> ACampfireActor::ActiveCampfires;

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// Don't push the player's spring-arm when the camera passes the flame.
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	WarmthZone = CreateDefaultSubobject<USphereComponent>(TEXT("WarmthZone"));
	WarmthZone->SetupAttachment(MeshComponent);
	WarmthZone->SetSphereRadius(WarmthRadius);
	// Pure query volume — no overlap/block, just used for the radius value.
	WarmthZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(MeshComponent);
	FireLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	FireLight->SetIntensity(5000.f);
	FireLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
	FireLight->SetAttenuationRadius(WarmthRadius);
	FireLight->SetCastShadows(false);
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();

	// Reflect the canonical radius onto the visualiser sphere (BP-level edits
	// to WarmthRadius win over whatever was set on the sphere in the editor).
	if (WarmthZone)
	{
		WarmthZone->SetSphereRadius(WarmthRadius);
	}

	ActiveCampfires.Add(this);
}

void ACampfireActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveCampfires.RemoveAllSwap([this](const TWeakObjectPtr<ACampfireActor>& Fire)
	{
		return !Fire.IsValid() || Fire.Get() == this;
	});

	Super::EndPlay(EndPlayReason);
}

bool ACampfireActor::IsWarming(const FVector& Location) const
{
	if (!bIsActive)
	{
		return false;
	}
	return FVector::DistSquared(GetActorLocation(), Location) <= (WarmthRadius * WarmthRadius);
}

bool ACampfireActor::IsLocationWarmed(const UObject* WorldContext, const FVector& Location)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	for (const TWeakObjectPtr<ACampfireActor>& Fire : ActiveCampfires)
	{
		const ACampfireActor* Campfire = Fire.Get();
		if (!Campfire || !Campfire->bIsActive)
		{
			continue;
		}
		// Skip campfires from a different PIE world / map.
		if (World && Campfire->GetWorld() != World)
		{
			continue;
		}
		if (Campfire->IsWarming(Location))
		{
			return true;
		}
	}
	return false;
}
