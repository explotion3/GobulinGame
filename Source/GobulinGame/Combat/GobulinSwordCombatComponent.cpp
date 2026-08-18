#include "Combat/GobulinSwordCombatComponent.h"

#include "CollisionQueryParams.h"
#include "Combat/CombatantRegistrySubsystem.h"
#include "Combat/CombatEventSubsystem.h"
#include "Combat/CombatSubsystem.h"
#include "Combat/DamageProtocol.h"
#include "Combat/GobulinWeaponViewComponent.h"
#include "Core/CombatTags.h"
#include "Core/GobulinCollisionChannels.h"
#include "Data/GobulinSwordDefinition.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGobulinSword, Log, All);

UGobulinSwordCombatComponent::UGobulinSwordCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGobulinSwordCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponView)
	{
		WeaponView->SetSwordDefinition(SwordDefinition);
	}

	if (!SwordDefinition)
	{
		UE_LOG(LogGobulinSword, Warning, TEXT("SwordCombat on %s has no SwordDefinition."), *GetNameSafe(GetOwner()));
	}

	if (!SwordTip)
	{
		UE_LOG(LogGobulinSword, Warning, TEXT("SwordCombat on %s has no SwordTip component; attacks will have no melee trace."), *GetNameSafe(GetOwner()));
	}

	if (UCombatEventSubsystem* Events = GetWorld() ? GetWorld()->GetSubsystem<UCombatEventSubsystem>() : nullptr)
	{
		DamageResolvedDelegateHandle = Events->OnDamageResolved().AddUObject(
			this,
			&UGobulinSwordCombatComponent::HandleDamageResolved);
	}
}

void UGobulinSwordCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UCombatEventSubsystem* Events = GetWorld() ? GetWorld()->GetSubsystem<UCombatEventSubsystem>() : nullptr)
	{
		Events->OnDamageResolved().Remove(DamageResolvedDelegateHandle);
	}
	PendingDamageCommands.Reset();
	Super::EndPlay(EndPlayReason);
}

void UGobulinSwordCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AttackState == EGobulinSwordAttackState::Idle || !SwordDefinition)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (AttackState == EGobulinSwordAttackState::Attacking)
	{
		const float Duration = FMath::Max(SwordDefinition->AttackDuration, KINDA_SMALL_NUMBER);
		const float PreviousNormalizedTime = FMath::Clamp(AttackElapsed / Duration, 0.0f, 1.0f);
		AttackElapsed = FMath::Min(AttackElapsed + DeltaTime, Duration);
		const float CurrentNormalizedTime = FMath::Clamp(AttackElapsed / Duration, 0.0f, 1.0f);

		if (WeaponView)
		{
			WeaponView->SetAttackNormalizedTime(CurrentNormalizedTime);
		}

		if (!bSwingSoundPlayed && CurrentNormalizedTime >= FMath::Clamp(SwordDefinition->SwingSoundNormalizedTime, 0.0f, 1.0f))
		{
			bSwingSoundPlayed = true;
			NotifySwingTriggered();
		}

		if (SwordTip)
		{
			const FVector CurrentSwordTipLocation = SwordTip->GetComponentLocation();
			const float HitWindowStart = FMath::Clamp(SwordDefinition->HitWindowStartNormalizedTime, 0.0f, 1.0f);
			const float HitWindowEnd = FMath::Max(HitWindowStart, FMath::Clamp(SwordDefinition->HitWindowEndNormalizedTime, 0.0f, 1.0f));

			if (bHasPreviousSwordTipLocation && CurrentNormalizedTime >= HitWindowStart && PreviousNormalizedTime <= HitWindowEnd)
			{
				FVector TraceStart = PreviousSwordTipLocation;
				FVector TraceEnd = CurrentSwordTipLocation;

				// Clip a large frame step to the authored damage window so the trace
				// does not damage targets before the sword becomes active or after it ends.
				if (PreviousNormalizedTime < HitWindowStart && WeaponView)
				{
					WeaponView->SetAttackNormalizedTime(HitWindowStart);
					TraceStart = SwordTip->GetComponentLocation();
					WeaponView->SetAttackNormalizedTime(CurrentNormalizedTime);
				}

				if (CurrentNormalizedTime > HitWindowEnd && WeaponView)
				{
					WeaponView->SetAttackNormalizedTime(HitWindowEnd);
					TraceEnd = SwordTip->GetComponentLocation();
					WeaponView->SetAttackNormalizedTime(CurrentNormalizedTime);
				}

				ProcessSwordTipTrace(TraceStart, TraceEnd);
			}

			PreviousSwordTipLocation = CurrentSwordTipLocation;
			bHasPreviousSwordTipLocation = true;
		}

		if (!bAttackInputBufferOpen && CurrentNormalizedTime >= FMath::Clamp(SwordDefinition->InputBufferOpenNormalizedTime, 0.0f, 1.0f))
		{
			bAttackInputBufferOpen = true;
		}

		if (AttackElapsed >= Duration)
		{
			BeginRecovery();
		}
	}
	else if (AttackState == EGobulinSwordAttackState::Recovery)
	{
		RecoveryElapsed += DeltaTime;
		if (RecoveryElapsed >= FMath::Max(0.0f, SwordDefinition->RecoveryDuration))
		{
			CompleteRecovery();
		}
	}
}

