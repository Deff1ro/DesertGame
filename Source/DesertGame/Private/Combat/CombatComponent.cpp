// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombat, Log, All);

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Default 4-hit combo section names
	ComboSectionNames = { TEXT("Attack1"), TEXT("Attack2"), TEXT("Attack3"), TEXT("Attack4") };
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache AnimInstance and bind montage end delegate
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			CachedAnimInstance = Mesh->GetAnimInstance();
			if (CachedAnimInstance)
			{
				CachedAnimInstance->OnMontageEnded.AddDynamic(this, &UCombatComponent::OnMontageEnded);
			}
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DodgeSlideTimeRemaining <= 0.f)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		const float FrameMove = DodgeSlideSpeed * DeltaTime;
		Owner->AddActorWorldOffset(DodgeSlideDirection * FrameMove, true);
		DodgeSlideTimeRemaining -= DeltaTime;

		if (DodgeSlideTimeRemaining <= 0.f)
		{
			DodgeSlideTimeRemaining = 0.f;
			SetComponentTickEnabled(false);
		}
	}
}

// ============================================================
// Public API
// ============================================================

void UCombatComponent::RequestAttack()
{
	if (!ComboMontage || !CachedAnimInstance || ComboSectionNames.Num() == 0)
	{
		return;
	}

	if (CombatState == ECombatState::Blocking || CombatState == ECombatState::Dodging)
	{
		return;
	}

	if (CombatState == ECombatState::Idle)
	{
		// Start the combo from the first section
		CombatState = ECombatState::Attacking;
		ComboIndex = 0;
		bComboWindowOpen = false;
		bPendingNextCombo = false;

		CachedAnimInstance->Montage_Play(ComboMontage);
		CachedAnimInstance->Montage_JumpToSection(ComboSectionNames[0], ComboMontage);

		UE_LOG(LogCombat, Log, TEXT("Combo started: %s"), *ComboSectionNames[0].ToString());
	}
	else if (CombatState == ECombatState::Attacking && bComboWindowOpen)
	{
		// Queue next combo hit
		bPendingNextCombo = true;
		UE_LOG(LogCombat, Log, TEXT("Combo input saved (window open, index=%d)"), ComboIndex);
	}
}

void UCombatComponent::OpenComboWindow()
{
	bComboWindowOpen = true;
	UE_LOG(LogCombat, Log, TEXT("Combo window OPENED (index=%d)"), ComboIndex);
}

void UCombatComponent::CloseComboWindow()
{
	bComboWindowOpen = false;

	if (bPendingNextCombo)
	{
		bPendingNextCombo = false;

		const int32 NextIndex = ComboIndex + 1;
		if (NextIndex < ComboSectionNames.Num())
		{
			ComboIndex = NextIndex;
			CachedAnimInstance->Montage_JumpToSection(ComboSectionNames[ComboIndex], ComboMontage);
			UE_LOG(LogCombat, Log, TEXT("Combo advanced: %s (index=%d)"), *ComboSectionNames[ComboIndex].ToString(), ComboIndex);
		}
		else
		{
			// Reached end of combo chain — let the last animation finish
			UE_LOG(LogCombat, Log, TEXT("Combo chain complete, letting last attack finish"));
		}
	}
	else
	{
		// No input during window — animation plays out to idle on its own
		UE_LOG(LogCombat, Log, TEXT("Combo window CLOSED, no input — returning to idle"));
	}
}

// ============================================================
// Block
// ============================================================

void UCombatComponent::StartBlock()
{
	if (!CachedAnimInstance || !BlockMontage)
	{
		return;
	}

	if (CombatState == ECombatState::Dodging)
	{
		return;
	}

	InterruptCurrentState();

	CombatState = ECombatState::Blocking;
	CachedAnimInstance->Montage_Play(BlockMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f);
	UE_LOG(LogCombat, Log, TEXT("Block started"));
}

