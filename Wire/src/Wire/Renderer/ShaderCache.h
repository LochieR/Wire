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
        Pixel,
        Compute
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
        uint32_t SHA256[8];
        ShaderConfiguration Config;
        std::vector<ShaderObject> Objects;
    };

    struct ShaderInfo
    {
        std::filesystem::path Path;
        bool IsGraphics;
        std::string VertexOrComputeEntryPoint;
        std::string PixelEntryPoint;
    };

    struct ShaderCacheDesc
    {
        std::filesystem::path CachePath;
        std::vector<ShaderInfo> ShaderInfos;
    };

    struct ShaderResult
    {
        bool IsGraphics;
        ShaderObject VertexOrCompute;
        ShaderObject Pixel;
    };

    class ShaderCache
    {
    public:
        ShaderCache() = default;
        ShaderCache(const std::vector<ShaderGroup>& groups);

        void outputToFile(const std::filesystem::path& path);

        ShaderResult getShaderFromURL(const std::filesystem::path& path, RendererAPI api, bool isGraphics);

        const std::vector<ShaderGroup>& getGroups() const { return m_Groups; }

        static ShaderCache createFromFile(const std::filesystem::path& path);
        static ShaderCache createOrGetShaderCache(const ShaderCacheDesc& desc);
        static ShaderCache combineShaderCaches(const ShaderCache& lhs, const ShaderCache& rhs);
    private:
        uint32_t m_Version;
        std::vector<ShaderGroup> m_Groups;
    };

}
