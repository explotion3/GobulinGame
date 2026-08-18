#include "AssetSetupCommandlet.h"

#include "AssetToolsModule.h"
#include "Curves/CurveVector.h"
#include "Data/GobulinSwordDefinition.h"
#include "Factories/CurveFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputModifiers.h"
#include "Data/TitleDefinition.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UObject* CreateAssetInFolder(UClass* AssetClass, const FString& FolderPath, const FString& AssetName, UFactory* Factory = nullptr)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *FolderPath, *AssetName, *AssetName);
		if (UObject* ExistingAsset = LoadObject<UObject>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		return AssetTools.CreateAsset(AssetName, FolderPath, AssetClass, Factory);
	}

	bool SaveCreatedAssets(const TArray<UObject*>& Assets)
	{
		TArray<UPackage*> Packages;
		for (UObject* Asset : Assets)
		{
			if (!Asset)
			{
				continue;
			}
			Asset->MarkPackageDirty();
			Packages.AddUnique(Asset->GetPackage());
		}
		return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
	}

	UInputAction* CreateInputAction(const FString& Folder, const FString& Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = Cast<UInputAction>(CreateAssetInFolder(UInputAction::StaticClass(), Folder, Name));
		if (Action)
		{
			Action->ValueType = ValueType;
		}
		return Action;
	}

	UInputModifierSwizzleAxis* MakeSwizzle(UObject* Outer, EInputAxisSwizzle Order = EInputAxisSwizzle::YXZ)
	{
		UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>(Outer);
		Modifier->Order = Order;
		return Modifier;
	}

	UInputModifierNegate* MakeNegate(UObject* Outer)
	{
		return NewObject<UInputModifierNegate>(Outer);
	}

	void MapKeyWithModifiers(UInputMappingContext* IMC, UInputAction* Action, const FKey& Key, const TArray<UInputModifier*>& Modifiers)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Action, Key);
		for (UInputModifier* Modifier : Modifiers)
		{
			if (Modifier)
			{
				Mapping.Modifiers.Add(Modifier);
			}
		}
	}

	void SetVectorCurveKeys(UCurveVector* Curve, const TArray<TPair<float, FVector>>& Keys)
	{
		if (!Curve)
		{
			return;
		}

		for (FRichCurve& FloatCurve : Curve->FloatCurves)
		{
			FloatCurve.Reset();
			FloatCurve.SetDefaultValue(0.0f);
		}

		for (const TPair<float, FVector>& Key : Keys)
		{
			const FVector& Value = Key.Value;
			const float Components[3] = { Value.X, Value.Y, Value.Z };
			for (int32 ComponentIndex = 0; ComponentIndex < 3; ++ComponentIndex)
			{
				const FKeyHandle KeyHandle = Curve->FloatCurves[ComponentIndex].AddKey(Key.Key, Components[ComponentIndex]);
				FRichCurveKey& RichKey = Curve->FloatCurves[ComponentIndex].GetKey(KeyHandle);
				RichKey.InterpMode = RCIM_Cubic;
				RichKey.TangentMode = RCTM_Auto;
			}
		}

		Curve->MarkPackageDirty();
	}

	UCurveVector* CreateSwordCurve(const FString& Folder, const FString& Name, const TArray<TPair<float, FVector>>& Keys)
	{
		UCurveVectorFactory* Factory = NewObject<UCurveVectorFactory>();
		UCurveVector* Curve = Cast<UCurveVector>(CreateAssetInFolder(UCurveVector::StaticClass(), Folder, Name, Factory));
		SetVectorCurveKeys(Curve, Keys);
		return Curve;
	}

	UMaterial* CreateSwordMaterial(const FString& Folder, UTexture2D* SwordTexture)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		UMaterial* Material = Cast<UMaterial>(CreateAssetInFolder(UMaterial::StaticClass(), Folder, TEXT("M_GobulinWeaponView"), Factory));
		if (!Material || Material->GetExpressionCollection().Expressions.Num() > 0)
		{
			return Material;
		}

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->ParameterName = TEXT("WeaponTexture");
		TextureSample->Texture = SwordTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		Material->GetExpressionCollection().Expressions.Add(TextureSample);

		if (UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData())
		{
			EditorData->BaseColor.Expression = TextureSample;
			EditorData->BaseColor.OutputIndex = 0;
			EditorData->Opacity.Expression = TextureSample;
			EditorData->Opacity.OutputIndex = 1;
		}

		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->TwoSided = true;
		Material->SetShadingModel(MSM_Unlit);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return Material;
	}
}

