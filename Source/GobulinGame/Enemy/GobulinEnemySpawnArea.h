#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemySpawnProtocol.h"
#include "GameFramework/Actor.h"
#include "GobulinEnemySpawnArea.generated.h"

class UBoxComponent;
class UCombatEventSubsystem;
class UGobulinEnemyArchetype;
struct FEnemySpawnResolvedEvent;

UENUM(BlueprintType)
enum class EEnemySpawnAreaSubmissionResult : uint8
{
	Submitted,
	PartiallySubmitted,
	InvalidConfiguration,
	NoValidLocation,
	SubsystemUnavailable
};

/** 一次区域生成的同步提交结果；敌人实例的异步结果通过 OnEnemySpawnResolved 返回。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemySpawnAreaSubmission
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	EEnemySpawnAreaSubmissionResult Result = EEnemySpawnAreaSubmissionResult::InvalidConfiguration;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	int32 RequestedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	int32 SubmittedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	int32 RejectedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	int32 SpawnGroupId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn Area")
	TArray<FCombatCommandId> CommandIds;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FEnemySpawnAreaSubmittedSignature,
	const FEnemySpawnAreaSubmission&,
	Submission);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FEnemySpawnAreaResolvedSignature,
	const FEnemySpawnResultData&,
	Result);

/**
 * 可直接放入关卡的敌人批量生成区域。
 * 区域只计算地面锚点并提交协议请求，不持有具体 Actor，也不会创建子 SpawnPoint。
 */
UCLASS(Blueprintable, ClassGroup = (Enemy), meta = (DisplayName = "Gobulin Enemy Spawn Area"))
class GOBULINGAME_API AGobulinEnemySpawnArea : public AActor
{
	GENERATED_BODY()

public:
	AGobulinEnemySpawnArea();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 按 SpawnCount 生成默认批次。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn Area")
	FEnemySpawnAreaSubmission SpawnDefaultBatch();

	/** 生成指定数量的敌人；每次显式调用都表示新建一个批次。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn Area", meta = (ClampMin = "1"))
	FEnemySpawnAreaSubmission SpawnEnemies(int32 Count);

	/** 仅计算下一批可用的地面锚点，不提交敌人。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Spawn Area", meta = (ClampMin = "1"))
	TArray<FTransform> GenerateSpawnTransforms(int32 Count) const;

	/** 在编辑器中重新扫描区域内的地面并保存本地候选点。 */
	UFUNCTION(CallInEditor, Category = "Enemy|Spawn Area|Editor")
	void RebuildSpawnCandidates();

	/** 在视口中短暂显示当前缓存候选点。 */
	UFUNCTION(CallInEditor, Category = "Enemy|Spawn Area|Editor")
	void PreviewSpawnCandidates();

	/** 清除已保存的候选点；PIE 时将改为运行时采样。 */
	UFUNCTION(CallInEditor, Category = "Enemy|Spawn Area|Editor")
	void ClearSpawnCandidates();

	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Area")
	UBoxComponent* GetSpawnBounds() const { return SpawnBounds; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Spawn Area")
	int32 GetPendingSpawnCount() const { return PendingCommandIds.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Spawn Area|Events")
	FEnemySpawnAreaSubmittedSignature OnSpawnBatchSubmitted;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Spawn Area|Events")
	FEnemySpawnAreaResolvedSignature OnEnemySpawnResolved;

private:
	TArray<FTransform> GenerateSpawnTransformsInternal(int32 Count, int32 BatchSequence) const;
	bool TryResolveGroundCandidate(
		const FVector2D& LocalXY,
		FTransform& OutGroundTransform,
		bool bCheckClearance) const;
	bool IsFarEnoughFromAccepted(
		const FVector& Location,
		const TArray<FTransform>& AcceptedTransforms) const;
	bool GetLocalSamplingRange(float& OutMinX, float& OutMaxX, float& OutMinY, float& OutMaxY) const;
	void BuildCandidateCache(TArray<FTransform>& OutLocalCandidates) const;
	void FinalizeSubmission(FEnemySpawnAreaSubmission& Submission);
	void HandleEnemySpawnResolved(const FEnemySpawnResolvedEvent& Event);
	void BindCombatEvents();
	void UnbindCombatEvents();
	float GetEffectiveMinimumSpacing() const;
	void GetPlacementCapsule(float& OutRadius, float& OutHalfHeight) const;

	/** 区域尺寸和 Transform；每个区域应只覆盖一个楼层。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> SpawnBounds;

	/** 该区域生成的敌人定义；定义自身较轻，其纸片和材质仍通过 Asset Bundle 异步加载。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGobulinEnemyArchetype> EnemyArchetype;

	/** 进入游戏时是否自动提交一个默认批次。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (AllowPrivateAccess = "true"))
	bool bSpawnOnBeginPlay = true;

	/** 默认批次数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (ClampMin = "1", ClampMax = "4096", AllowPrivateAccess = "true"))
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (AllowPrivateAccess = "true"))
	FCombatantHandle SpawnOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (AllowPrivateAccess = "true"))
	uint8 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (ClampMin = "1", AllowPrivateAccess = "true"))
	int32 EnemyLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float PowerScale = 1.0f;

	/** 为 -1 时，每次生成自动申请新的组 ID；非负值用于外部波次指定固定分组。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Enemy", meta = (ClampMin = "-1", AllowPrivateAccess = "true"))
	int32 SpawnGroupId = INDEX_NONE;

	/** 相同区域和种子会产生可重复的候选分布。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (AllowPrivateAccess = "true"))
	int32 RandomSeed = 0;

	/** 敌人地面锚点的最小水平间距，实际值不会小于敌人胶囊直径。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
	float MinimumSpacing = 120.0f;

	/** 候选点与 Box XY 边缘的留白。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
	float EdgePadding = 30.0f;

	/** 允许生成的最大地面坡度。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (ClampMin = "0.0", ClampMax = "89.0", Units = "deg", AllowPrivateAccess = "true"))
	float MaximumGroundSlope = 45.0f;

	/** 开启后，地面点还必须能投射到当前 NavMesh。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Navigation", meta = (AllowPrivateAccess = "true"))
	bool bRequireNavigation = true;

	/** NavMesh 投射搜索范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Navigation", meta = (EditCondition = "bRequireNavigation", Units = "cm", AllowPrivateAccess = "true"))
	FVector NavigationProjectionExtent = FVector(50.0f, 50.0f, 200.0f);

	/** 是否允许 WorldDynamic 物体作为地面；默认只接受 WorldStatic。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (AllowPrivateAccess = "true"))
	bool bAllowDynamicGround = false;

	/** 运行时和编辑器重建时是否检查敌人胶囊空间。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (AllowPrivateAccess = "true"))
	bool bCheckSpawnClearance = true;

	/** 防止超大区域意外保存过多候选数据；不限制单批运行时补充采样。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Placement", meta = (ClampMin = "1", ClampMax = "4096", AllowPrivateAccess = "true"))
	int32 MaximumCachedCandidateCount = 512;

	/** 相对 SpawnBounds 保存的候选地面 Transform；不会创建额外 Actor。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn Area|Runtime", meta = (AllowPrivateAccess = "true"))
	TArray<FTransform> CachedLocalCandidates;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Spawn Area|Runtime", meta = (AllowPrivateAccess = "true"))
	FEnemySpawnAreaSubmission LastSubmission;

	TSet<FCombatCommandId> PendingCommandIds;
	FDelegateHandle SpawnResolvedDelegateHandle;
	int32 RuntimeBatchSequence = 0;

	static constexpr int32 MaxSpawnCountPerBatch = 4096;
};
