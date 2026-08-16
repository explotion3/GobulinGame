#pragma once

#include "DreamShaderCompilerInterfaces.h"

namespace UE::DreamShader::Compiler
{
	class DREAMSHADERCOMPILER_API FDreamShaderCompileService
	{
	public:
		explicit FDreamShaderCompileService(IDreamShaderCompiler& InCompiler)
			: Compiler(InCompiler)
		{
		}

		FDreamShaderCompileResult CompileAssets(const FString& SourceFilePath, bool bForce = false, bool bTransient = false);
		FDreamShaderCompileResult CompileMaterial(const FString& SourceFilePath, bool bForce = false, bool bTransient = false);

	private:
		IDreamShaderCompiler& Compiler;
	};
}
