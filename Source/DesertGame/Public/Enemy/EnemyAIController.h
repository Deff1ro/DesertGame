// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
struct FAIStimulus;

UCLASS()
class DESERTGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	// How long (seconds) to keep investigating LastKnownLocation after losing
	// the player before giving up and returning to patrol.
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.5"))
	float InvestigationTimeout = 6.f;

	// Combat music that starts when the enemy aggros and stops when it loses
	// the player. Plays as a separate 2D track on top of the day/night ambient.
	// (Per-species spot-player sound is configured on the enemy pawn itself —
	// see AEnemyCharacter::SpotPlayerSound.)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Audio")
	TObjectPtr<class USoundBase> CombatMusic;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void OnInvestigationTimeout();

	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	FTimerHandle InvestigationTimerHandle;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> CombatMusicComponent;

	// Tick-driven "I can feel you breathing on me" check: if the player is
	// within ProximityRadius of the pawn, treat them as spotted regardless of
	// facing direction. Set ProximityRadius to 0 to disable.
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.0"))
	float ProximityRadius = 500.f;

	void TickProximitySense();
};
