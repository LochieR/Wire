#pragma once

#include "Wire/Core/Application.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

namespace bloom {

	struct ModelVertex
	{
		glm::vec4 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		glm::vec3 Normal;

		bool operator==(const ModelVertex& other) const
		{
			return Position == other.Position && Color == other.Color && TexCoord == other.TexCoord && Normal == other.Normal;
		}
	};

	class EngineLayer : public wire::Layer
	{
	public:
		EngineLayer() = default;
		~EngineLayer() = default;

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onUpdate(float timestep) override;
		virtual void onEvent(wire::Event& event) override;
	private:
		std::shared_ptr<wire::Device> m_Device = nullptr;

        std::shared_ptr<wire::RenderPass> m_RenderPass = nullptr;
        
		std::shared_ptr<wire::Texture2D> m_ModelTexture = nullptr;
		std::shared_ptr<wire::Sampler> m_ModelSampler = nullptr;

		std::vector<ModelVertex> m_ModelVertices;
		std::vector<uint32_t> m_ModelIndices;
		std::shared_ptr<wire::Buffer> m_ModelVertexBuffer = nullptr;
		std::shared_ptr<wire::Buffer> m_ModelIndexBuffer = nullptr;
		std::shared_ptr<wire::Buffer> m_ModelUniformBuffer = nullptr;
		glm::mat4* m_ModelUniformData = nullptr;

        std::shared_ptr<wire::ShaderResourceLayout> m_ModelResourceLayout = nullptr;
        std::shared_ptr<wire::ShaderResource> m_ModelResource = nullptr;
		std::shared_ptr<wire::GraphicsPipeline> m_ModelPipeline = nullptr;

		std::vector<wire::CommandList> m_CommandLists;
	};

}

namespace std {

    template<> struct hash<bloom::ModelVertex>
    {
        std::size_t operator()(const bloom::ModelVertex& vertex) const
        {
			auto hash_combine = [](std::size_t& seed, std::size_t value)
			{
				seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			};

            std::size_t seed = 0;
            hash_combine(seed, std::hash<glm::vec4>{}(vertex.Position));
            hash_combine(seed, std::hash<glm::vec4>{}(vertex.Color));
            hash_combine(seed, std::hash<glm::vec2>{}(vertex.TexCoord));
            hash_combine(seed, std::hash<glm::vec3>{}(vertex.Normal));
            return seed;
        }
    };
}
