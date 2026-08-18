#include "EnemyAssetSetupCommandlet.h"

#include "AssetToolsModule.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Engine/AssetManager.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "UObject/UObjectGlobals.h"

int32 UEnemyAssetSetupCommandlet::Main(const FString& Params)
{
	(void)Params;

	const FString AssetFolder = TEXT("/Game/_Game/Enemy");
	const FString AssetName = TEXT("EA_BasicEnemy");
	const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AssetFolder, *AssetName, *AssetName);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("_Game/Enemy")), true);

	UGobulinEnemyArchetype* Archetype = LoadObject<UGobulinEnemyArchetype>(nullptr, *ObjectPath);
	bool bCreatedNewAsset = false;
	if (!Archetype)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		Archetype = Cast<UGobulinEnemyArchetype>(AssetTools.CreateAsset(
			AssetName,
			AssetFolder,
			UGobulinEnemyArchetype::StaticClass(),
			nullptr));
		bCreatedNewAsset = Archetype != nullptr;
	}

	if (!Archetype)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyAssetSetup] Failed to create %s."), *ObjectPath);
		return 1;
	}

	if (bCreatedNewAsset)
	{
		Archetype->MaxHealth = 100.0f;
		Archetype->MoveSpeed = 300.0f;
		Archetype->TargetAcquisitionRadius = 5000.0f;
		Archetype->TargetLoseRadius = 6500.0f;
		Archetype->ContactDamage.bEnabled = true;
		Archetype->ContactDamage.BaseDamage = 10.0f;
		Archetype->ContactDamage.DamageInterval = 0.8f;
		Archetype->ContactDamage.ContactEnterTolerance = 5.0f;
		Archetype->ContactDamage.ContactExitTolerance = 15.0f;
		Archetype->DecisionInterval = 0.25f;
		Archetype->NavigationRetryDelay = 1.0f;
		Archetype->bUseRVOAvoidance = true;
		Archetype->AvoidanceConsiderationRadius = 250.0f;
		Archetype->SpawnDuration = 0.15f;
		Archetype->Body.CapsuleRadius = 34.0f;
		Archetype->Body.CapsuleHalfHeight = 88.0f;
	}

	// 只补齐空的语义标签，保留内容制作者已经选择的自定义攻击与伤害类型。
	if (!Archetype->ContactDamage.AttackTag.IsValid())
	{
		Archetype->ContactDamage.AttackTag = FGameplayTag::RequestGameplayTag(
			TEXT("Combat.Attack.Melee"));
	}
	if (!Archetype->ContactDamage.DamageType.IsValid())
	{
		Archetype->ContactDamage.DamageType = FGameplayTag::RequestGameplayTag(
			TEXT("Combat.Damage.Physical"));
	}
	if (!Archetype->Reaction.HeavyAttackTag.IsValid())
	{
		Archetype->Reaction.HeavyAttackTag = FGameplayTag::RequestGameplayTag(
			TEXT("Combat.Attack.Melee.Heavy"));
	}

	UPaperFlipbook* IdleTowardFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Idle_D.FieldPaladin__Idle_D"));
	UPaperFlipbook* IdleAwayFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Idle_U.FieldPaladin__Idle_U"));
	UPaperFlipbook* IdleLeftFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Idle_L.FieldPaladin__Idle_L"));
	UPaperFlipbook* IdleRightFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Idle_R.FieldPaladin__Idle_R"));
	UPaperFlipbook* RunTowardFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Run_D.FieldPaladin__Run_D"));
	UPaperFlipbook* RunAwayFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Run_U.FieldPaladin__Run_U"));
	UPaperFlipbook* RunLeftFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Run_L.FieldPaladin__Run_L"));
	UPaperFlipbook* RunRightFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/FieldPaladin/FieldPaladin__Run_R.FieldPaladin__Run_R"));
	UPaperFlipbook* DeathFlipbook = LoadObject<UPaperFlipbook>(
		nullptr,
		TEXT("/Game/Art/PaperAssets/Party/BattlePaladin/BattlePaladin__Downed.BattlePaladin__Downed"));
	UMaterialInterface* AliveMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/_Game/Material/Enemy/M_GobulinEnemySpriteMasked.M_GobulinEnemySpriteMasked"));
	UMaterialInterface* DeathMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/_Game/Material/Enemy/M_GobulinEnemySpriteTranslucent.M_GobulinEnemySpriteTranslucent"));
	if (!IdleTowardFlipbook
		|| !IdleAwayFlipbook
		|| !IdleLeftFlipbook
		|| !IdleRightFlipbook
		|| !RunTowardFlipbook
		|| !RunAwayFlipbook
		|| !RunLeftFlipbook
		|| !RunRightFlipbook
		|| !DeathFlipbook
		|| !AliveMaterial
		|| !DeathMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyAssetSetup] Missing FieldPaladin flipbooks, temporary Death flipbook, or generated enemy materials."));
		return 1;
	}

	// EA_BasicEnemy 明确采用 FieldPaladin 的八个方向化资源；其他人工装配参数保持不变。
	FGobulinEnemyFlipbookSet& Flipbooks = Archetype->Presentation.Flipbooks;
	Flipbooks.Idle.TowardViewer = IdleTowardFlipbook;
	Flipbooks.Idle.AwayFromViewer = IdleAwayFlipbook;
	Flipbooks.Idle.ViewerLeft = IdleLeftFlipbook;
	Flipbooks.Idle.ViewerRight = IdleRightFlipbook;
	Flipbooks.Run.TowardViewer = RunTowardFlipbook;
	Flipbooks.Run.AwayFromViewer = RunAwayFlipbook;
	Flipbooks.Run.ViewerLeft = RunLeftFlipbook;
	Flipbooks.Run.ViewerRight = RunRightFlipbook;
	if (Archetype->Presentation.Flipbooks.Death.IsNull())
	{
		Archetype->Presentation.Flipbooks.Death = DeathFlipbook;
	}
	Archetype->Presentation.MaterialOverride = AliveMaterial;
	Archetype->Presentation.DeathMaterialOverride = DeathMaterial;
	Archetype->Presentation.bLoopDeathFlipbook = true;

	if (bCreatedNewAsset)
	{
		Archetype->Presentation.SpriteColor = FLinearColor::White;
		Archetype->Presentation.TranslucencySortPriority = 0;
		Archetype->Presentation.bCastShadow = false;
		Archetype->Presentation.bFaceLocalPlayer = true;
		Archetype->Presentation.MinimumDirectionalSpeed = 10.0f;
		Archetype->Presentation.DirectionSwitchHysteresis = 0.12f;

		const FBoxSphereBounds Bounds = IdleTowardFlipbook->GetRenderBounds();
		const float LocalBottom = Bounds.Origin.Z - Bounds.BoxExtent.Z;
		Archetype->Presentation.VisualTransformFromGround = FTransform(
			FQuat::Identity,
			FVector(0.0f, 0.0f, -LocalBottom),
			FVector::OneVector);
	}

	if (!Archetype->IsDefinitionValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyAssetSetup] Generated archetype definition is invalid."));
		return 1;
	}

	Archetype->PostEditChange();
	Archetype->MarkPackageDirty();
	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages({ Archetype->GetPackage() }, true);
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.ScanPathForPrimaryAssets(
		UGobulinEnemyArchetype::PrimaryAssetType,
		AssetFolder,
		UGobulinEnemyArchetype::StaticClass(),
		false,
		false,
		true);
	const FSoftObjectPath RegisteredPath = AssetManager.GetPrimaryAssetPath(Archetype->GetPrimaryAssetId());
	const bool bRegistered = RegisteredPath.IsValid();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("[EnemyAssetSetup] Asset=%s PrimaryAssetId=%s Saved=%s Registered=%s"),
		*ObjectPath,
		*Archetype->GetPrimaryAssetId().ToString(),
		bSaved ? TEXT("true") : TEXT("false"),
		bRegistered ? TEXT("true") : TEXT("false"));
	return bSaved && bRegistered ? 0 : 1;
}
