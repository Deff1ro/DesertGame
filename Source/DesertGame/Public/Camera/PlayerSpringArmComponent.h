// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerSpringArmComponent.generated.h"

// Spring-arm that ignores transient/dynamic actors (enemies, item drops, resource nodes)
// when running its camera collision probe. This prevents the camera from snapping
// inward — and revealing the inside of the player mesh — when an enemy attacks at
// melee range or when a resource drop falls through the camera lane.
//
// Works regardless of Blueprint-level collision overrides on those actors.
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UPlayerSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UPlayerSpringArmComponent();

protected:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;
};
