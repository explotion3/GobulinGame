#include "DreamShaderEditorCompileAdapter.h"

namespace UE::DreamShader::Editor
{
	Compiler::FDreamShaderCompileResult FEditorCompileAdapter::CompileAssets(const Compiler::FDreamShaderCompileRequest& Request)
	{
		Compiler::FDreamShaderCompileResult Result;
		Result.bSucceeded = FMaterialGenerator::GenerateAssetsFromFile(Request.SourceFilePath, Result.Message, Request.bForce, Request.bTransient);
		return Result;
	}

	Compiler::FDreamShaderCompileResult FEditorCompileAdapter::CompileMaterial(const Compiler::FDreamShaderCompileRequest& Request)
	{
		Compiler::FDreamShaderCompileResult Result;
		Result.bSucceeded = FMaterialGenerator::GenerateMaterialFromFile(Request.SourceFilePath, Result.Message, Request.bForce, Request.bTransient);
		return Result;
	}

	FEditorCompileAdapter& GetEditorCompileAdapter()
	{
		static FEditorCompileAdapter Adapter;
		return Adapter;
	}
}
