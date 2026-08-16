#pragma once

#include "CoreMinimal.h"

class UObject;
class UMaterial;
class UMaterialFunction;

namespace UE::DreamShader::Editor
{
	enum class EDreamShaderDecompiledFunctionKind : uint8
	{
		Function,
		MaterialLayer,
		MaterialLayerBlend
	};

	struct FDreamShaderDecompileRequest
	{
		UObject* Asset = nullptr;
		FString OutputFilePath;
	};

	struct FDreamShaderDecompileResult
	{
		bool bSucceeded = false;
		FString SourceText;
		FString OutputFilePath;
		FString Error;
	};

	class IDreamShaderDecompiler
	{
	public:
		virtual ~IDreamShaderDecompiler() = default;

		virtual bool DecompileMaterial(UMaterial* Material, const FString& DecompiledName, FString& OutSourceText, FString& OutError) = 0;
		virtual bool DecompileFunction(
			UMaterialFunction* MaterialFunction,
			const FString& DecompiledName,
			EDreamShaderDecompiledFunctionKind FunctionKind,
			FString& OutSourceText,
			FString& OutError) = 0;
	};
}

namespace UE::DreamShader::Editor::Private
{
	struct FDecompiledAssetNaming
	{
		static FString MakeMaterialFilePath(const UMaterial* Material);
		static FString MakeFunctionFilePath(const UMaterialFunction* MaterialFunction);
		static const TCHAR* GetFunctionCategory(EDreamShaderDecompiledFunctionKind FunctionKind);
		static EDreamShaderDecompiledFunctionKind GetFunctionKind(const UMaterialFunction* MaterialFunction);
		static FString MakeAssetName(const UObject* Asset, const TCHAR* Category);
	};

	struct FDecompiledSourceWriter
	{
		static bool Save(const FDreamShaderDecompileResult& Result, FString& OutError);
	};

	class FDreamShaderDecompileService
	{
	public:
		explicit FDreamShaderDecompileService(IDreamShaderDecompiler& InDecompiler)
			: Decompiler(InDecompiler)
		{
		}

		FDreamShaderDecompileResult DecompileAsset(const FDreamShaderDecompileRequest& Request);

	private:
		IDreamShaderDecompiler& Decompiler;
	};

}