int32 UAssetSetupCommandlet::Main(const FString& Params)
{
	const FString ActionsFolder = TEXT("/Game/Input/Actions");
	const FString IMCFolder = TEXT("/Game/Input/IMC");
	const FString TitlesFolder = TEXT("/Game/Data/Titles");
	const FString SwordFolder = TEXT("/Game/Art/Weapon");

	// 物理目录先行创建，确保 CreateAsset 路径可用
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Input/Actions")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Input/IMC")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Data/Titles")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Art/Weapon")), true);

	TArray<UObject*> CreatedAssets;

	// ---------- Input Actions ----------
	UInputAction* IA_Move = CreateInputAction(ActionsFolder, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	UInputAction* IA_Look = CreateInputAction(ActionsFolder, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	UInputAction* IA_Jump = CreateInputAction(ActionsFolder, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* IA_Sprint = CreateInputAction(ActionsFolder, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	UInputAction* IA_SwordAttack = CreateInputAction(ActionsFolder, TEXT("IA_SwordAttack"), EInputActionValueType::Boolean);
	UInputAction* IA_Interact = CreateInputAction(ActionsFolder, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	UInputAction* IA_PostBounty = CreateInputAction(ActionsFolder, TEXT("IA_PostBounty"), EInputActionValueType::Boolean);
	UInputAction* IA_Pause = CreateInputAction(ActionsFolder, TEXT("IA_Pause"), EInputActionValueType::Boolean);

	for (UInputAction* Action : { IA_Move, IA_Look, IA_Jump, IA_Sprint, IA_SwordAttack, IA_Interact, IA_PostBounty, IA_Pause })
	{
		if (Action)
		{
			CreatedAssets.Add(Action);
		}
	}

	// ---------- Input Mapping Context ----------
	UInputMappingContext* IMC_Default = Cast<UInputMappingContext>(CreateAssetInFolder(UInputMappingContext::StaticClass(), IMCFolder, TEXT("IMC_Default")));
	if (IMC_Default)
	{
		CreatedAssets.Add(IMC_Default);
		IMC_Default->UnmapAll();

		// 移动：W/S 映射到 Y 轴（Swizzle YXZ），A/D 映射到 X 轴
		MapKeyWithModifiers(IMC_Default, IA_Move, EKeys::W, { MakeSwizzle(IMC_Default) });
		MapKeyWithModifiers(IMC_Default, IA_Move, EKeys::S, { MakeSwizzle(IMC_Default), MakeNegate(IMC_Default) });
		MapKeyWithModifiers(IMC_Default, IA_Move, EKeys::A, { MakeNegate(IMC_Default) });
		MapKeyWithModifiers(IMC_Default, IA_Move, EKeys::D, {});

		MapKeyWithModifiers(IMC_Default, IA_Look, EKeys::MouseX, {});
		MapKeyWithModifiers(IMC_Default, IA_Look, EKeys::MouseY, { MakeSwizzle(IMC_Default) });
		MapKeyWithModifiers(IMC_Default, IA_Jump, EKeys::SpaceBar, {});
		MapKeyWithModifiers(IMC_Default, IA_Sprint, EKeys::LeftShift, {});
		MapKeyWithModifiers(IMC_Default, IA_Interact, EKeys::E, {});
		MapKeyWithModifiers(IMC_Default, IA_PostBounty, EKeys::T, {});
		MapKeyWithModifiers(IMC_Default, IA_Pause, EKeys::Escape, {});
	}

	// ---------- Player Input Mapping Context ----------
	// Keep the old IMC_Default intact for template compatibility. This context is
	// the minimal PC first-person input set used by AGobulinPlayerCharacter.
	UInputMappingContext* IMC_Player = Cast<UInputMappingContext>(CreateAssetInFolder(UInputMappingContext::StaticClass(), IMCFolder, TEXT("IMC_Player")));
	if (IMC_Player)
	{
		CreatedAssets.Add(IMC_Player);
		IMC_Player->UnmapAll();

		MapKeyWithModifiers(IMC_Player, IA_Move, EKeys::W, { MakeSwizzle(IMC_Player) });
		MapKeyWithModifiers(IMC_Player, IA_Move, EKeys::S, { MakeSwizzle(IMC_Player), MakeNegate(IMC_Player) });
		MapKeyWithModifiers(IMC_Player, IA_Move, EKeys::A, { MakeNegate(IMC_Player) });
		MapKeyWithModifiers(IMC_Player, IA_Move, EKeys::D, {});

		MapKeyWithModifiers(IMC_Player, IA_Look, EKeys::MouseX, {});
		MapKeyWithModifiers(IMC_Player, IA_Look, EKeys::MouseY, { MakeSwizzle(IMC_Player) });
		MapKeyWithModifiers(IMC_Player, IA_Jump, EKeys::SpaceBar, {});
		MapKeyWithModifiers(IMC_Player, IA_Sprint, EKeys::LeftShift, {});
		MapKeyWithModifiers(IMC_Player, IA_SwordAttack, EKeys::LeftMouseButton, {});
	}

	// ---------- Sword ----------
	UTexture2D* SwordTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Art/Weapon/Sword.Sword"));
	UMaterial* SwordMaterial = CreateSwordMaterial(SwordFolder, SwordTexture);
	UCurveVector* SwordLocationCurve = CreateSwordCurve(SwordFolder, TEXT("CV_Sword_AttackLocation"), {
		{ 0.0f, FVector::ZeroVector },
		{ 0.18f, FVector(0.0f, 16.0f, -12.0f) },
		{ 0.45f, FVector(-8.0f, -50.0f, 26.0f) },
		{ 0.65f, FVector(-2.0f, -20.0f, 10.0f) },
		{ 1.0f, FVector::ZeroVector }
	});
	UCurveVector* SwordRotationCurve = CreateSwordCurve(SwordFolder, TEXT("CV_Sword_AttackRotation"), {
		{ 0.0f, FVector::ZeroVector },
		{ 0.18f, FVector(-8.0f, 0.0f, 20.0f) },
		{ 0.45f, FVector(8.0f, 0.0f, -80.0f) },
		{ 0.65f, FVector(2.0f, 0.0f, -30.0f) },
		{ 1.0f, FVector::ZeroVector }
	});
	CreatedAssets.AddUnique(SwordMaterial);
	CreatedAssets.AddUnique(SwordLocationCurve);
	CreatedAssets.AddUnique(SwordRotationCurve);

	UGobulinSwordDefinition* SwordDefinition = Cast<UGobulinSwordDefinition>(CreateAssetInFolder(UGobulinSwordDefinition::StaticClass(), SwordFolder, TEXT("DA_Sword")));
	if (SwordDefinition)
	{
		SwordDefinition->WeaponTexture = SwordTexture;
		SwordDefinition->WeaponMaterial = SwordMaterial;
		SwordDefinition->AttackLocationCurve = SwordLocationCurve;
		SwordDefinition->AttackRotationCurve = SwordRotationCurve;
		SwordDefinition->HitWindowStartNormalizedTime = 0.18f;
		SwordDefinition->HitWindowEndNormalizedTime = 0.65f;
		SwordDefinition->InputBufferOpenNormalizedTime = 0.75f;
		SwordDefinition->SwingSoundNormalizedTime = 0.20f;
		SwordDefinition->HitCameraShakeScale = 1.0f;
		SwordDefinition->TipTraceRadius = 6.0f;
		SwordDefinition->HitKnockbackSpeed = 220.0f;
		SwordDefinition->HitLaunchSpeed = 90.0f;
		SwordDefinition->DamageSourceId = TEXT("Sword");
		SwordDefinition->MarkPackageDirty();
		CreatedAssets.Add(SwordDefinition);
	}

	// ---------- Titles (S01 / S02) ----------
	auto CreateTitle = [&](const FString& Name, FName TitleId, ETitleType Type, ETitleFunction Primary, ETitleFunction Secondary, const TArray<FName>& PreferredWeapons) -> UTitleDefinition*
	{
		UTitleDefinition* Definition = Cast<UTitleDefinition>(CreateAssetInFolder(UTitleDefinition::StaticClass(), TitlesFolder, Name));
		if (!Definition)
		{
			return nullptr;
		}
		Definition->TitleId = TitleId;
		Definition->TitleType = Type;
		Definition->PrimaryFunction = Primary;
		Definition->SecondaryFunction = Secondary;
		Definition->PreferredWeaponIds = PreferredWeapons;
		CreatedAssets.Add(Definition);
		return Definition;
	};

	CreateTitle(TEXT("S01"), TEXT("S01"), ETitleType::Melee, ETitleFunction::Tank, ETitleFunction::Damage, { TEXT("Sword") });
	CreateTitle(TEXT("S02"), TEXT("S02"), ETitleType::Spell, ETitleFunction::Damage, ETitleFunction::Control, { TEXT("Sword") });

	const bool bSaved = SaveCreatedAssets(CreatedAssets);
	UE_LOG(LogTemp, Warning, TEXT("[AssetSetup] Created %d assets, saved=%s"), CreatedAssets.Num(), bSaved ? TEXT("true") : TEXT("false"));
	return bSaved ? 0 : 1;
}
