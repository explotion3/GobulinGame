#include "Combat/WeaponComponent.h"

#include "Combat/FPSWeaponAnimator.h"
#include "Core/Damageable.h"
#include "Data/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::SetWeaponDefinition(UWeaponDefinition* InDefinition)
{
	WeaponDefinition = InDefinition;
}

bool UWeaponComponent::TryFire()
{
	if (!WeaponDefinition || !GetWorld())
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float FireInterval = WeaponDefinition->FireRate > 0.0f ? 1.0f / WeaponDefinition->FireRate : 0.1f;
	if (Now < NextFireTime)
	{
		return false;
	}
	NextFireTime = Now + FireInterval;

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetCameraView(ViewLocation, ViewRotation))
	{
		return false;
	}

	// 散布：在视角方向上叠加随机锥角
	const float SpreadRadians = FMath::DegreesToRadians(FMath::Max(0.0f, WeaponDefinition->Spread));
	ViewRotation.Pitch += FMath::FRandRange(-SpreadRadians, SpreadRadians);
	ViewRotation.Yaw += FMath::FRandRange(-SpreadRadians, SpreadRadians);

	const FVector End = ViewLocation + ViewRotation.Vector() * MaxTraceDistance;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, QueryParams))
	{
		if (IDamageable* Damageable = Cast<IDamageable>(Hit.GetActor()))
		{
			FDamageInfo DamageInfo;
			DamageInfo.Amount = GetDamageAtDistance(Hit.Distance);
			DamageInfo.Instigator = GetOwner();
			DamageInfo.DamageSourceId = WeaponDefinition->WeaponId;
			Damageable->Execute_TakeDamage(Hit.GetActor(), DamageInfo);
		}
	}

	// 第一人称武器表现反馈（若角色已挂 Animator）
	if (UFPSWeaponAnimator* Animator = GetOwner()->FindComponentByClass<UFPSWeaponAnimator>())
	{
		Animator->TriggerFire();
	}

	return true;
}

float UWeaponComponent::GetDamageAtDistance(float Distance) const
{
	if (!WeaponDefinition)
	{
		return 0.0f;
	}

	const float Start = WeaponDefinition->FalloffStartDistance;
	const float End = WeaponDefinition->FalloffEndDistance;
	if (End <= 0.0f || Distance <= Start)
	{
		return WeaponDefinition->BaseDamage;
	}

	const float FalloffFactor = FMath::GetMappedRangeValueClamped(
		FVector2D(Start, End),
		FVector2D(1.0f, 0.6f),
		Distance);
	return WeaponDefinition->BaseDamage * FalloffFactor;
}

bool UWeaponComponent::GetCameraView(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PlayerController->PlayerCameraManager)
			{
				PlayerController->PlayerCameraManager->GetCameraViewPoint(OutLocation, OutRotation);
				return true;
			}
		}
	}

	if (const UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>())
	{
		OutLocation = Camera->GetComponentLocation();
		OutRotation = Camera->GetComponentRotation();
		return true;
	}

	return false;
}
