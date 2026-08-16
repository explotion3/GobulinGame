#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#ifndef DREAMSHADERCOMPILER_API
#define DREAMSHADERCOMPILER_API
#endif

class DREAMSHADERCOMPILER_API FDreamShaderCompilerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
