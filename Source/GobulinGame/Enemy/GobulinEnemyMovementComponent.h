#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GobulinEnemyMovementComponent.generated.h"

/**
 * Actor 敌人的地面移动执行器。
 * 基础怪潮只认脚底中心附近的地面支撑，避免胶囊下半球在锐利边缘悬挂或沿边滑动。
 */
UCLASS()
class GOBULINGAME_API UGobulinEnemyMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** 使用世界厘米配置脚底中心支撑范围；应在应用最终胶囊尺寸后调用。 */
	void ConfigureGroundSupport(float InSupportRadius, float InSnapDownHeight);

	bool HasGroundSupportSample() const { return bHasGroundSupportSample; }
	bool HasCenterGroundSupport() const { return bHasCenterGroundSupport; }
	float GetGroundSupportRadius() const { return GroundSupportRadius; }
	float GetGroundSnapDownHeight() const { return GroundSnapDownHeight; }
	FVector GetGroundSupportLocation() const { return GroundSupportLocation; }

protected:
	virtual bool ShouldCatchAir(
		const FFindFloorResult& OldFloor,
		const FFindFloorResult& NewFloor) override;
	virtual void OnMovementModeChanged(
		EMovementMode PreviousMovementMode,
		uint8 PreviousCustomMode) override;

private:
	/** 脚底中心用于确认地面支撑的小胶囊半径，单位为厘米。 */
	float GroundSupportRadius = 3.0f;

	/** 仍可作为连续地面的最大向下吸附距离，单位为厘米。 */
	float GroundSnapDownHeight = 8.0f;

	bool bHasGroundSupportSample = false;
	bool bHasCenterGroundSupport = true;
	FVector GroundSupportLocation = FVector::ZeroVector;
};
