#include "AssetSetupCommandlet.h"

#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "FileHelpers.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputModifiers.h"
#include "Data/GameTypes.h"
#include "Data/TitleDefinition.h"
#include "Data/WeaponDefinition.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	UObject* CreateAssetInFolder(UClass* AssetClass, const FString& FolderPath, const FString& AssetName)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *FolderPath, *AssetName, *AssetName);
		if (UObject* ExistingAsset = LoadObject<UObject>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		return AssetTools.CreateAsset(AssetName, FolderPath, AssetClass, nullptr);
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
}

int32 UAssetSetupCommandlet::Main(const FString& Params)
{
	const FString ActionsFolder = TEXT("/Game/Input/Actions");
	const FString IMCFolder = TEXT("/Game/Input/IMC");
	const FString WeaponsFolder = TEXT("/Game/Data/Weapons");
	const FString TitlesFolder = TEXT("/Game/Data/Titles");

	// 物理目录先行创建，确保 CreateAsset 路径可用
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Input/Actions")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Input/IMC")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Data/Weapons")), true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() / TEXT("Data/Titles")), true);

	TArray<UObject*> CreatedAssets;

	// ---------- Input Actions ----------
	UInputAction* IA_Move = CreateInputAction(ActionsFolder, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	UInputAction* IA_Look = CreateInputAction(ActionsFolder, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	UInputAction* IA_Jump = CreateInputAction(ActionsFolder, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* IA_Sprint = CreateInputAction(ActionsFolder, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	UInputAction* IA_Fire = CreateInputAction(ActionsFolder, TEXT("IA_Fire"), EInputActionValueType::Boolean);
	UInputAction* IA_Aim = CreateInputAction(ActionsFolder, TEXT("IA_Aim"), EInputActionValueType::Boolean);
	UInputAction* IA_Reload = CreateInputAction(ActionsFolder, TEXT("IA_Reload"), EInputActionValueType::Boolean);
	UInputAction* IA_Melee = CreateInputAction(ActionsFolder, TEXT("IA_Melee"), EInputActionValueType::Boolean);
	UInputAction* IA_SwitchWeapon = CreateInputAction(ActionsFolder, TEXT("IA_SwitchWeapon"), EInputActionValueType::Axis1D);
	UInputAction* IA_Interact = CreateInputAction(ActionsFolder, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	UInputAction* IA_PostBounty = CreateInputAction(ActionsFolder, TEXT("IA_PostBounty"), EInputActionValueType::Boolean);
	UInputAction* IA_Pause = CreateInputAction(ActionsFolder, TEXT("IA_Pause"), EInputActionValueType::Boolean);

	for (UInputAction* Action : { IA_Move, IA_Look, IA_Jump, IA_Sprint, IA_Fire, IA_Aim, IA_Reload, IA_Melee, IA_SwitchWeapon, IA_Interact, IA_PostBounty, IA_Pause })
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
		MapKeyWithModifiers(IMC_Default, IA_Fire, EKeys::LeftMouseButton, {});
		MapKeyWithModifiers(IMC_Default, IA_Aim, EKeys::RightMouseButton, {});
		MapKeyWithModifiers(IMC_Default, IA_Reload, EKeys::R, {});
		MapKeyWithModifiers(IMC_Default, IA_Melee, EKeys::V, {});
		MapKeyWithModifiers(IMC_Default, IA_SwitchWeapon, EKeys::One, {});
		MapKeyWithModifiers(IMC_Default, IA_SwitchWeapon, EKeys::Two, {});
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
		MapKeyWithModifiers(IMC_Player, IA_Fire, EKeys::LeftMouseButton, {});
		MapKeyWithModifiers(IMC_Player, IA_Aim, EKeys::RightMouseButton, {});
		MapKeyWithModifiers(IMC_Player, IA_Reload, EKeys::R, {});
		MapKeyWithModifiers(IMC_Player, IA_Melee, EKeys::V, {});
		MapKeyWithModifiers(IMC_Player, IA_SwitchWeapon, EKeys::One, {});
		MapKeyWithModifiers(IMC_Player, IA_SwitchWeapon, EKeys::Two, {});
	}

	// ---------- Weapons (W01 / W03 / W05) ----------
	auto CreateWeapon = [&](const FString& Name, FName WeaponId, EWeaponType Type, EWeaponFunction Primary, EWeaponFunction Secondary, EAmmoType Ammo, float BaseDamage, float FireRate, float Spread, float ProjectileSpeed, float FalloffStart, float FalloffEnd) -> UWeaponDefinition*
	{
		UWeaponDefinition* Definition = Cast<UWeaponDefinition>(CreateAssetInFolder(UWeaponDefinition::StaticClass(), WeaponsFolder, Name));
		if (!Definition)
		{
			return nullptr;
		}
		Definition->WeaponId = WeaponId;
		Definition->WeaponType = Type;
		Definition->PrimaryFunction = Primary;
		Definition->SecondaryFunction = Secondary;
		Definition->AmmoType = Ammo;
		Definition->BaseDamage = BaseDamage;
		Definition->FireRate = FireRate;
		Definition->Spread = Spread;
		Definition->ProjectileSpeed = ProjectileSpeed;
		Definition->FalloffStartDistance = FalloffStart;
		Definition->FalloffEndDistance = FalloffEnd;
		Definition->MaxTier = 5;
		Definition->ModSlotCount = 3;
		CreatedAssets.Add(Definition);
		return Definition;
	};

	CreateWeapon(TEXT("W01"), TEXT("W01"), EWeaponType::Melee, EWeaponFunction::HordeClear, EWeaponFunction::EnergyReturn, EAmmoType::None, 20.0f, 2.22f, 0.0f, 0.0f, 0.0f, 0.0f);
	CreateWeapon(TEXT("W03"), TEXT("W03"), EWeaponType::Precision, EWeaponFunction::SingleTarget, EWeaponFunction::ArmorBreak, EAmmoType::Arrows, 45.0f, 1.2f, 0.0f, 3000.0f, 1500.0f, 4500.0f);
	CreateWeapon(TEXT("W05"), TEXT("W05"), EWeaponType::Spell, EWeaponFunction::HordeClear, EWeaponFunction::Control, EAmmoType::Mana, 25.0f, 2.0f, 0.0f, 2500.0f, 0.0f, 0.0f);

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

	CreateTitle(TEXT("S01"), TEXT("S01"), ETitleType::Melee, ETitleFunction::Tank, ETitleFunction::Damage, { TEXT("W01"), TEXT("W05") });
	CreateTitle(TEXT("S02"), TEXT("S02"), ETitleType::Spell, ETitleFunction::Damage, ETitleFunction::Control, { TEXT("W05"), TEXT("W03") });

	const bool bSaved = SaveCreatedAssets(CreatedAssets);
	UE_LOG(LogTemp, Warning, TEXT("[AssetSetup] Created %d assets, saved=%s"), CreatedAssets.Num(), bSaved ? TEXT("true") : TEXT("false"));
	return bSaved ? 0 : 1;
}