void UCombatComponent::StopBlock()
{
	if (CombatState != ECombatState::Blocking)
	{
		return;
	}

	if (CachedAnimInstance && BlockMontage)
	{
		CachedAnimInstance->Montage_Stop(0.1f, BlockMontage);
	}

	CombatState = ECombatState::Idle;
	UE_LOG(LogCombat, Log, TEXT("Block stopped"));
}

bool UCombatComponent::IsBlocking() const
{
	return CombatState == ECombatState::Blocking;
}

// ============================================================
// Dodge
// ============================================================

void UCombatComponent::RequestDodge(EDodgeDirection Direction)
{
	if (!CachedAnimInstance)
	{
		return;
	}

	const TObjectPtr<UAnimMontage>* MontagePtr = DodgeMontages.Find(Direction);
	if (!MontagePtr || !*MontagePtr)
	{
		return;
	}

	// Dodge overrides everything except another dodge
	if (CombatState == ECombatState::Dodging)
	{
		return;
	}

	InterruptCurrentState();

	ActiveDodgeMontage = *MontagePtr;
	CombatState = ECombatState::Dodging;
	CachedAnimInstance->Montage_Play(ActiveDodgeMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f);

	// Start extra slide on top of root motion
	if (const AActor* Owner = GetOwner())
	{
		const FRotator Rot = Owner->GetActorRotation();

		switch (Direction)
		{
		case EDodgeDirection::Forward:  DodgeSlideDirection = Rot.Vector(); break;
		case EDodgeDirection::Backward: DodgeSlideDirection = -Rot.Vector(); break;
		case EDodgeDirection::Left:     DodgeSlideDirection = -FRotationMatrix(Rot).GetUnitAxis(EAxis::Y); break;
		case EDodgeDirection::Right:    DodgeSlideDirection = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y); break;
		}

		DodgeSlideDirection.Z = 0.f;
		DodgeSlideDirection.Normalize();
		DodgeSlideTimeRemaining = DodgeSlideDuration;
		DodgeSlideSpeed = DodgeExtraDistance / DodgeSlideDuration;
		SetComponentTickEnabled(true);
	}

	UE_LOG(LogCombat, Log, TEXT("Dodge: %d"), static_cast<uint8>(Direction));
}

bool UCombatComponent::IsDodging() const
{
	return CombatState == ECombatState::Dodging;
}

// ============================================================
// Internal
// ============================================================

void UCombatComponent::InterruptCurrentState()
{
	switch (CombatState)
	{
	case ECombatState::Attacking:
		ResetCombo();
		break;
	case ECombatState::Blocking:
		if (BlockMontage)
		{
			CachedAnimInstance->Montage_Stop(0.f, BlockMontage);
		}
		CombatState = ECombatState::Idle;
		break;
	default:
		break;
	}
}

void UCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ComboMontage)
	{
		UE_LOG(LogCombat, Log, TEXT("Combo montage ended (interrupted=%d)"), bInterrupted);
		// Only reset to Idle if we're still in Attacking state —
		// if we transitioned to Blocking, don't overwrite that
		if (CombatState == ECombatState::Attacking)
		{
			ResetCombo();
		}
		else
		{
			// Just clear combo internals without touching CombatState
			ComboIndex = 0;
			bComboWindowOpen = false;
			bPendingNextCombo = false;
		}
	}
	else if (Montage == BlockMontage && CombatState == ECombatState::Blocking)
	{
		CombatState = ECombatState::Idle;
	}
	else if (Montage == ActiveDodgeMontage && CombatState == ECombatState::Dodging)
	{
		UE_LOG(LogCombat, Log, TEXT("Dodge montage ended"));
		ActiveDodgeMontage = nullptr;
		CombatState = ECombatState::Idle;
	}
}

void UCombatComponent::ResetCombo()
{
	CombatState = ECombatState::Idle;
	ComboIndex = 0;
	bComboWindowOpen = false;
	bPendingNextCombo = false;
}
