#pragma once

#include "DreamShaderCompileService.h"
#include "MaterialAssetGeneration/DreamShaderMaterialGenerator.h"

namespace UE::DreamShader::Editor
{
	class FEditorCompileAdapter final : public Compiler::IDreamShaderCompiler
	{
	public:
		virtual Compiler::FDreamShaderCompileResult CompileAssets(const Compiler::FDreamShaderCompileRequest& Request) override;
		virtual Compiler::FDreamShaderCompileResult CompileMaterial(const Compiler::FDreamShaderCompileRequest& Request) override;
	};

	FEditorCompileAdapter& GetEditorCompileAdapter();
}
