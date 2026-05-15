// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycleManager.generated.h"

class ADirectionalLight;

DECLARE_LOG_CATEGORY_EXTERN(LogDayNight, Log, All);

UENUM(BlueprintType)
enum class EDayNightPhase : uint8
{
	Day   UMETA(DisplayName = "Day"),
	Night UMETA(DisplayName = "Night")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayNightPhaseChanged, EDayNightPhase, NewPhase);

// Drives the day/night cycle. Rotates an assigned directional light, broadcasts
// phase transitions, and exposes the gameplay multipliers used by the player's
// attribute & survival systems. Place one of these on the map.
UCLASS()
class DESERTGAME_API ADayNightCycleManager : public AActor
{
	GENERATED_BODY()

public:
	ADayNightCycleManager();

	virtual void Tick(float DeltaSeconds) override;

	// ============================================================
	// Cycle configuration
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "1.0"))
	float DayDurationSeconds = 600.f;   // 10 minutes

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "1.0"))
	float NightDurationSeconds = 300.f; // 5 minutes

	// Which phase the cycle starts in when the level loads.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	EDayNightPhase StartingPhase = EDayNightPhase::Day;

	// Directional light treated as the sun/moon. Its pitch is rotated linearly
	// from SunriseRotation to SunsetRotation across the day, and back again
	// (through nadir) across the night.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle|Sun")
	TObjectPtr<ADirectionalLight> SunLight;

	// Sun rotation at the start of day (sunrise, just above horizon — east).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle|Sun")
	FRotator SunriseRotation = FRotator(-10.f, 0.f, 0.f);

	// Sun rotation at the end of day (sunset, just above horizon — west).
	// Pitch goes through -90 (zenith / noon) on its way from sunrise to sunset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle|Sun")
	FRotator SunsetRotation = FRotator(-170.f, 0.f, 0.f);

	// ============================================================
	// Gameplay multipliers (used by AttributeComponent and friends)
	// ============================================================

	// Day-time multiplier on thirst drain. 1.5 = thirst drops 50% faster than base.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Day", meta = (ClampMin = "1.0"))
	float DayThirstMultiplier = 1.5f;

	// Night-time multiplier on hunger drain.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Night", meta = (ClampMin = "1.0"))
	float NightHungerMultiplier = 1.5f;

	// HP loss per second when the player has no Heat-protection (cloak) during day.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Day", meta = (ClampMin = "0.0"))
	float DayUnprotectedHealthDrainPerSecond = 2.f;

	// HP loss per second when the player has no Cold-protection (jacket) during night.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay|Night", meta = (ClampMin = "0.0"))
	float NightUnprotectedHealthDrainPerSecond = 2.f;

	// Ambient music for the day phase. Loops; cross-fades with NightMusic on phase change.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> DayMusic;

	// Ambient music for the night phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<class USoundBase> NightMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0"))
	float MusicFadeSeconds = 1.5f;

	// ============================================================
	// Events
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Cycle|Events")
	FOnDayNightPhaseChanged OnPhaseChanged;

	// ============================================================
	// Getters
	// ============================================================

	UFUNCTION(BlueprintPure, Category = "Cycle")
	EDayNightPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	bool IsDay() const { return CurrentPhase == EDayNightPhase::Day; }

	UFUNCTION(BlueprintPure, Category = "Cycle")
	bool IsNight() const { return CurrentPhase == EDayNightPhase::Night; }

	// Normalized [0..1] progress through the current phase.
	UFUNCTION(BlueprintPure, Category = "Cycle")
	float GetPhaseProgress() const;

	// Convenience: finds the manager actor in the current world. Returns nullptr
	// if no manager has been placed.
	UFUNCTION(BlueprintPure, Category = "Cycle", meta = (WorldContext = "WorldContextObject"))
	static ADayNightCycleManager* Get(const UObject* WorldContextObject);

protected:
	virtual void BeginPlay() override;

private:
	void EnterPhase(EDayNightPhase NewPhase, bool bForceBroadcast);
	void UpdateSunRotation() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Cycle|State")
	EDayNightPhase CurrentPhase = EDayNightPhase::Day;

	UPROPERTY(VisibleInstanceOnly, Category = "Cycle|State")
	float ElapsedInPhase = 0.f;

	UPROPERTY()
	TObjectPtr<class UAudioComponent> ActiveMusicComponent;

	void PlayPhaseMusic();
};