void UGobulinSwordCombatComponent::SetWeaponView(UGobulinWeaponViewComponent* InWeaponView)
{
	WeaponView = InWeaponView;
	if (WeaponView)
	{
		WeaponView->SetSwordDefinition(SwordDefinition);
	}
}

void UGobulinSwordCombatComponent::SetSwordTip(USceneComponent* InSwordTip)
{
	SwordTip = InSwordTip;
	bHasPreviousSwordTipLocation = false;
}

bool UGobulinSwordCombatComponent::RequestAttack()
{
	if (!SwordDefinition || !GetWorld())
	{
		return false;
	}

	if (AttackState == EGobulinSwordAttackState::Idle)
	{
		return StartAttackInternal();
	}

	if (SwordDefinition->bCanBufferAttack && bAttackInputBufferOpen)
	{
		bAttackInputBuffered = true;
		return true;
	}

	return false;
}

bool UGobulinSwordCombatComponent::StartAttack()
{
	return RequestAttack();
}

bool UGobulinSwordCombatComponent::StartAttackInternal()
{
	if (AttackState != EGobulinSwordAttackState::Idle || !SwordDefinition || !GetWorld())
	{
		return false;
	}

	bAttackInputBuffered = false;
	bAttackInputBufferOpen = SwordDefinition->InputBufferOpenNormalizedTime <= 0.0f;
	bSwingSoundPlayed = false;
	AttackElapsed = 0.0f;
	RecoveryElapsed = 0.0f;
	ResetAttackTraceState();
	SetAttackState(EGobulinSwordAttackState::Attacking);

	if (WeaponView)
	{
		WeaponView->BeginAttackPose();
		WeaponView->SetAttackNormalizedTime(0.0f);
	}

	SetComponentTickEnabled(true);
	return true;
}

void UGobulinSwordCombatComponent::CancelAttack()
{
	SetAttackState(EGobulinSwordAttackState::Idle);
	bAttackInputBuffered = false;
	bAttackInputHeld = false;
	bAttackInputBufferOpen = false;
	bSwingSoundPlayed = false;
	AttackElapsed = 0.0f;
	RecoveryElapsed = 0.0f;
	ResetAttackTraceState();
	SetComponentTickEnabled(false);

	if (WeaponView)
	{
		WeaponView->ResetPose();
	}
}

void UGobulinSwordCombatComponent::BeginRecovery()
{
	AttackElapsed = 0.0f;
	RecoveryElapsed = 0.0f;
	bAttackInputBufferOpen = true;
	bHasPreviousSwordTipLocation = false;

	if (WeaponView)
	{
		WeaponView->ResetPose();
	}

	SetAttackState(EGobulinSwordAttackState::Recovery);

	if (SwordDefinition && SwordDefinition->RecoveryDuration <= KINDA_SMALL_NUMBER)
	{
		CompleteRecovery();
	}
}

