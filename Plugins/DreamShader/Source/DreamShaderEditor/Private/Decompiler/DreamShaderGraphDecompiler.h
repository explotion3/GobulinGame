#pragma once

#include "CoreMinimal.h"

class UMaterialFunction;

namespace UE::DreamShader::Editor
{
	class IDreamShaderDecompiler;
}

namespace UE::DreamShader::Editor::Private
{
	IDreamShaderDecompiler& GetGraphDecompiler();

	bool BuildGraphDecompilerVirtualFunctionDefinition(
		const UMaterialFunction* MaterialFunction,
		FString& OutDefinition,
		FString& OutError);
}
