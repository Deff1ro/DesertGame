// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CampfireActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPointLightComponent;

// World-placed campfire. While alive (bIsActive) it warms anyone within its
// WarmthRadius — the player's cold-suffering check skips them, so they take
// no cold HP drain at night.
//
// Not craftable. Just drag BP_Campfire onto the level.
UCLASS()
class DESERTGAME_API ACampfireActor : public AActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

	// ============================================================
	// Components
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// The actual warmth zone — game logic uses its radius. Marked
	// BlueprintReadOnly so BP can read radius but the canonical value to tweak
	// is WarmthRadius below (we mirror it onto the sphere in BeginPlay).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> WarmthZone;

	// Optional cosmetic light. Tweak intensity/colour in BP.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FireLight;

	// ============================================================
	// Tunables (BP)
	// ============================================================

	// Radius (cm) of the warmth zone. Players inside this radius skip the
	// cold-damage check at night.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire", meta = (ClampMin = "0.0"))
	float WarmthRadius = 500.f;

	// If false the campfire is "out" — it still exists in the world but no
	// longer warms anyone. Toggle from BP if you want a burn-out timer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire")
	bool bIsActive = true;

	// ============================================================
	// Global query (used by the player's environment tick)
	// ============================================================

	// Returns true if the given location falls inside any active campfire's
	// warmth radius. World argument is used to verify the campfire belongs to
	// the same world (multi-PIE safety).
	static bool IsLocationWarmed(const UObject* WorldContext, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "Campfire")
	bool IsWarming(const FVector& Location) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// All campfires currently registered. Weak so a destroyed campfire
	// silently drops out without leaving a dangling pointer.
	static TArray<TWeakObjectPtr<ACampfireActor>> ActiveCampfires;
};
