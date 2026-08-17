#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GobulinSwordCombatComponent.generated.h"

class UGobulinSwordDefinition;
class UGobulinWeaponViewComponent;
class USceneComponent;
class AActor;
struct FDamageResult;

UENUM(BlueprintType)
enum class EGobulinSwordAttackState : uint8
{
	Idle,
	Attacking,
	Recovery
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGobulinSwordAttackStateChangedSignature,
	EGobulinSwordAttackState, PreviousState,
	EGobulinSwordAttackState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FGobulinSwordSwingTriggeredSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FGobulinSwordHitConfirmedSignature,
	AActor*, HitActor,
	FVector, HitLocation,
	FVector, HitNormal,
	float, AppliedDamage,
	bool, bKilled);

/** 管理单次挥剑状态、命中检测与伤害派发。 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UGobulinSwordCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGobulinSwordCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword")
	TObjectPtr<UGobulinSwordDefinition> SwordDefinition;

	UFUNCTION(BlueprintCallable, Category = "Sword")
	void SetWeaponView(UGobulinWeaponViewComponent* InWeaponView);

	UFUNCTION(BlueprintCallable, Category = "Sword")
	void SetSwordTip(USceneComponent* InSwordTip);

	UFUNCTION(BlueprintPure, Category = "Sword")
	UGobulinSwordDefinition* GetSwordDefinition() const { return SwordDefinition; }

	/** 请求一次攻击；输入缓存窗口开启后，会保存一个待执行的攻击请求。 */
	UFUNCTION(BlueprintCallable, Category = "Sword")
	bool RequestAttack();

	/** 兼容现有蓝图或 C++ 调用方的攻击入口。 */
	UFUNCTION(BlueprintCallable, Category = "Sword")
	bool StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Sword")
	void CancelAttack();

	/** 设置攻击键是否处于按住状态；按住时会在当前攻击结束后自动衔接下一次攻击。 */
	UFUNCTION(BlueprintCallable, Category = "Sword")
	void SetAttackInputHeld(bool bHeld) { bAttackInputHeld = bHeld; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	bool IsAttackInputHeld() const { return bAttackInputHeld; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	bool IsAttacking() const { return AttackState == EGobulinSwordAttackState::Attacking; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	bool IsBusy() const { return AttackState != EGobulinSwordAttackState::Idle; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	EGobulinSwordAttackState GetAttackState() const { return AttackState; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	bool IsAttackInputBuffered() const { return bAttackInputBuffered; }

	UFUNCTION(BlueprintPure, Category = "Sword")
	bool IsAttackInputBufferOpen() const { return bAttackInputBufferOpen; }

	/** 剑在待机、攻击和收招状态之间切换时广播。 */
	UPROPERTY(BlueprintAssignable, Category = "Sword|Events")
	FGobulinSwordAttackStateChangedSignature OnAttackStateChanged;

	/** 攻击时间轴到达挥剑音效节点时广播一次。 */
	UPROPERTY(BlueprintAssignable, Category = "Sword|Events")
	FGobulinSwordSwingTriggeredSignature OnSwingTriggered;

	/** 每个目标实际受到剑伤害后广播一次。 */
	UPROPERTY(BlueprintAssignable, Category = "Sword|Events")
	FGobulinSwordHitConfirmedSignature OnHitConfirmed;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UGobulinWeaponViewComponent> WeaponView;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SwordTip;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|State")
	EGobulinSwordAttackState AttackState = EGobulinSwordAttackState::Idle;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|State")
	bool bAttackInputBuffered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|State")
	bool bAttackInputHeld = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|State")
	bool bAttackInputBufferOpen = false;

	float AttackElapsed = 0.0f;
	float RecoveryElapsed = 0.0f;
	FVector PreviousSwordTipLocation = FVector::ZeroVector;
	bool bHasPreviousSwordTipLocation = false;
	bool bSwingSoundPlayed = false;
	TSet<TWeakObjectPtr<AActor>> DamagedActorsThisAttack;

	bool StartAttackInternal();
	void BeginRecovery();
	void CompleteRecovery();
	void SetAttackState(EGobulinSwordAttackState NewState);
	void ProcessSwordTipTrace(const FVector& Start, const FVector& End);
	void NotifySwingTriggered();
	void NotifyHitConfirmed(AActor* HitActor, const FHitResult& Hit, const FDamageResult& DamageResult);
	void ResetAttackTraceState();
};
