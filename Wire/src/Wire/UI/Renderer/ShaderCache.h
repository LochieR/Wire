#pragma once

#include <vector>
#include <string>
#include <filesystem>

namespace wire {
	
    enum class RendererAPI
    {
        Vulkan = 0
    };

    enum class ShaderType
    {
        Vertex = 0,
        Pixel
    };

    enum class ShaderConfiguration
    {
        Debug = 0,
        Release
    };

	struct ShaderObject
	{
		RendererAPI API;
		ShaderType Type;
		std::string EntryPoint;
		std::vector<uint8_t> Bytecode;
	};

    struct ShaderGroup
    {
        std::string Name;
        char SHA256[32];
        ShaderConfiguration Config;
        std::vector<ShaderObject> Objects;
    };

    struct ShaderInfo
    {
        std::filesystem::path Path;
        std::string VertexEntryPoint;
        std::string PixelEntryPoint;
    };

    struct ShaderCacheDesc
    {
        std::filesystem::path CachePath;
        std::vector<ShaderInfo> ShaderInfos;
    };

    struct ShaderResult
    {
        ShaderObject Vertex;
        ShaderObject Pixel;
    };

    class ShaderCache
    {
    public:
        ShaderCache() = default;
        ShaderCache(const std::vector<ShaderGroup>& groups);

        void outputToFile(const std::filesystem::path& path);

        ShaderResult getShaderFromURL(const std::filesystem::path& path, RendererAPI api);

        const std::vector<ShaderGroup>& getGroups() const { return m_Groups; }

        static ShaderCache createFromFile(const std::filesystem::path& path);
        static ShaderCache createOrGetShaderCache(const ShaderCacheDesc& desc);
        static ShaderCache combineShaderCaches(const ShaderCache& lhs, const ShaderCache& rhs);
    private:
        uint32_t m_Version;
        std::vector<ShaderGroup> m_Groups;
    };

}
