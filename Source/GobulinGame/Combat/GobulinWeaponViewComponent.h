#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "GobulinWeaponViewComponent.generated.h"

class UGobulinSwordDefinition;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * First-person weapon pivot. This intentionally keeps the historical
 * UStaticMeshComponent base so existing Blueprint component templates remain
 * loadable. The component itself is hidden; the actual Plane mesh is a child
 * component and this transform is used as the handle pivot.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UGobulinWeaponViewComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UGobulinWeaponViewComponent();

	/** Assigns the child component that renders the weapon Plane. */
	void SetVisualMesh(UStaticMeshComponent* InVisualMesh);

	UFUNCTION(BlueprintCallable, Category = "Sword|Presentation")
	void SetSwordDefinition(UGobulinSwordDefinition* InDefinition);

	/** Captures the current relative transform as the base pose for one attack or preview. */
	UFUNCTION(BlueprintCallable, Category = "Sword|Presentation")
	void BeginAttackPose();

	UFUNCTION(BlueprintCallable, Category = "Sword|Presentation")
	void SetAttackNormalizedTime(float NormalizedTime);

	UFUNCTION(BlueprintCallable, Category = "Sword|Presentation")
	void ResetPose();

	/** Applies PreviewTime in the editor without entering PIE. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sword|Preview")
	void PreviewAttackPose();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sword|Preview")
	void ResetPreviewPose();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreviewTime = 0.0f;

	/** Legacy compatibility property. The child visual transform is now fully designer-authored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword|Presentation")
	bool bPreserveTextureAspectRatio = true;

	/** Read-only width / height ratio of the assigned texture. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|Presentation")
	float TextureAspectRatio = 1.0f;

	virtual void OnRegister() override;

protected:
	void ApplySwordDefinition();
	void InitializeMaterial();
	void ApplyVisualLayout();

	UPROPERTY(Transient)
	FTransform PoseBaseTransform;

	bool bHasPoseBase = false;

	/** Resolved from SwordCombat and kept on the view so editor reconstruction does not lose it. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sword|Presentation")
	TObjectPtr<UGobulinSwordDefinition> SwordDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> VisualMesh;
};
