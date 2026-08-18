#include "GobulinEnemyPreviewActor.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Enemy/GobulinEnemyPresentationComponent.h"
#include "Enemy/GobulinEnemyPresentationTypes.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "PaperFlipbook.h"
#include "ScopedTransaction.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GobulinEnemyPreviewActor"

namespace
{
	void ShowBakeError(const FText& Message)
	{
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("BakeErrorTitle", "敌人烘焙失败"));
	}

	void ShowBakeSuccess(const UGobulinEnemyArchetype* Archetype)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("BakeSuccess", "已写入 {0}，资产已标记为待保存"),
			FText::FromString(GetPathNameSafe(Archetype))));
		Info.ExpireDuration = 4.0f;
		Info.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

AGobulinEnemyPreviewActor::AGobulinEnemyPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = true;
	bReplicates = false;

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	SetRootComponent(CollisionCapsule);
	CollisionCapsule->InitCapsuleSize(34.0f, 88.0f);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionCapsule->SetGenerateOverlapEvents(false);
	CollisionCapsule->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	FeetAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FeetAnchor"));
	FeetAnchor->SetupAttachment(CollisionCapsule);
	FeetAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

	PresentationComponent = CreateDefaultSubobject<UGobulinEnemyPresentationComponent>(TEXT("Presentation"));
	PresentationComponent->SetupAttachment(FeetAnchor);
	PresentationComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GroundForwardMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("GroundForwardMarker"));
	GroundForwardMarker->SetupAttachment(FeetAnchor);
	GroundForwardMarker->ArrowColor = FColor::Green;
	GroundForwardMarker->ArrowSize = 1.5f;
	GroundForwardMarker->SetHiddenInGame(true);
}

void AGobulinEnemyPreviewActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateFeetAnchor();
}

void AGobulinEnemyPreviewActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AGobulinEnemyPreviewActor, TargetArchetype))
	{
		LoadFromArchetype();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AGobulinEnemyPreviewActor, PreviewAnimation)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(AGobulinEnemyPreviewActor, PreviewDirection))
	{
		RefreshPreviewAnimation();
	}
}

void AGobulinEnemyPreviewActor::LoadFromArchetype()
{
	if (!TargetArchetype || !CollisionCapsule || !PresentationComponent)
	{
		return;
	}

	LoadReferencedAssetsSynchronously();
	CollisionCapsule->SetCapsuleSize(
		TargetArchetype->Body.CapsuleRadius,
		TargetArchetype->Body.CapsuleHalfHeight,
		false);
	UpdateFeetAnchor();
	bFaceLocalPlayer = TargetArchetype->Presentation.bFaceLocalPlayer;
	PresentationComponent->ApplyDefinition(TargetArchetype->Presentation);
	RefreshPreviewAnimation();
}

void AGobulinEnemyPreviewActor::BakeToArchetype()
{
	FText ValidationError;
	if (!ValidateBake(ValidationError))
	{
		ShowBakeError(ValidationError);
		return;
	}

	UPaperFlipbook* CurrentFlipbook = PresentationComponent->GetFlipbook();
	UMaterialInterface* CurrentMaterial = PresentationComponent->OverrideMaterials.IsValidIndex(0)
		? PresentationComponent->OverrideMaterials[0].Get()
		: nullptr;

	const FScopedTransaction Transaction(LOCTEXT("BakeEnemyPreview", "烘焙敌人预览到数据资产"));
	TargetArchetype->Modify();
	TargetArchetype->Body.CapsuleRadius = CollisionCapsule->GetUnscaledCapsuleRadius();
	TargetArchetype->Body.CapsuleHalfHeight = CollisionCapsule->GetUnscaledCapsuleHalfHeight();
	TargetArchetype->Presentation.VisualTransformFromGround = PresentationComponent->GetRelativeTransform();
	if (PreviewAnimation != EGobulinEnemyPreviewAnimation::Death)
	{
		const EGobulinEnemyLocomotionAnimation Locomotion =
			PreviewAnimation == EGobulinEnemyPreviewAnimation::Run
			? EGobulinEnemyLocomotionAnimation::Run
			: EGobulinEnemyLocomotionAnimation::Idle;
		TargetArchetype->Presentation.Flipbooks.SetForLocomotion(
			PreviewDirection,
			Locomotion,
			CurrentFlipbook);
	}
	else
	{
		TargetArchetype->Presentation.Flipbooks.Death = CurrentFlipbook;
	}
	TargetArchetype->Presentation.MaterialOverride = CurrentMaterial;
	TargetArchetype->Presentation.SpriteColor = PresentationComponent->GetSpriteColor();
	TargetArchetype->Presentation.TranslucencySortPriority = PresentationComponent->TranslucencySortPriority;
	TargetArchetype->Presentation.bCastShadow = PresentationComponent->CastShadow;
	TargetArchetype->Presentation.bFaceLocalPlayer = bFaceLocalPlayer;
	TargetArchetype->PostEditChange();
	TargetArchetype->MarkPackageDirty();
	UAssetManager::Get().RefreshAssetData(TargetArchetype);

	PresentationComponent->ApplyDefinition(TargetArchetype->Presentation);
	RefreshPreviewAnimation();
	ShowBakeSuccess(TargetArchetype);
}

