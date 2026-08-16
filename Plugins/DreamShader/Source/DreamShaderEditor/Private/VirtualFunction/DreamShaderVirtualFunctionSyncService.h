#pragma once

#include "CoreMinimal.h"

#include "Diagnostics/DreamShaderDiagnosticsStore.h"
#include "DreamShaderTypes.h"

class UMaterialFunction;

namespace UE::DreamShader::Editor::Private
{
	struct FDreamShaderVirtualFunctionDefinitionLocation
	{
		FString SourceFilePath;
		FString FunctionName;
		FString AssetObjectPath;
		FString CurrentText;
		TArray<FTextShaderFunctionParameter> Inputs;
		TArray<FTextShaderFunctionParameter> Outputs;
		int32 StartIndex = INDEX_NONE;
		int32 EndIndex = INDEX_NONE;
		int32 Line = 1;
		int32 Column = 1;
	};

	struct FDreamShaderVirtualFunctionSyncFileResult
	{
		FString SourceFilePath;
		int32 DefinitionCount = 0;
		int32 UpdatedDefinitionCount = 0;
		TArray<FDreamShaderDiagnosticRecord> Diagnostics;
	};

	struct FDreamShaderVirtualFunctionSyncResult
	{
		int32 ScannedDefinitionCount = 0;
		int32 UpdatedDefinitionCount = 0;
		int32 ErrorCount = 0;
		TArray<FDreamShaderVirtualFunctionSyncFileResult> Files;
	};

	struct FDreamShaderVirtualFunctionSyncService
	{
		using FDefinitionBuilder = TFunctionRef<bool(const UMaterialFunction*, FString&, FString&)>;

		static bool FindDefinitionForMaterialFunction(
			const UMaterialFunction* MaterialFunction,
			FDreamShaderVirtualFunctionDefinitionLocation& OutLocation);

		static FDreamShaderVirtualFunctionSyncResult SyncDefinitions(FDefinitionBuilder DefinitionBuilder);
	};
}
