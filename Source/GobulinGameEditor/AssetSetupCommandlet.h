#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AssetSetupCommandlet.generated.h"

/**
 * 一次性资产生成命令：玩家基础输入、IMC_Default、IMC_Player、Sword 曲线和数据资产。
 * 运行：UnrealEditor-Cmd.exe <uproject> -run=AssetSetup -unattended -nosplash -nullrhi
 */
UCLASS()
class UAssetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
