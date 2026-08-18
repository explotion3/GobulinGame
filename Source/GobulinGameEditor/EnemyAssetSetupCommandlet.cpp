#include "EnemyAssetSetupCommandlet.h"

#include "AssetToolsModule.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Engine/AssetManager.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
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
		Archetype->AttackReadyDistance = 160.0f;
		Archetype->ResumeMoveDistance = 220.0f;
		Archetype->DecisionInterval = 0.25f;
		Archetype->NavigationRetryDelay = 1.0f;
		Archetype->bUseRVOAvoidance = true;
		Archetype->AvoidanceConsiderationRadius = 250.0f;
		Archetype->SpawnDuration = 0.15f;
		Archetype->DeathDuration = 0.6f;
		Archetype->Body.CapsuleRadius = 34.0f;
		Archetype->Body.CapsuleHalfHeight = 88.0f;
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
		TEXT("/Game/Art/PaperAssets/Party/BattleMage/BattleMage__Downed.BattleMage__Downed"));
	if (!IdleTowardFlipbook
		|| !IdleAwayFlipbook
		|| !IdleLeftFlipbook
		|| !IdleRightFlipbook
		|| !RunTowardFlipbook
		|| !RunAwayFlipbook
		|| !RunLeftFlipbook
		|| !RunRightFlipbook
		|| !DeathFlipbook)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyAssetSetup] Missing one or more FieldPaladin Idle/Run or temporary Death PaperFlipbooks."));
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

	if (bCreatedNewAsset)
	{
		Archetype->Presentation.MaterialOverride.Reset();
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
