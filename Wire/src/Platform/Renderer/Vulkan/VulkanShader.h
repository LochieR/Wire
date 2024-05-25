#pragma once

#include "VulkanRenderer.h"

#include "Wire/Core/Base.h"
#include "Wire/Renderer/Shader.h"

#include <array>
#include <vector>

struct VkShaderModule_T; typedef VkShaderModule_T* VkShaderModule;

namespace Wire {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(Renderer* renderer, std::string_view filepath);
		virtual ~VulkanShader();

		virtual void Reload() override;
		virtual rbRef<GraphicsPipeline> CreatePipeline(const InputLayout& layout, PrimitiveTopology topology) override;
		virtual rbRef<GraphicsPipeline> CreatePipeline(const InputLayout& layout, PrimitiveTopology topology, rbRef<Framebuffer> framebuffer) override;

		virtual const std::filesystem::path& GetFilepath() const override { return m_Filepath; }
	private:
		void CompileOrGetVulkanBinaries(const std::array<std::string, 2>& sources);
		void CreateShaderModules();

		void Reflect(const std::string& name, const std::vector<uint32_t>& data) const;

		static std::string ReadFile(const std::filesystem::path& filepath);
		static std::array<std::string, 2> PreProcess(const std::string& source);
	private:
		VulkanRenderer* m_Renderer = nullptr;

		std::filesystem::path m_Filepath;

		std::vector<uint32_t> m_VertexData;
		std::vector<uint32_t> m_FragmentData;

		VkShaderModule m_VertexModule = nullptr;
		VkShaderModule m_FragmentModule = nullptr;
	};

}
