#include "Combat/PlayerCombatComponent.h"

#include "Combat/FPSWeaponOverlayComponent.h"
#include "Core/Damageable.h"
#include "Data/GameTypes.h"
#include "Data/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"

UPlayerCombatComponent::UPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadDefaultWeaponDefinitions();
	InitializeWeaponStates();

	if (WeaponStates.Num() > 0)
	{
		CurrentWeaponIndex = 0;
		EquipCurrentWeapon();
	}
}

void UPlayerCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFireHeld)
	{
		TryFire();
	}
}

void UPlayerCombatComponent::SetWeaponOverlay(UFPSWeaponOverlayComponent* InOverlay)
{
	WeaponOverlay = InOverlay;
	ApplyCurrentWeaponToOverlay();
}

UWeaponDefinition* UPlayerCombatComponent::GetCurrentWeaponDefinition() const
{
	if (!WeaponStates.IsValidIndex(CurrentWeaponIndex))
	{
		return nullptr;
	}

	return WeaponStates[CurrentWeaponIndex].Definition;
}

int32 UPlayerCombatComponent::GetCurrentAmmo() const
{
	return WeaponStates.IsValidIndex(CurrentWeaponIndex) ? WeaponStates[CurrentWeaponIndex].CurrentAmmo : 0;
}

int32 UPlayerCombatComponent::GetReserveAmmo() const
{
	return WeaponStates.IsValidIndex(CurrentWeaponIndex) ? WeaponStates[CurrentWeaponIndex].ReserveAmmo : 0;
}

void UPlayerCombatComponent::StartFire()
{
	bFireHeld = true;
	TryFire();
}

void UPlayerCombatComponent::StopFire()
{
	bFireHeld = false;
}

void UPlayerCombatComponent::SetAiming(bool bInAiming)
{
	bAiming = bInAiming;

	if (WeaponOverlay)
	{
		WeaponOverlay->SetAiming(bAiming);
	}
}

void UPlayerCombatComponent::Reload()
{
	if (!WeaponStates.IsValidIndex(CurrentWeaponIndex))
	{
		return;
	}

	const UWeaponDefinition* Definition = WeaponStates[CurrentWeaponIndex].Definition;
	if (!Definition || Definition->AmmoType == EAmmoType::None || Definition->AmmoType == EAmmoType::Mana)
	{
		return;
	}

	const int32 MagazineSize = GetMagazineSize(Definition);
	const int32 MissingAmmo = FMath::Max(0, MagazineSize - WeaponStates[CurrentWeaponIndex].CurrentAmmo);
	const int32 ReloadAmount = FMath::Min(MissingAmmo, WeaponStates[CurrentWeaponIndex].ReserveAmmo);
	if (ReloadAmount <= 0)
	{
		return;
	}

	WeaponStates[CurrentWeaponIndex].CurrentAmmo += ReloadAmount;
	WeaponStates[CurrentWeaponIndex].ReserveAmmo -= ReloadAmount;
	BroadcastAmmoChanged();

	if (WeaponOverlay)
	{
		WeaponOverlay->PlayClip(TEXT("Reload"));
	}
}

void UPlayerCombatComponent::StartMelee()
{
	for (int32 Index = 0; Index < WeaponStates.Num(); ++Index)
	{
		const UWeaponDefinition* Definition = WeaponStates[Index].Definition;
		if (Definition && Definition->WeaponType == EWeaponType::Melee)
		{
			if (CurrentWeaponIndex != Index)
			{
				CurrentWeaponIndex = Index;
				EquipCurrentWeapon();
			}
			StartFire();
			return;
		}
	}
}

void UPlayerCombatComponent::SwitchWeapon(int32 Direction)
{
	if (WeaponStates.Num() < 2)
	{
		return;
	}

	StopFire();

	const int32 Step = Direction < 0 ? -1 : 1;
	CurrentWeaponIndex = (CurrentWeaponIndex + Step + WeaponStates.Num()) % WeaponStates.Num();
	NextFireTime = 0.0f;
	EquipCurrentWeapon();
}