void AGobulinEnemyPreviewActor::UpdateFeetAnchor()
{
	if (!CollisionCapsule || !FeetAnchor)
	{
		return;
	}

	FeetAnchor->SetRelativeLocation(FVector(
		0.0f,
		0.0f,
		-CollisionCapsule->GetUnscaledCapsuleHalfHeight()));
}

void AGobulinEnemyPreviewActor::RefreshPreviewAnimation()
{
	if (!TargetArchetype || !PresentationComponent)
	{
		return;
	}

	if (PreviewAnimation != EGobulinEnemyPreviewAnimation::Death)
	{
		const EGobulinEnemyLocomotionAnimation Locomotion =
			PreviewAnimation == EGobulinEnemyPreviewAnimation::Run
			? EGobulinEnemyLocomotionAnimation::Run
			: EGobulinEnemyLocomotionAnimation::Idle;
		TargetArchetype->Presentation.Flipbooks.GetForLocomotion(
			PreviewDirection,
			Locomotion).LoadSynchronous();
		TargetArchetype->Presentation.Flipbooks.GetForLocomotion(
			EGobulinEnemyMoveDirection::TowardViewer,
			Locomotion).LoadSynchronous();
		PresentationComponent->ApplyVisualState(EGobulinEnemyVisualState::Alive, true);
		PresentationComponent->ApplyLocomotion(PreviewDirection, Locomotion, true, false);
		PresentationComponent->PlayFromStart();
	}
	else
	{
		TargetArchetype->Presentation.Flipbooks.Death.LoadSynchronous();
		PresentationComponent->ApplyVisualState(EGobulinEnemyVisualState::Death, true);
	}
}

void AGobulinEnemyPreviewActor::LoadReferencedAssetsSynchronously()
{
	FGobulinEnemyFlipbookSet& Flipbooks = TargetArchetype->Presentation.Flipbooks;
	auto LoadDirectionalSet = [](FGobulinEnemyDirectionalFlipbookSet& Set)
	{
		Set.TowardViewer.LoadSynchronous();
		Set.AwayFromViewer.LoadSynchronous();
		Set.ViewerLeft.LoadSynchronous();
		Set.ViewerRight.LoadSynchronous();
	};
	LoadDirectionalSet(Flipbooks.Idle);
	LoadDirectionalSet(Flipbooks.Run);
	Flipbooks.Death.LoadSynchronous();
	TargetArchetype->Presentation.MaterialOverride.LoadSynchronous();
}

bool AGobulinEnemyPreviewActor::ValidateBake(FText& OutError) const
{
	if (!TargetArchetype)
	{
		OutError = LOCTEXT("MissingTarget", "请先设置 Target Enemy Archetype。");
		return false;
	}

	if (!GetWorld() || GetWorld()->WorldType != EWorldType::Editor)
	{
		OutError = LOCTEXT("WrongWorld", "只能从普通编辑器关卡中的 PreviewActor 烘焙，不能在 PIE 中执行。");
		return false;
	}

	if (!GetActorScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER))
	{
		OutError = LOCTEXT("ScaledActor", "PreviewActor 的世界缩放必须为 1。请直接调整胶囊尺寸和表现组件缩放。");
		return false;
	}

	FGobulinEnemyBodyDefinition Body;
	Body.CapsuleRadius = CollisionCapsule->GetUnscaledCapsuleRadius();
	Body.CapsuleHalfHeight = CollisionCapsule->GetUnscaledCapsuleHalfHeight();
	if (!Body.IsValid())
	{
		OutError = LOCTEXT("InvalidBody", "胶囊尺寸无效：半径必须大于零，半高必须不小于半径。");
		return false;
	}

	UPaperFlipbook* CurrentFlipbook = PresentationComponent->GetFlipbook();
	if (!CurrentFlipbook || !CurrentFlipbook->IsAsset())
	{
		OutError = LOCTEXT("MissingFlipbook", "当前表现组件必须使用已保存的 PaperFlipbook 资产。");
		return false;
	}

	UMaterialInterface* CurrentMaterial = PresentationComponent->OverrideMaterials.IsValidIndex(0)
		? PresentationComponent->OverrideMaterials[0].Get()
		: nullptr;
	if (CurrentMaterial && !CurrentMaterial->IsAsset())
	{
		OutError = LOCTEXT("TransientMaterial", "不能烘焙动态或临时材质实例，请使用已保存的材质资产。");
		return false;
	}

	FGobulinEnemyPaperPresentationDefinition Presentation = TargetArchetype->Presentation;
	Presentation.VisualTransformFromGround = PresentationComponent->GetRelativeTransform();
	if (PreviewAnimation != EGobulinEnemyPreviewAnimation::Death)
	{
		const EGobulinEnemyLocomotionAnimation Locomotion =
			PreviewAnimation == EGobulinEnemyPreviewAnimation::Run
			? EGobulinEnemyLocomotionAnimation::Run
			: EGobulinEnemyLocomotionAnimation::Idle;
		Presentation.Flipbooks.SetForLocomotion(PreviewDirection, Locomotion, CurrentFlipbook);
	}
	else
	{
		Presentation.Flipbooks.Death = CurrentFlipbook;
	}
	if (!Presentation.IsValid())
	{
		OutError = LOCTEXT(
			"InvalidPresentation",
			"表现定义无效；请保证 Transform 缩放各轴非零，并为四方向 Idle、四方向 Run 和死亡分别设置 Flipbook。");
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
