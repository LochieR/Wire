module;

#include <vector>
#include <string>
#include <filesystem>

export module wire.ui.renderer:shaderCompiler;

import :shaderCache;

namespace wire {

	export struct ShaderCompilationResult
	{
		bool Success;
		std::string ErrorMessage;

		std::vector<uint8_t> Bytecode;
		std::string EntryPoint;
	};

	export class ShaderCompiler
	{
	public:
		static ShaderCompilationResult compileHLSLToSpirv(const std::filesystem::path& path, ShaderType type, const std::string& entryPoint);
		static ShaderCache createShaderCacheHLSL(const std::filesystem::path& path, const std::string& vertexEntryPoint, const std::string& pixelEntryPoint);
		static ShaderCache createShaderCacheHLSL(const std::filesystem::path& path, const std::string& vertexEntryPoint, const std::string& pixelEntryPoint, const std::vector<RendererAPI>& apis);
		static ShaderCache createShaderCacheHLSL(const std::vector<std::filesystem::path>& paths, const std::vector<std::string>& vertexEntryPoints, const std::vector<std::string>& pixelEntryPoints);
		static ShaderCache createShaderCacheHLSL(const std::vector<std::filesystem::path>& paths, const std::vector<std::string>& vertexEntryPoints, const std::vector<std::string>& pixelEntryPoints, const std::vector<RendererAPI>& apis);
	};

}