void UGobulinSwordCombatComponent::CompleteRecovery()
{
	RecoveryElapsed = 0.0f;

	const bool bShouldStartBufferedAttack = bAttackInputBuffered && SwordDefinition && SwordDefinition->bCanBufferAttack;
	const bool bShouldStartHeldAttack = bAttackInputHeld && SwordDefinition && SwordDefinition->bCanAutoRepeatAttack;
	if (bShouldStartBufferedAttack || bShouldStartHeldAttack)
	{
		bAttackInputBuffered = false;
		SetAttackState(EGobulinSwordAttackState::Idle);
		StartAttackInternal();
		return;
	}

	bAttackInputBuffered = false;
	bAttackInputBufferOpen = false;
	bSwingSoundPlayed = false;
	ResetAttackTraceState();
	SetAttackState(EGobulinSwordAttackState::Idle);
	SetComponentTickEnabled(false);
}

void UGobulinSwordCombatComponent::SetAttackState(EGobulinSwordAttackState NewState)
{
	if (AttackState == NewState)
	{
		return;
	}

	const EGobulinSwordAttackState PreviousState = AttackState;
	AttackState = NewState;
	OnAttackStateChanged.Broadcast(PreviousState, NewState);
}

void UGobulinSwordCombatComponent::ProcessSwordTipTrace(const FVector& Start, const FVector& End)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld() || !SwordDefinition || SwordDefinition->TipTraceRadius <= 0.0f)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GobulinSwordTipTrace), false);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		GobulinCollision::CombatTrace,
		FCollisionShape::MakeSphere(SwordDefinition->TipTraceRadius),
		QueryParams);

	Hits.Sort([](const FHitResult& Left, const FHitResult& Right)
	{
		return Left.Time < Right.Time;
	});

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == OwnerActor)
		{
			continue;
		}

		UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>();
		UCombatSubsystem* Combat = GetWorld()->GetSubsystem<UCombatSubsystem>();
		const FCombatantHandle Target = Registry ? Registry->FindHandleForActor(HitActor) : FCombatantHandle();
		if (Target.IsSet() && Combat)
		{
			if (!DamagedTargetsThisAttack.Contains(Target))
			{
				DamagedTargetsThisAttack.Add(Target);

				FCombatDamageRequest Request;
				Request.Source = Registry->FindHandleForActor(OwnerActor);
				Request.Target = Target;
				Request.BaseAmount = SwordDefinition->BaseDamage;
				Request.AttackTag = CombatTag_Attack_Melee;
				Request.DamageType = CombatTag_Damage_Physical;
				Request.HitLocation = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.Location;
				Request.HitNormal = Hit.bBlockingHit ? Hit.ImpactNormal : FVector::UpVector;
				FVector KnockbackDirection = (HitActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
				if (KnockbackDirection.IsNearlyZero())
				{
					KnockbackDirection = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
				}
				Request.Impulse = KnockbackDirection * SwordDefinition->HitKnockbackSpeed
					+ FVector::UpVector * SwordDefinition->HitLaunchSpeed;
				PendingDamageCommands.Add(Combat->SubmitDamage(MoveTemp(Request)));
			}
		}
		else if (Hit.bBlockingHit)
		{
			break;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (SwordDefinition->bDebugDrawTipTrace)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.1f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), Start, SwordDefinition->TipTraceRadius, 8, FColor::Yellow, false, 0.1f);
		DrawDebugSphere(GetWorld(), End, SwordDefinition->TipTraceRadius, 8, FColor::Green, false, 0.1f);
	}
#endif
}

void UGobulinSwordCombatComponent::HandleDamageResolved(const FCombatDamageResolvedEvent& Event)
{
	if (!PendingDamageCommands.Remove(Event.Result.CommandId))
	{
		return;
	}

	if (Event.Result.DidApplyDamage())
	{
		NotifyHitConfirmed(
			Event.Result.Target,
			Event.Request.HitLocation,
			Event.Request.HitNormal,
			Event.Result);
	}
}

void UGobulinSwordCombatComponent::NotifySwingTriggered()
{
	OnSwingTriggered.Broadcast();
}

void UGobulinSwordCombatComponent::NotifyHitConfirmed(
	FCombatantHandle Target,
	const FVector& HitLocation,
	const FVector& HitNormal,
	const FCombatDamageResult& DamageResult)
{
	OnHitConfirmed.Broadcast(Target, HitLocation, HitNormal, DamageResult.AppliedAmount, DamageResult.bKilled);
}

void UGobulinSwordCombatComponent::ResetAttackTraceState()
{
	PreviousSwordTipLocation = FVector::ZeroVector;
	bHasPreviousSwordTipLocation = false;
	DamagedTargetsThisAttack.Reset();
}
