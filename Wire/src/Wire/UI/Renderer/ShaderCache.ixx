module;

#include <vector>
#include <string>
#include <filesystem>

export module wire.ui.renderer:shaderCache;

namespace wire {
	
    export enum class RendererAPI
    {
        Vulkan = 0
    };

    export enum class ShaderType
    {
        Vertex = 0,
        Pixel
    };

    export enum class ShaderConfiguration
    {
        Debug = 0,
        Release
    };

	export struct ShaderObject
	{
		RendererAPI API;
		ShaderType Type;
		std::string EntryPoint;
		std::vector<uint8_t> Bytecode;
	};

    export struct ShaderGroup
    {
        std::string Name;
        char SHA256[32];
        ShaderConfiguration Config;
        std::vector<ShaderObject> Objects;
    };

    export struct ShaderInfo
    {
        std::filesystem::path Path;
        std::string VertexEntryPoint;
        std::string PixelEntryPoint;
    };

    export struct ShaderCacheDesc
    {
        std::filesystem::path CachePath;
        std::vector<ShaderInfo> ShaderInfos;
    };

    export struct ShaderResult
    {
        ShaderObject Vertex;
        ShaderObject Pixel;
    };

    export class ShaderCache
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
