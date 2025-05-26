module;

#include "Wire/Core/Assert.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_hlsl.hpp>

#include <string>
#include <fstream>
#include <sstream>

module wire.ui.renderer:shaderCompiler;

import :renderer;
import wire.serialization;

namespace wire {

	namespace Utils {

		static shaderc_shader_kind ConvertShaderType(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::Vertex:
				return shaderc_vertex_shader;
			case ShaderType::Pixel:
				return shaderc_fragment_shader;
			default:
				break;
			}

			WR_ASSERT(false, "Unkown shader type");
			return shaderc_shader_kind(0);
		}

	}

	ShaderCompilationResult ShaderCompiler::compileHLSLToSpirv(const std::filesystem::path& path, ShaderType type, const std::string& entryPoint)
	{
		ShaderCompilationResult comp;

		std::ifstream file(path);
		std::stringstream ss;
		ss << file.rdbuf();
		file.close();

		if (!file.good())
		{
			return ShaderCompilationResult{
				.Success = false,
				.ErrorMessage = "Failed to open file " + path.string()
			};
		}

		std::string shader = ss.str();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetSourceLanguage(shaderc_source_language_hlsl);
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.AddMacroDefinition("SPIRV");

		constexpr bool debug =
#ifdef WR_DEBUG
			true;
#else
			false;
#endif

		if constexpr (debug)
		{
			options.SetGenerateDebugInfo();
			options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}
		else
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shader, Utils::ConvertShaderType(type), path.string().c_str(), entryPoint.c_str(), options);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			comp.Success = false;
			comp.ErrorMessage = result.GetErrorMessage();

			return comp;
		}

		const uint8_t* begin = reinterpret_cast<const uint8_t*>(result.cbegin());
		const uint8_t* end = reinterpret_cast<const uint8_t*>(result.cend());

		comp.Bytecode = { begin, end };
		comp.EntryPoint = entryPoint;

		return comp;
	}

	ShaderCache ShaderCompiler::createShaderCacheHLSL(const std::filesystem::path& path, const std::string& vertexEntryPoint, const std::string& pixelEntryPoint)
	{
		return createShaderCacheHLSL(path, vertexEntryPoint, pixelEntryPoint, { RendererAPI::Vulkan });
	}

	ShaderCache ShaderCompiler::createShaderCacheHLSL(const std::filesystem::path& path, const std::string& vertexEntryPoint, const std::string& pixelEntryPoint, const std::vector<RendererAPI>& apis)
	{
		return createShaderCacheHLSL(std::vector<std::filesystem::path>{ path }, std::vector<std::string>{ vertexEntryPoint }, std::vector<std::string>{ pixelEntryPoint }, apis);
	}

	ShaderCache ShaderCompiler::createShaderCacheHLSL(const std::vector<std::filesystem::path>& paths, const std::vector<std::string>& vertexEntryPoints, const std::vector<std::string>& pixelEntryPoints)
	{
		return createShaderCacheHLSL(paths, vertexEntryPoints, pixelEntryPoints, { RendererAPI::Vulkan });
	}

	ShaderCache ShaderCompiler::createShaderCacheHLSL(const std::vector<std::filesystem::path>& paths, const std::vector<std::string>& vertexEntryPoints, const std::vector<std::string>& pixelEntryPoints, const std::vector<RendererAPI>& apis)
	{
		WR_ASSERT(paths.size() == vertexEntryPoints.size() && paths.size() == pixelEntryPoints.size(), "ShaderCache must have same number of vertex and pixel shaders!");

		std::vector<ShaderGroup> groups;

		for (uint32_t i = 0; i < paths.size(); i++)
		{
			std::filesystem::path path = paths[i];
			std::string vertexEntryPoint = vertexEntryPoints[i];
			std::string pixelEntryPoint = pixelEntryPoints[i];

			std::ifstream file(path);
			std::stringstream ss;
			ss << file.rdbuf();

			std::string fileString = ss.str();
			file.close();

			std::string sha256 = generateSHA256(fileString);

			ShaderGroup group;
			group.Name = path.filename().string();

			std::memcpy(group.SHA256, sha256.data(), sizeof(char) * 32);

			if (std::find(apis.begin(), apis.end(), RendererAPI::Vulkan) != apis.end())
			{
				ShaderCompilationResult vkVShader = compileHLSLToSpirv(path, ShaderType::Vertex, vertexEntryPoint);
				ShaderObject vkVertex;
				vkVertex.API = RendererAPI::Vulkan;
				vkVertex.Type = ShaderType::Vertex;
				vkVertex.EntryPoint = vertexEntryPoint;
				vkVertex.Bytecode = vkVShader.Bytecode;

				ShaderCompilationResult vkPShader = compileHLSLToSpirv(path, ShaderType::Pixel, pixelEntryPoint);
				ShaderObject vkPixel;
				vkPixel.API = RendererAPI::Vulkan;
				vkPixel.Type = ShaderType::Pixel;
				vkPixel.EntryPoint = pixelEntryPoint;
				vkPixel.Bytecode = vkPShader.Bytecode;

				group.Objects.push_back(vkVertex);
				group.Objects.push_back(vkPixel);
			}

			groups.push_back(group);
		}

		return ShaderCache(groups);
	}

}
