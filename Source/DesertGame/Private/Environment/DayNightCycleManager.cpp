// Fill out your copyright notice in the Description page of Project Settings.

#include "Environment/DayNightCycleManager.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Core/DesertGameInstance.h"
#include "ProtagonistCharacter/ProtagonistCharacter.h"

DEFINE_LOG_CATEGORY(LogDayNight);

ADayNightCycleManager::ADayNightCycleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADayNightCycleManager::BeginPlay()
{
	Super::BeginPlay();

	ElapsedInPhase = 0.f;
	EnterPhase(StartingPhase, /*bForceBroadcast*/ true);
}

void ADayNightCycleManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedInPhase += DeltaSeconds;

	const float Duration = (CurrentPhase == EDayNightPhase::Day) ? DayDurationSeconds : NightDurationSeconds;
	if (Duration <= 0.f)
	{
		return;
	}

	if (ElapsedInPhase >= Duration)
	{
		// Wrap over into the opposite phase
		ElapsedInPhase -= Duration;
		EnterPhase(CurrentPhase == EDayNightPhase::Day ? EDayNightPhase::Night : EDayNightPhase::Day, /*bForceBroadcast*/ false);
	}

	UpdateSunRotation();
}

void ADayNightCycleManager::EnterPhase(EDayNightPhase NewPhase, bool bForceBroadcast)
{
	const bool bChanged = (NewPhase != CurrentPhase) || bForceBroadcast;
	CurrentPhase = NewPhase;

	UpdateSunRotation();

	if (bChanged)
	{
		UE_LOG(LogDayNight, Log, TEXT("DayNightCycle: entered %s"),
			NewPhase == EDayNightPhase::Day ? TEXT("Day") : TEXT("Night"));
		PlayPhaseMusic();
		OnPhaseChanged.Broadcast(CurrentPhase);

		// A full day = wrapping back to StartingPhase after at least one real
		// transition. bForceBroadcast is the initial BeginPlay entry — exclude
		// that so we don't "save day 0" at level start.
		if (!bForceBroadcast && NewPhase == StartingPhase)
		{
			++CompletedDayCount;
			UE_LOG(LogDayNight, Log, TEXT("DayNightCycle: full cycle completed (day %d)"), CompletedDayCount);
			OnFullDayCompleted.Broadcast(CompletedDayCount);

			if (bAutoSaveOnFullCycle)
			{
				TriggerAutoSave();
			}
		}
	}
}

void ADayNightCycleManager::TriggerAutoSave()
{
	UDesertGameInstance* GI = GetGameInstance<UDesertGameInstance>();
	if (!GI)
	{
		UE_LOG(LogDayNight, Warning, TEXT("AutoSave skipped: no UDesertGameInstance"));
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AProtagonistCharacter* Player = Cast<AProtagonistCharacter>(PlayerPawn);
	if (!Player)
	{
		UE_LOG(LogDayNight, Warning, TEXT("AutoSave skipped: player pawn is not AProtagonistCharacter"));
		return;
	}

	const bool bOk = GI->SaveGame(Player);
	UE_LOG(LogDayNight, Log, TEXT("AutoSave on day %d: %s"),
		CompletedDayCount, bOk ? TEXT("OK") : TEXT("FAILED"));
}

void ADayNightCycleManager::PlayPhaseMusic()
{
	USoundBase* TargetMusic = (CurrentPhase == EDayNightPhase::Day) ? DayMusic.Get() : NightMusic.Get();

	// Fade out the previous track if any.
	if (ActiveMusicComponent)
	{
		ActiveMusicComponent->FadeOut(MusicFadeSeconds, 0.f);
		ActiveMusicComponent = nullptr;
	}

	if (TargetMusic)
	{
		ActiveMusicComponent = UGameplayStatics::SpawnSound2D(this, TargetMusic);
		if (ActiveMusicComponent && MusicFadeSeconds > 0.f)
		{
			ActiveMusicComponent->FadeIn(MusicFadeSeconds);
		}
	}
}

float ADayNightCycleManager::GetPhaseProgress() const
{
	const float Duration = (CurrentPhase == EDayNightPhase::Day) ? DayDurationSeconds : NightDurationSeconds;
	if (Duration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ElapsedInPhase / Duration, 0.f, 1.f);
}

void ADayNightCycleManager::UpdateSunRotation() const
{
	if (!SunLight)
	{
		return;
	}

	const float Progress = GetPhaseProgress();

	// Day: sun arcs from sunrise pitch up through zenith and down to sunset pitch.
	// Night: sun continues the same direction below the horizon (sunset to a point
	// 360° further in pitch, which brings it back to sunrise rotation).
	FRotator Result;
	if (CurrentPhase == EDayNightPhase::Day)
	{
		Result = FMath::Lerp(SunriseRotation, SunsetRotation, Progress);
	}
	else
	{
		// Continue past sunset through the underside of the world and back to sunrise.
		// SunriseRotation.Pitch - 360 keeps the pitch monotonically decreasing.
		const FRotator NightStart = SunsetRotation;
		const FRotator NightEnd(SunriseRotation.Pitch - 360.f, SunriseRotation.Yaw, SunriseRotation.Roll);
		Result = FMath::Lerp(NightStart, NightEnd, Progress);
	}

	SunLight->SetActorRotation(Result);
}

ADayNightCycleManager* ADayNightCycleManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ADayNightCycleManager> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
