// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceNode.generated.h"

class UStaticMeshComponent;
class UResourceNodeData;
class UItemDataAsset;
class AItemActor;

DECLARE_LOG_CATEGORY_EXTERN(LogDesertResource, Log, All);

UCLASS()
class DESERTGAME_API AResourceNode : public AActor
{
	GENERATED_BODY()

public:
	AResourceNode();

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void ApplyHit(UItemDataAsset* UsedTool, int32 Damage);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<UResourceNodeData> NodeData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Класс для спавна дропа. По умолчанию — базовый AItemActor.
	UPROPERTY(EditDefaultsOnly, Category = "Resource|Drop")
	TSubclassOf<AItemActor> DropActorClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resource")
	int32 CurrentHealth = 0;

private:
	void SpawnDrops();
};
