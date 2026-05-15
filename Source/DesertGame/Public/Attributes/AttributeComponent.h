// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttributeDepleted);

UCLASS(ClassGroup = (Attributes), meta = (BlueprintSpawnableComponent))
class DESERTGAME_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// Health
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Attributes|Health")
	float CurrentHealth = 100.0f;

	// ============================================================
	// Stamina
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Stamina", meta = (ClampMin = "1.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Attributes|Stamina")
	float CurrentStamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Stamina")
	float StaminaDrainRate = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Stamina")
	float StaminaRegenRate = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Stamina")
	float StaminaRegenDelay = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Stamina")
	float StaminaMinToSprint = 10.0f;

	// ============================================================
	// Hunger
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Hunger", meta = (ClampMin = "1.0"))
	float MaxHunger = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Attributes|Hunger")
	float CurrentHunger = 100.0f;

	// Units of hunger drained per second. Default 100/300s = ~0.333 → 100% over 5 minutes.
	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Hunger", meta = (ClampMin = "0.0"))
	float HungerDrainPerSecond = 0.333f;

	// ============================================================
	// Thirst
	// ============================================================

	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Thirst", meta = (ClampMin = "1.0"))
	float MaxThirst = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Attributes|Thirst")
	float CurrentThirst = 100.0f;

	// Units of thirst drained per second. Default 100/180s = ~0.555 → 100% over 3 minutes.
	UPROPERTY(EditDefaultsOnly, Category = "Attributes|Thirst", meta = (ClampMin = "0.0"))
	float ThirstDrainPerSecond = 0.555f;

	// ============================================================
	// Events
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChanged OnHungerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChanged OnThirstChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeDepleted OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeDepleted OnStaminaDepleted;

	// ============================================================
	// Getters
	// ============================================================

	UFUNCTION(BlueprintPure, Category = "Attributes|Health")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Attributes|Stamina")
	float GetStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Attributes|Hunger")
	float GetHunger() const { return CurrentHunger; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Hunger")
	float GetMaxHunger() const { return MaxHunger; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Hunger")
	float GetHungerPercent() const;

	UFUNCTION(BlueprintPure, Category = "Attributes|Thirst")
	float GetThirst() const { return CurrentThirst; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Thirst")
	float GetMaxThirst() const { return MaxThirst; }

	UFUNCTION(BlueprintPure, Category = "Attributes|Thirst")
	float GetThirstPercent() const;

	// ============================================================
	// Health API
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Attributes|Health")
	void ApplyDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = "Attributes|Health")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	// ============================================================
	// Stamina API
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Attributes|Stamina")
	void SetSprintingState(bool bSprinting);

	UFUNCTION(BlueprintPure, Category = "Attributes|Stamina")
	bool CanStartSprint() const;

	// ============================================================
	// Hunger / Thirst API (stubs for future systems)
	// ============================================================

	UFUNCTION(BlueprintCallable, Category = "Attributes|Hunger")
	void ConsumeHunger(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Hunger")
	void RestoreHunger(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Thirst")
	void ConsumeThirst(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Attributes|Thirst")
	void RestoreThirst(float Amount);

	// External (e.g. day/night) multipliers applied to drain this frame. Set by
	// the player every tick; defaults to 1.0 when no environment is driving it.
	UFUNCTION(BlueprintCallable, Category = "Attributes|Environment")
	void SetEnvironmentMultipliers(float InThirstMultiplier, float InHungerMultiplier);

private:
	float ThirstMultiplier = 1.f;
	float HungerMultiplier = 1.f;
	bool bIsDrainingStamina = false;
	float TimeSinceLastStaminaDrain = 0.0f;
};
