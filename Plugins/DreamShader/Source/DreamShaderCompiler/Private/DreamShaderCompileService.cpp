#include "DreamShaderCompileService.h"

namespace UE::DreamShader::Compiler
{
	FDreamShaderCompileResult FDreamShaderCompileService::CompileAssets(const FString& SourceFilePath, const bool bForce, const bool bTransient)
	{
		FDreamShaderCompileRequest Request;
		Request.SourceFilePath = SourceFilePath;
		Request.bForce = bForce;
		Request.bTransient = bTransient;
		return Compiler.CompileAssets(Request);
	}

	FDreamShaderCompileResult FDreamShaderCompileService::CompileMaterial(const FString& SourceFilePath, const bool bForce, const bool bTransient)
	{
		FDreamShaderCompileRequest Request;
		Request.SourceFilePath = SourceFilePath;
		Request.bForce = bForce;
		Request.bTransient = bTransient;
		return Compiler.CompileMaterial(Request);
	}
}
