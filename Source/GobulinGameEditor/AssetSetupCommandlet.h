#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AssetSetupCommandlet.generated.h"

/**
 * 一次性资产生成命令：InputAction、IMC_Default、IMC_Player、W01/W03/W05、S01/S02。
 * 运行：UnrealEditor-Cmd.exe <uproject> -run=AssetSetup -unattended -nosplash -nullrhi
 */
UCLASS()
class UAssetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
