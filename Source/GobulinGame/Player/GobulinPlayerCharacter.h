#pragma once

#include "CoreMinimal.h"
#include "Core/Damageable.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "GobulinPlayerCharacter.generated.h"

class UBattleAttributeComponent;
class UCameraComponent;
class UInputAction;
class UPlayerCombatComponent;
class UFPSWeaponOverlayComponent;
struct FInputActionValue;

/**
 * 正式玩家角色：只负责角色本体、移动、相机、属性和输入路由。
 * 武器与弹药由 UPlayerCombatComponent 管理。
 */
UCLASS()
class GOBULINGAME_API AGobulinPlayerCharacter : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	AGobulinPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//~Begin IDamageable interface
	virtual void TakeDamage_Implementation(const FDamageInfo& DamageInfo) override;
	//~End IDamageable interface

	/** 兼容 UGameplayStatics::ApplyDamage 等经典伤害入口。 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Player")
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UFPSWeaponOverlayComponent* GetFirstPersonWeaponOverlay() const { return FirstPersonWeaponOverlay; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UBattleAttributeComponent* GetBattleAttributeComponent() const { return Attributes; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UPlayerCombatComponent* GetPlayerCombatComponent() const { return Combat; }

	UFUNCTION(BlueprintPure, Category = "Player")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintCallable, Category = "Player|Movement")
	void SetSprinting(bool bInSprinting);

protected:
	/** 直接挂在胶囊体根节点上，不依赖骨骼或 AnimBP。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** 仅本地玩家可见的 2D 武器层。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSWeaponOverlayComponent> FirstPersonWeaponOverlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBattleAttributeComponent> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerCombatComponent> Combat;

	// ---------- Enhanced Input ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MeleeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SwitchWeaponAction;

	// ---------- Movement ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeedMultiplier = 1.35f;

	bool bSprinting = false;
	bool bDead = false;

	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);
	void JumpStarted();
	void JumpCompleted();
	void SprintStarted();
	void SprintCompleted();
	void FireStarted();
	void FireCompleted();
	void AimStarted();
	void AimCompleted();
	void ReloadStarted();
	void MeleeStarted();
	void MeleeCompleted();
	void SwitchWeaponStarted();

	void ApplyMovementSettings();
	void Die();

	UFUNCTION()
	void OnAttributeChanged(FGameplayTag AttributeTag, float NewValue);
};
