#include "Combat/GobulinWeaponViewComponent.h"

#include "Curves/CurveVector.h"
#include "Data/GobulinSwordDefinition.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UGobulinWeaponViewComponent::UGobulinWeaponViewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetVisibility(false, false);
	SetOnlyOwnerSee(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetReceivesDecals(false);
	SetMobility(EComponentMobility::Movable);
}

void UGobulinWeaponViewComponent::SetVisualMesh(UStaticMeshComponent* InVisualMesh)
{
	VisualMesh = InVisualMesh;
	ApplySwordDefinition();
}

void UGobulinWeaponViewComponent::SetSwordDefinition(UGobulinSwordDefinition* InDefinition)
{
	SwordDefinition = InDefinition;
	MaterialInstance = nullptr;
	ApplySwordDefinition();
}

void UGobulinWeaponViewComponent::BeginAttackPose()
{
	PoseBaseTransform = GetRelativeTransform();
	bHasPoseBase = true;
}

void UGobulinWeaponViewComponent::SetAttackNormalizedTime(float NormalizedTime)
{
	if (!SwordDefinition)
	{
		return;
	}

	const float Time = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
	const FVector LocationOffset = SwordDefinition->AttackLocationCurve
		? SwordDefinition->AttackLocationCurve->GetVectorValue(Time)
		: FVector::ZeroVector;
	const FVector RotationOffset = SwordDefinition->AttackRotationCurve
		? SwordDefinition->AttackRotationCurve->GetVectorValue(Time)
		: FVector::ZeroVector;

	if (!bHasPoseBase)
	{
		BeginAttackPose();
	}

	FTransform Pose = PoseBaseTransform;
	Pose.AddToTranslation(LocationOffset);
	Pose.SetRotation((Pose.GetRotation() * FRotator(
		RotationOffset.X,
		RotationOffset.Y,
		RotationOffset.Z).Quaternion()).GetNormalized());
	SetRelativeTransform(Pose);
}

void UGobulinWeaponViewComponent::ResetPose()
{
	if (bHasPoseBase)
	{
		SetRelativeTransform(PoseBaseTransform);
		bHasPoseBase = false;
	}
}

void UGobulinWeaponViewComponent::PreviewAttackPose()
{
	SetAttackNormalizedTime(PreviewTime);
}

void UGobulinWeaponViewComponent::ResetPreviewPose()
{
	PreviewTime = 0.0f;
	ResetPose();
}

void UGobulinWeaponViewComponent::OnRegister()
{
	Super::OnRegister();

	// Registration can happen after a Blueprint component transform edit. Only
	// correct the texture aspect here; never overwrite location or rotation.
	if (SwordDefinition)
	{
		ApplyVisualLayout();
	}
}

void UGobulinWeaponViewComponent::ApplySwordDefinition()
{
	if (!VisualMesh)
	{
		return;
	}

	// Keep the historical parent component as a transform-only pivot. Any
	// Plane serialized by the old Blueprint template must not render alongside
	// the new FirstPersonWeaponVisual child.
	SetVisibility(false, false);
	SetStaticMesh(nullptr);

	if (!SwordDefinition)
	{
		VisualMesh->SetVisibility(false, true);
		VisualMesh->SetMaterial(0, nullptr);
		MaterialInstance = nullptr;
		return;
	}

	VisualMesh->SetVisibility(true, true);
	ApplyVisualLayout();

	if (!IsRegistered())
	{
		if (SwordDefinition->WeaponMaterial)
		{
			VisualMesh->SetMaterial(0, SwordDefinition->WeaponMaterial);
		}
		return;
	}

	InitializeMaterial();
}

void UGobulinWeaponViewComponent::ApplyVisualLayout()
{
	if (!VisualMesh || !SwordDefinition || !SwordDefinition->WeaponTexture)
	{
		return;
	}

	const int32 TextureWidth = SwordDefinition->WeaponTexture->GetSizeX();
	const int32 TextureHeight = SwordDefinition->WeaponTexture->GetSizeY();
	if (TextureWidth > 0 && TextureHeight > 0)
	{
		TextureAspectRatio = static_cast<float>(TextureWidth) / static_cast<float>(TextureHeight);
	}

	// The child transform is fully designer-authored. Do not modify its
	// location, rotation, or scale here; the sword handle can be aligned to the
	// parent pivot manually in the Blueprint editor.
}

void UGobulinWeaponViewComponent::InitializeMaterial()
{
	if (!VisualMesh || !SwordDefinition || !SwordDefinition->WeaponMaterial)
	{
		if (VisualMesh)
		{
			VisualMesh->SetMaterial(0, nullptr);
		}
		MaterialInstance = nullptr;
		return;
	}

	MaterialInstance = VisualMesh->CreateDynamicMaterialInstance(0, SwordDefinition->WeaponMaterial);
	if (MaterialInstance && SwordDefinition->WeaponTexture)
	{
		MaterialInstance->SetTextureParameterValue(TEXT("WeaponTexture"), SwordDefinition->WeaponTexture);
	}
}
