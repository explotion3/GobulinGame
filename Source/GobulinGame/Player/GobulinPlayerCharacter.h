#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantEndpoint.h"
#include "Combat/GobulinSwordCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "GobulinPlayerCharacter.generated.h"

class UBattleAttributeComponent;
class UCameraComponent;
class UInputAction;
class UArrowComponent;
class UStaticMeshComponent;
class UGobulinCameraFeedbackComponent;
class UGobulinSwordFeedbackComponent;
class UGobulinWeaponViewComponent;
struct FInputActionValue;

/**
 * 正式玩家角色：负责角色本体、移动、相机、属性和输入路由。
 * 第一把剑的攻击逻辑由 UGobulinSwordCombatComponent 管理。
 */
UCLASS()
class GOBULINGAME_API AGobulinPlayerCharacter : public ACharacter, public ICombatantEndpoint
{
	GENERATED_BODY()

public:
	AGobulinPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//~Begin ICombatantEndpoint interface
	virtual FCombatDamageResult ResolveCombatDamage_Implementation(const FCombatDamageRequest& Request) override;
	//~End ICombatantEndpoint interface

	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	FCombatantHandle GetCombatantHandle() const { return CombatantHandle; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UGobulinWeaponViewComponent* GetFirstPersonWeaponView() const { return FirstPersonWeaponView; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UBattleAttributeComponent* GetBattleAttributeComponent() const { return Attributes; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UGobulinSwordCombatComponent* GetSwordCombatComponent() const { return SwordCombat; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UGobulinSwordFeedbackComponent* GetSwordFeedbackComponent() const { return SwordFeedback; }

	UFUNCTION(BlueprintPure, Category = "Player")
	UGobulinCameraFeedbackComponent* GetCameraFeedbackComponent() const { return CameraFeedback; }

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
	TObjectPtr<UGobulinWeaponViewComponent> FirstPersonWeaponView;

	/** 实际显示剑贴图的 Plane 网格；相对偏移用于将剑柄对齐到视图支点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FirstPersonWeaponVisual;

	/** 可编辑的剑尖标记，用作剑尖轨迹检测的世界空间起点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FirstPersonSwordTip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBattleAttributeComponent> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGobulinCameraFeedbackComponent> CameraFeedback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGobulinSwordCombatComponent> SwordCombat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGobulinSwordFeedbackComponent> SwordFeedback;

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
	TObjectPtr<UInputAction> SwordAttackAction;

	// ---------- Movement ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeedMultiplier = 1.35f;

	/** 剑处于挥砍阶段时的移动速度倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword", meta = (ClampMin = "0.0"))
	float SwordAttackMoveSpeedMultiplier = 0.85f;

	/** 剑处于收招阶段时的移动速度倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword", meta = (ClampMin = "0.0"))
	float SwordRecoveryMoveSpeedMultiplier = 1.0f;

	/** 挥砍阶段是否允许跳跃输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword")
	bool bAllowJumpDuringSwordAttack = false;

	/** 收招阶段是否允许跳跃输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword")
	bool bAllowJumpDuringSwordRecovery = false;

	/** 挥砍阶段是否允许冲刺输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword")
	bool bAllowSprintDuringSwordAttack = false;

	/** 收招阶段是否允许冲刺输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword")
	bool bAllowSprintDuringSwordRecovery = false;

	/** 剑攻击真正开始时是否立即取消冲刺。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sword")
	bool bCancelSprintOnSwordAttack = true;

	bool bSprinting = false;
	bool bDead = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	FCombatantHandle CombatantHandle;

	/** 玩家作为战斗目标注册时使用的队伍；默认与 SpawnArea 的敌人队伍 0 敌对。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	uint8 CombatTeamId = 1;

	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);
	void JumpStarted();
	void JumpCompleted();
	void SprintStarted();
	void SprintCompleted();
	void SwordAttackStarted();
	void SwordAttackCompleted();

	void ApplyMovementSettings();
	void Die();

	bool CanJumpDuringSwordState() const;
	bool CanSprintDuringSwordState() const;

	UFUNCTION()
	void OnSwordAttackStateChanged(EGobulinSwordAttackState PreviousState, EGobulinSwordAttackState NewState);

	UFUNCTION()
	void OnAttributeChanged(FGameplayTag AttributeTag, float NewValue);
};
