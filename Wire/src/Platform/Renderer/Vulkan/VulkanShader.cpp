#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanShader.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanFramebuffer.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace Wire {

	namespace Utils {

		VkVertexInputBindingDescription GetBindingDescription(const BufferLayout& layout);
		std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(const BufferLayout& layout);

		static const char* GetCacheDirectory()
		{
			return "assets/cache/shader";
		}

		static void CreateCacheDirectory()
		{
			std::string cacheDir = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDir))
				std::filesystem::create_directories(cacheDir);
		}

		static VkShaderStageFlags GetVkShaderStageFromWireStage(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
				case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			WR_ASSERT(false);
			return 0;
		}

		static VkDescriptorType GetVkDescriptorTypeFromWireResourceType(ShaderResourceType type)
		{
			switch (type)
			{
				case ShaderResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				case ShaderResourceType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				case ShaderResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			WR_ASSERT(false);
			return (VkDescriptorType)-1;
		}

		static VkPrimitiveTopology GetVkPrimitiveTopologyFromWirePrimitiveTopology(PrimitiveTopology topology)
		{
			switch (topology)
			{
				case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case PrimitiveTopology::LineList:	  return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				case PrimitiveTopology::LineStrip:    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			}

			WR_ASSERT(false);
			return (VkPrimitiveTopology)-1;
		}

		static VkSampleCountFlagBits GetMaxUsableSampleCount(VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(vkd.PhysicalDevice, &physicalDeviceProperties);

			VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
			if (counts & VK_SAMPLE_COUNT_64_BIT)
				return VK_SAMPLE_COUNT_64_BIT;
			if (counts & VK_SAMPLE_COUNT_32_BIT)
				return VK_SAMPLE_COUNT_32_BIT;
			if (counts & VK_SAMPLE_COUNT_16_BIT)
				return VK_SAMPLE_COUNT_16_BIT;
			if (counts & VK_SAMPLE_COUNT_8_BIT)
				return VK_SAMPLE_COUNT_8_BIT;
			if (counts & VK_SAMPLE_COUNT_4_BIT)
				return VK_SAMPLE_COUNT_4_BIT;
			if (counts & VK_SAMPLE_COUNT_2_BIT)
				return VK_SAMPLE_COUNT_2_BIT;

			return VK_SAMPLE_COUNT_1_BIT;
		}

	}

	VulkanShader::VulkanShader(Renderer* renderer, std::string_view path)
		: m_Renderer((VulkanRenderer*)renderer), m_Filepath(path.data())
	{
		Reload();
	}

	VulkanShader::~VulkanShader()
	{
		m_Renderer->SubmitResourceFree([vertexModule = m_VertexModule, fragmentModule = m_FragmentModule](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyShaderModule(vkd.Device, vertexModule, vkd.Allocator);
			vkDestroyShaderModule(vkd.Device, fragmentModule, vkd.Allocator);
		});
	}

	rbRef<GraphicsPipeline> VulkanShader::CreatePipeline(const InputLayout& layout, PrimitiveTopology topology, bool multiSample)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		VulkanGraphicsPipeline* pipeline = new VulkanGraphicsPipeline(m_Renderer);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_VertexModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_FragmentModule;
		fragShaderStageInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		if (topology == PrimitiveTopology::LineList || topology == PrimitiveTopology::LineStrip)
		{
			dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		}

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();
		
		VkVertexInputBindingDescription bindingDescription = Utils::GetBindingDescription(layout.VertexLayout);
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Utils::GetAttributeDescriptions(layout.VertexLayout);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = Utils::GetVkPrimitiveTopologyFromWirePrimitiveTopology(topology);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)vkd.SwapchainExtent.width;
		viewport.height = (float)vkd.SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = vkd.SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = vkd.MultisampleCount;
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.minSampleShading = 0.2f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		const std::vector<PushConstantInfo>& pushConstantInfos = layout.PushConstants;
		std::vector<VkPushConstantRange> pushConstantRanges;

		for (const auto& pushConstant : pushConstantInfos)
		{
			VkPushConstantRange& range = pushConstantRanges.emplace_back();
			range.size = pushConstant.Size;
			range.offset = pushConstant.Offset;
			range.stageFlags = Utils::GetVkShaderStageFromWireStage(pushConstant.Stage);
		}

		const std::vector<ShaderResourceInfo>& shaderResourceInfos = layout.ShaderResources;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		uint32_t i = 0;
		for (const auto& shaderResource : shaderResourceInfos)
		{
			VkDescriptorType descriptorType = Utils::GetVkDescriptorTypeFromWireResourceType(shaderResource.ResourceType);

			VkDescriptorSetLayoutBinding binding{};
			binding.binding = shaderResource.Binding;
			binding.descriptorType = descriptorType;
			binding.descriptorCount = shaderResource.ResourceCount;
			binding.stageFlags = Utils::GetVkShaderStageFromWireStage(shaderResource.Stage);
			binding.pImmutableSamplers = nullptr;

			bindings.push_back(binding);

			i++;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(vkd.Device, &layoutInfo, vkd.Allocator, &pipeline->m_SetLayout);
		VK_CHECK(result, "Failed to create Vulkan descriptor set layout!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = vkd.DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &pipeline->m_SetLayout;

		result = vkAllocateDescriptorSets(vkd.Device, &allocInfo, &pipeline->m_DescriptorSet);
		VK_CHECK(result, "Failed to allocate Vulkan descriptor sets!");

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &pipeline->m_SetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		result = vkCreatePipelineLayout(vkd.Device, &pipelineLayoutInfo, vkd.Allocator, &pipeline->m_PipelineLayout);
		VK_CHECK(result, "Failed to create Vulkan pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipeline->m_PipelineLayout;
		pipelineInfo.renderPass = vkd.RenderPass;
		pipelineInfo.subpass = 0;

		result = vkCreateGraphicsPipelines(vkd.Device, nullptr, 1, &pipelineInfo, vkd.Allocator, &pipeline->m_Pipeline);
		VK_CHECK(result, "Failed to create Vulkan graphics pipeline!");

		rbRef<GraphicsPipeline> ref = pipeline;
		vkd.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<GraphicsPipeline> VulkanShader::CreatePipeline(const InputLayout& layout, PrimitiveTopology topology, bool multiSample, rbRef<Framebuffer> framebuffer)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		VulkanGraphicsPipeline* pipeline = new VulkanGraphicsPipeline(m_Renderer);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_VertexModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_FragmentModule;
		fragShaderStageInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		if (topology == PrimitiveTopology::LineList || topology == PrimitiveTopology::LineStrip)
		{
			dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		}

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkVertexInputBindingDescription bindingDescription = Utils::GetBindingDescription(layout.VertexLayout);
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Utils::GetAttributeDescriptions(layout.VertexLayout);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = Utils::GetVkPrimitiveTopologyFromWirePrimitiveTopology(topology);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)vkd.SwapchainExtent.width;
		viewport.height = (float)vkd.SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = vkd.SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		if (multiSample)
		{
			multisampling.rasterizationSamples = Utils::GetMaxUsableSampleCount(m_Renderer);
			multisampling.sampleShadingEnable = VK_TRUE;
			multisampling.minSampleShading = 0.2f;
		}
		else
		{
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.minSampleShading = 0.0f;
		}
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		std::vector blendAttachments = { colorBlendAttachment };

		// TODO
		for (uint32_t i = 0; i < framebuffer->GetColorAttachmentCount() - 1; i++)
		{
			VkPipelineColorBlendAttachmentState attachment{};
			attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachment.blendEnable = VK_FALSE;

			blendAttachments.push_back(attachment);
		}
		
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = (uint32_t)blendAttachments.size();
		colorBlending.pAttachments = blendAttachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		const std::vector<PushConstantInfo>& pushConstantInfos = layout.PushConstants;
		std::vector<VkPushConstantRange> pushConstantRanges;

		for (const auto& pushConstant : pushConstantInfos)
		{
			VkPushConstantRange& range = pushConstantRanges.emplace_back();
			range.size = pushConstant.Size;
			range.offset = pushConstant.Offset;
			range.stageFlags = Utils::GetVkShaderStageFromWireStage(pushConstant.Stage);
		}

		const std::vector<ShaderResourceInfo>& shaderResourceInfos = layout.ShaderResources;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		uint32_t i = 0;
		for (const auto& shaderResource : shaderResourceInfos)
		{
			VkDescriptorType descriptorType = Utils::GetVkDescriptorTypeFromWireResourceType(shaderResource.ResourceType);

			VkDescriptorSetLayoutBinding binding{};
			binding.binding = i;
			binding.descriptorType = descriptorType;
			binding.descriptorCount = shaderResource.ResourceCount;
			binding.stageFlags = Utils::GetVkShaderStageFromWireStage(shaderResource.Stage);
			binding.pImmutableSamplers = nullptr;

			bindings.push_back(binding);

			i++;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(vkd.Device, &layoutInfo, vkd.Allocator, &pipeline->m_SetLayout);
		VK_CHECK(result, "Failed to create Vulkan descriptor set layout!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = vkd.DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &pipeline->m_SetLayout;

		result = vkAllocateDescriptorSets(vkd.Device, &allocInfo, &pipeline->m_DescriptorSet);
		VK_CHECK(result, "Failed to allocate Vulkan descriptor sets!");

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &pipeline->m_SetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		result = vkCreatePipelineLayout(vkd.Device, &pipelineLayoutInfo, vkd.Allocator, &pipeline->m_PipelineLayout);
		VK_CHECK(result, "Failed to create Vulkan pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipeline->m_PipelineLayout;
		pipelineInfo.renderPass = ((VulkanFramebuffer*)framebuffer.Get())->GetRenderPass();
		pipelineInfo.subpass = 0;

		result = vkCreateGraphicsPipelines(vkd.Device, nullptr, 1, &pipelineInfo, vkd.Allocator, &pipeline->m_Pipeline);
		VK_CHECK(result, "Failed to create Vulkan graphics pipeline!");

		rbRef<GraphicsPipeline> ref = pipeline;
		vkd.Resources.push_back(ref.GetResource());

		return ref;
	}

	void VulkanShader::Reload()
	{
		m_Renderer->SubmitResourceFree([vertexModule = m_VertexModule, fragmentModule = m_FragmentModule](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			if (vertexModule)
				vkDestroyShaderModule(vkd.Device, vertexModule, vkd.Allocator);
			if (fragmentModule)
				vkDestroyShaderModule(vkd.Device, fragmentModule, vkd.Allocator);
		});

		Utils::CreateCacheDirectory();

		std::string source = ReadFile(m_Filepath);
		std::array<std::string, 2> shaders = PreProcess(source);

		CompileOrGetVulkanBinaries(shaders);
		CreateShaderModules();
	}

	void VulkanShader::CompileOrGetVulkanBinaries(const std::array<std::string, 2>& sources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
		const bool debug = false;
		if (debug)
		{
			options.SetGenerateDebugInfo();
			options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}
		else
		{
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		}

		{ // vertex shader
			std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

			std::filesystem::path shaderFilePath = m_Filepath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + ".cached_vulkan.vert");

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				m_VertexData.resize(size / sizeof(uint32_t));
				in.read((char*)m_VertexData.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(sources[0], shaderc_glsl_vertex_shader, shaderFilePath.string().c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					std::cerr << module.GetErrorMessage() << std::endl;
					WR_ASSERT(false);
				}

				m_VertexData = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					out.write((char*)m_VertexData.data(), m_VertexData.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
		{ // fragment shader
			std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

			std::filesystem::path shaderFilePath = m_Filepath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + ".cached_vulkan.frag");

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())
			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);

				m_FragmentData.resize(size / sizeof(uint32_t));
				in.read((char*)m_FragmentData.data(), size);
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(sources[1], shaderc_glsl_fragment_shader, shaderFilePath.string().c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					std::cerr << module.GetErrorMessage() << std::endl;
					WR_ASSERT(false);
				}

				m_FragmentData = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					out.write((char*)m_FragmentData.data(), m_FragmentData.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		Reflect("Vertex Shader", m_VertexData);
		Reflect("Fragment Shader", m_FragmentData);
	}

	void VulkanShader::CreateShaderModules()
	{
		auto& vkd = m_Renderer->GetVulkanData();

		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = 4 * m_VertexData.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(m_VertexData.data());

			VkResult result = vkCreateShaderModule(vkd.Device, &createInfo, vkd.Allocator, &m_VertexModule);
			VK_CHECK(result, "Failed to create shader module!");
		}
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = 4 * m_FragmentData.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(m_FragmentData.data());

			VkResult result = vkCreateShaderModule(vkd.Device, &createInfo, vkd.Allocator, &m_FragmentModule);
			VK_CHECK(result, "Failed to create shader module!");
		}
	}

	void VulkanShader::Reflect(const std::string& name, const std::vector<uint32_t>& data) const
	{
		spirv_cross::Compiler compiler(data);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		WR_INFO("Shader::Reflect - ", name, " ", m_Filepath);
		WR_INFO("\t", resources.uniform_buffers.size(), " uniform buffer", (resources.uniform_buffers.size() == 1 ? "" : "s"));
		WR_INFO("\t", resources.sampled_images.size(), " resource", (resources.sampled_images.size() == 1 ? "" : "s"));

		if (resources.uniform_buffers.size() > 0)
			WR_INFO("Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = static_cast<int>(bufferType.member_types.size());

			WR_INFO("  ", resource.name);
			WR_INFO("\tSize = ", bufferSize);
			WR_INFO("\tBinding = ", binding);
			WR_INFO("\tMembers = ", memberCount);
		}

		if (resources.sampled_images.size() > 0)
			WR_INFO("Resources:");
		for (const auto& resource : resources.sampled_images)
		{
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			WR_INFO("\tID = ", resource.id );
			WR_INFO("\tBinding = ", binding);
		}
	}

	std::string VulkanShader::ReadFile(const std::filesystem::path& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				WR_ERROR("Could not read from file '", filepath, "'");
			}
		}
		else
		{
			WR_ERROR("Could not open file '", filepath, "'");
		}

		return result;
	}

	std::array<std::string, 2> VulkanShader::PreProcess(const std::string& source)
	{
		std::array<std::string, 2> result;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			WR_ASSERT(eol != std::string::npos);
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);

			WR_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" && "Invalid shader source type!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			WR_ASSERT(nextLinePos != std::string::npos);
			pos = source.find(typeToken, nextLinePos);

			if (type == "vertex")
				result[0] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
			else if (type == "fragment" || type == "pixel")
				result[1] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		return result;
	}

}
