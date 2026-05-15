// Fill out your copyright notice in the Description page of Project Settings.

#include "Attributes/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
	CurrentHunger = MaxHunger;
	CurrentThirst = MaxThirst;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnHungerChanged.Broadcast(CurrentHunger, MaxHunger);
	OnThirstChanged.Broadcast(CurrentThirst, MaxThirst);
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Hunger / thirst drain — only while alive. Skip when zeroed to avoid spamming events.
	if (IsAlive())
	{
		if (HungerDrainPerSecond > 0.f && CurrentHunger > 0.f)
		{
			CurrentHunger = FMath::Max(0.f, CurrentHunger - HungerDrainPerSecond * HungerMultiplier * DeltaTime);
			OnHungerChanged.Broadcast(CurrentHunger, MaxHunger);
		}
		if (ThirstDrainPerSecond > 0.f && CurrentThirst > 0.f)
		{
			CurrentThirst = FMath::Max(0.f, CurrentThirst - ThirstDrainPerSecond * ThirstMultiplier * DeltaTime);
			OnThirstChanged.Broadcast(CurrentThirst, MaxThirst);
		}
	}

	if (bIsDrainingStamina)
	{
		CurrentStamina -= StaminaDrainRate * DeltaTime;
		if (CurrentStamina <= 0.0f)
		{
			CurrentStamina = 0.0f;
			bIsDrainingStamina = false;
			OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
			OnStaminaDepleted.Broadcast();
			return;
		}
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}
	else
	{
		TimeSinceLastStaminaDrain += DeltaTime;
		if (TimeSinceLastStaminaDrain >= StaminaRegenDelay && CurrentStamina < MaxStamina)
		{
			CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + StaminaRegenRate * DeltaTime);
			OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
		}
	}
}

// ============================================================
// Percent getters
// ============================================================

float UAttributeComponent::GetHealthPercent() const
{
	return (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
}

float UAttributeComponent::GetStaminaPercent() const
{
	return (MaxStamina > 0.0f) ? (CurrentStamina / MaxStamina) : 0.0f;
}

float UAttributeComponent::GetHungerPercent() const
{
	return (MaxHunger > 0.0f) ? (CurrentHunger / MaxHunger) : 0.0f;
}

float UAttributeComponent::GetThirstPercent() const
{
	return (MaxThirst > 0.0f) ? (CurrentThirst / MaxThirst) : 0.0f;
}

// ============================================================
// Health
// ============================================================

void UAttributeComponent::ApplyDamage(float Damage)
{
	if (!IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast();
	}
}

void UAttributeComponent::Heal(float Amount)
{
	if (!IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

// ============================================================
// Stamina
// ============================================================

void UAttributeComponent::SetSprintingState(bool bSprinting)
{
	if (bSprinting && CurrentStamina > 0.0f)
	{
		bIsDrainingStamina = true;
		TimeSinceLastStaminaDrain = 0.0f;
	}
	else
	{
		bIsDrainingStamina = false;
		TimeSinceLastStaminaDrain = 0.0f;
	}
}

bool UAttributeComponent::CanStartSprint() const
{
	return CurrentStamina >= StaminaMinToSprint;
}

// ============================================================
// Hunger / Thirst stubs
// ============================================================

void UAttributeComponent::ConsumeHunger(float Amount)
{
	CurrentHunger = FMath::Max(0.0f, CurrentHunger - Amount);
	OnHungerChanged.Broadcast(CurrentHunger, MaxHunger);
}

void UAttributeComponent::RestoreHunger(float Amount)
{
	CurrentHunger = FMath::Min(MaxHunger, CurrentHunger + Amount);
	OnHungerChanged.Broadcast(CurrentHunger, MaxHunger);
}

void UAttributeComponent::ConsumeThirst(float Amount)
{
	CurrentThirst = FMath::Max(0.0f, CurrentThirst - Amount);
	OnThirstChanged.Broadcast(CurrentThirst, MaxThirst);
}

void UAttributeComponent::RestoreThirst(float Amount)
{
	CurrentThirst = FMath::Min(MaxThirst, CurrentThirst + Amount);
	OnThirstChanged.Broadcast(CurrentThirst, MaxThirst);
}

void UAttributeComponent::SetEnvironmentMultipliers(float InThirstMultiplier, float InHungerMultiplier)
{
	ThirstMultiplier = FMath::Max(0.f, InThirstMultiplier);
	HungerMultiplier = FMath::Max(0.f, InHungerMultiplier);
}