bool UPlayerCombatComponent::TryFire()
{
	if (!GetWorld() || !WeaponStates.IsValidIndex(CurrentWeaponIndex))
	{
		return false;
	}

	UWeaponDefinition* Definition = WeaponStates[CurrentWeaponIndex].Definition;
	if (!Definition)
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float FireInterval = Definition->FireRate > 0.0f ? 1.0f / Definition->FireRate : 0.1f;
	if (Now < NextFireTime)
	{
		return false;
	}

	const int32 AmmoCost = GetAmmoCost(Definition);
	if (AmmoCost > 0 && WeaponStates[CurrentWeaponIndex].CurrentAmmo < AmmoCost)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetCameraView(ViewLocation, ViewRotation))
	{
		return false;
	}

	NextFireTime = Now + FireInterval;
	if (AmmoCost > 0)
	{
		WeaponStates[CurrentWeaponIndex].CurrentAmmo -= AmmoCost;
		BroadcastAmmoChanged();
	}

	const float SpreadDegrees = FMath::Max(0.0f, Definition->Spread);
	ViewRotation.Pitch += FMath::FRandRange(-SpreadDegrees, SpreadDegrees);
	ViewRotation.Yaw += FMath::FRandRange(-SpreadDegrees, SpreadDegrees);

	const float TraceDistance = Definition->WeaponType == EWeaponType::Melee ? 250.0f : 10000.0f;
	const FVector End = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponTrace), true);
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Visibility, QueryParams))
	{
		if (IDamageable* Damageable = Cast<IDamageable>(Hit.GetActor()))
		{
			FDamageInfo DamageInfo;
			DamageInfo.Amount = GetDamageAtDistance(Definition, Hit.Distance);
			DamageInfo.Instigator = GetOwner();
			DamageInfo.DamageSourceId = Definition->WeaponId;
			IDamageable::Execute_TakeDamage(Hit.GetActor(), DamageInfo);
		}
		else if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyDamage(
				HitActor,
				GetDamageAtDistance(Definition, Hit.Distance),
				Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr,
				GetOwner(),
				UDamageType::StaticClass());
		}
	}

	if (WeaponOverlay)
	{
		WeaponOverlay->PlayClip(TEXT("Fire"));
		WeaponOverlay->AddRecoil();
	}

	return true;
}

void UPlayerCombatComponent::LoadDefaultWeaponDefinitions()
{
	if (WeaponDefinitions.Num() > 0)
	{
		return;
	}

	for (const TCHAR* WeaponPath : {
		TEXT("/Game/Data/Weapons/W01.W01"),
		TEXT("/Game/Data/Weapons/W03.W03"),
		TEXT("/Game/Data/Weapons/W05.W05")
	})
	{
		if (UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, WeaponPath))
		{
			WeaponDefinitions.Add(Definition);
		}
	}
}

void UPlayerCombatComponent::InitializeWeaponStates()
{
	WeaponStates.Reset();

	for (UWeaponDefinition* Definition : WeaponDefinitions)
	{
		if (!Definition)
		{
			continue;
		}

		FPlayerWeaponRuntimeState& State = WeaponStates.AddDefaulted_GetRef();
		State.Definition = Definition;
		State.CurrentAmmo = GetMagazineSize(Definition);
		State.ReserveAmmo = GetInitialReserveAmmo(Definition);
	}
}

void UPlayerCombatComponent::EquipCurrentWeapon()
{
	UWeaponDefinition* Definition = GetCurrentWeaponDefinition();
	if (!Definition)
	{
		return;
	}

	ApplyCurrentWeaponToOverlay();
	OnWeaponChanged.Broadcast(Definition->WeaponId, CurrentWeaponIndex);
	BroadcastAmmoChanged();
}

void UPlayerCombatComponent::ApplyCurrentWeaponToOverlay()
{
	if (!WeaponOverlay)
	{
		return;
	}

	const UWeaponDefinition* Definition = GetCurrentWeaponDefinition();
	if (!Definition)
	{
		return;
	}

	WeaponOverlay->SetSpriteMaterial(Definition->FirstPersonSpriteMaterial);
	WeaponOverlay->SetFlipbook(Definition->FirstPersonFlipbook);
	WeaponOverlay->PlayClip(TEXT("Idle"));
	WeaponOverlay->SetAiming(bAiming);
}

int32 UPlayerCombatComponent::GetMagazineSize(const UWeaponDefinition* Definition) const
{
	if (!Definition)
	{
		return 0;
	}

	switch (Definition->AmmoType)
	{
	case EAmmoType::Arrows:
		return 6;
	case EAmmoType::Mana:
		return 100;
	case EAmmoType::None:
		return 0;
	default:
		return 1;
	}
}

int32 UPlayerCombatComponent::GetInitialReserveAmmo(const UWeaponDefinition* Definition) const
{
	return Definition && Definition->AmmoType == EAmmoType::Arrows ? 24 : 0;
}

int32 UPlayerCombatComponent::GetAmmoCost(const UWeaponDefinition* Definition) const
{
	if (!Definition)
	{
		return 0;
	}

	return Definition->AmmoType == EAmmoType::Mana ? 8 : (Definition->AmmoType == EAmmoType::None ? 0 : 1);
}

float UPlayerCombatComponent::GetDamageAtDistance(const UWeaponDefinition* Definition, float Distance) const
{
	if (!Definition)
	{
		return 0.0f;
	}

	if (Definition->FalloffEndDistance <= 0.0f || Distance <= Definition->FalloffStartDistance)
	{
		return Definition->BaseDamage;
	}

	const float FalloffFactor = FMath::GetMappedRangeValueClamped(
		FVector2D(Definition->FalloffStartDistance, Definition->FalloffEndDistance),
		FVector2D(1.0f, 0.6f),
		Distance);
	return Definition->BaseDamage * FalloffFactor;
}

bool UPlayerCombatComponent::GetCameraView(FVector& OutLocation, FRotator& OutRotation) const
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

void UPlayerCombatComponent::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(GetCurrentAmmo(), GetReserveAmmo());
}
