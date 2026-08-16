#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerCombatComponent.generated.h"

class UWeaponDefinition;
class UFPSWeaponOverlayComponent;

/** 单个武器槽位的运行时状态；WeaponDefinition 本身只保存静态配置。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FPlayerWeaponRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 ReserveAmmo = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerWeaponChangedSignature, FName, WeaponId, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerAmmoChangedSignature, int32, CurrentAmmo, int32, ReserveAmmo);

/**
 * 玩家战斗组件：管理 W01/W03/W05 武器槽位、开火、弹药、换弹和本地 2D 武器层。
 * 当前 M1 先走单机/本地命中路径，后续再把开火请求迁移到服务器 RPC。
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 蓝图可以覆盖；为空时自动加载 /Game/Data/Weapons/W01、W03、W05。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<TObjectPtr<UWeaponDefinition>> WeaponDefinitions;

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FPlayerWeaponChangedSignature OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapons")
	FPlayerAmmoChangedSignature OnAmmoChanged;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SetWeaponOverlay(UFPSWeaponOverlayComponent* InOverlay);

	UFUNCTION(BlueprintPure, Category = "Weapons")
	UWeaponDefinition* GetCurrentWeaponDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetCurrentWeaponIndex() const { return CurrentWeaponIndex; }

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	int32 GetReserveAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SetAiming(bool bInAiming);

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void StartMelee();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SwitchWeapon(int32 Direction = 1);

	/** 立即执行一次当前武器的攻击。 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool TryFire();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<FPlayerWeaponRuntimeState> WeaponStates;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	int32 CurrentWeaponIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UFPSWeaponOverlayComponent> WeaponOverlay;

	bool bFireHeld = false;
	bool bAiming = false;
	float NextFireTime = 0.0f;

	void LoadDefaultWeaponDefinitions();
	void InitializeWeaponStates();
	void EquipCurrentWeapon();
	void ApplyCurrentWeaponToOverlay();

	int32 GetMagazineSize(const UWeaponDefinition* Definition) const;
	int32 GetInitialReserveAmmo(const UWeaponDefinition* Definition) const;
	int32 GetAmmoCost(const UWeaponDefinition* Definition) const;
	float GetDamageAtDistance(const UWeaponDefinition* Definition, float Distance) const;

	bool GetCameraView(FVector& OutLocation, FRotator& OutRotation) const;
	void BroadcastAmmoChanged();
};
