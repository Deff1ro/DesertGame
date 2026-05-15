// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemActor.generated.h"

class UItemDataAsset;
class UStaticMeshComponent;
class USphereComponent;
class UPrimitiveComponent;
class AProtagonistCharacter;

UCLASS()
class DESERTGAME_API AItemActor : public AActor
{
	GENERATED_BODY()

public:
	AItemActor();

	// Call immediately after SpawnActor to set data for runtime-spawned drops.
	// Must be called before BeginPlay (or at the very start of BeginPlay at latest).
	void Initialize(UItemDataAsset* InData, int32 InQuantity);

	// Enables physics simulation on the mesh and applies an impulse. Used by AResourceNode
	// when spawning drops so they fall and scatter naturally.
	UFUNCTION(BlueprintCallable, Category = "Item")
	void LaunchAsDrop(const FVector& Impulse);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void TryPickup(AProtagonistCharacter* Picker);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnPickupZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	// Sound played when the player picks this actor up. If you want per-item-type
	// sounds, set this on the BP that spawns; if you want one global pickup chime,
	// keep this empty and rely on a HUD/inventory-level sound instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Audio")
	TObjectPtr<class USoundBase> PickupSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupZone;
};
