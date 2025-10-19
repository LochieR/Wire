#include "ShaderCache.h"

#include "Renderer.h"
#include "ShaderCompiler.h"
#include "Wire/Core/Assert.h"
#include "Wire/Serialization/Stream.h"
#include "Wire/Serialization/SHA-256.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace wire {

#define HEADER_VER(major, minor, patch, build) (static_cast<uint32_t>(major) << 24) \
    | (static_cast<uint32_t>(minor) << 16) \
    | (static_cast<uint32_t>(patch) << 8)  \
    | (static_cast<uint32_t>(build))

    struct ShaderCacheHeader
    {
        // ID data
        const char AppID[4] = { 'W', 'I', 'R', 'E' };    // WIRE
        const char TypeID[4] = { 'S', 'C', 'C', 'H' };   // SCCH  (shader cache)
        const uint32_t Version = HEADER_VER(1, 0, 3, 0); // 1.0.3.0

        // cache data
        size_t GroupCount;
    };

    ShaderCache::ShaderCache(const std::vector<ShaderGroup>& groups)
        : m_Version(ShaderCacheHeader{}.Version), m_Groups(groups)
    {
    }

    static void WriteGroup(StreamWriter& stream, const ShaderGroup& group);
    static void WriteObject(StreamWriter& stream, const ShaderObject& object);
    static void ReadGroup(StreamReader& stream, ShaderGroup& group);
    static void ReadObject(StreamReader& stream, ShaderObject& object);

    void ShaderCache::outputToFile(const std::filesystem::path& path)
    {
        /*for (const auto& group : m_Groups)
        {
            if (group.Objects.size() != m_Groups[0].Objects.size())
            {
                WR_ASSERT(false, "Each group in a shader cache must have the same number of objects");
                return;
            }
        }*/

        ShaderCacheHeader header{};
        header.GroupCount = m_Groups.size();

        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);

        WR_ASSERT(file.good(), "Failed to open file!");

        StreamWriter stream(file);

        stream.writeRaw(header);

        /*for (const auto& group : m_Groups)
        {
            stream.writeRaw<size_t>(group.Objects.size());
        }*/

        stream.writeArray<ShaderGroup>(m_Groups, WriteGroup, false);

        file.close();
    }

    ShaderResult ShaderCache::getShaderFromURL(const std::filesystem::path& path, RendererAPI api, bool isGraphics)
    {
        std::string pathStr = path.string();
        WR_ASSERT(pathStr.contains("shadercache://"), "Invalid shadercache path! (must begin with shadercache://)");

        std::string name = pathStr.substr(14);

        ShaderResult result;

        for (const auto& group : m_Groups)
        {
            if (group.Name == name)
            {
                for (const auto& object : group.Objects)
                {
                    if (object.API == api)
                    {
                        if (isGraphics)
                        {
                            result.IsGraphics = true;
                            if (object.Type == ShaderType::Vertex)
                                result.VertexOrCompute = object;
                            if (object.Type == ShaderType::Pixel)
                                result.Pixel = object;
                        }
                        else
                        {
                            result.IsGraphics = false;
                            if (object.Type == ShaderType::Compute)
                                result.VertexOrCompute = object;
                        }
                    }
                }
            }
        }

        return result;
    }

    ShaderCache ShaderCache::createFromFile(const std::filesystem::path& path)
    {
        ShaderCache cache;

        std::ifstream file(path, std::ios::binary);
        StreamReader stream(file);

        ShaderCacheHeader header;
        stream.readRaw(header);

        cache.m_Version = header.Version;

        std::vector<ShaderGroup>& groups = cache.m_Groups;
        groups.resize(header.GroupCount);

        stream.readArray<ShaderGroup>(groups, ReadGroup, header.GroupCount);

        file.close();

        return cache;
    }

    static ShaderCacheHeader getHeader(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        StreamReader stream(file);

        ShaderCacheHeader header;
        stream.readRaw(header);

        file.close();

        return header;
    }

    ShaderCache ShaderCache::createOrGetShaderCache(const ShaderCacheDesc& desc)
    {
        if (!std::filesystem::exists(desc.CachePath))
        {
            std::vector<std::filesystem::path> graphicsPaths;
            std::vector<std::filesystem::path> computePaths;
            std::vector<std::string> vertexEntryPoints;
            std::vector<std::string> pixelEntryPoints;
            std::vector<std::string> computeEntryPoints;

            for (const auto& shaderInfo : desc.ShaderInfos)
            {
                if (shaderInfo.IsGraphics)
                {
                    graphicsPaths.push_back(shaderInfo.Path);

                    vertexEntryPoints.push_back(shaderInfo.VertexOrComputeEntryPoint);
                    pixelEntryPoints.push_back(shaderInfo.PixelEntryPoint);
                }
                else
                {
                    computePaths.push_back(shaderInfo.Path);
                    computeEntryPoints.push_back(shaderInfo.VertexOrComputeEntryPoint);
                }
            }

            if (graphicsPaths.empty() && computePaths.empty())
                return {};

            ShaderCache graphicsCache = ShaderCompiler::createShaderCacheHLSL(graphicsPaths, vertexEntryPoints, pixelEntryPoints);
            ShaderCache computeCache = ShaderCompiler::createShaderCacheHLSL(computePaths, computeEntryPoints);

            ShaderCache cache = combineShaderCaches(graphicsCache, computeCache);
            cache.outputToFile(desc.CachePath);

            return cache;
        }

        ShaderCacheHeader oldCacheHeader = getHeader(desc.CachePath);
        ShaderCacheHeader currentHeader{};

        if (oldCacheHeader.GroupCount != desc.ShaderInfos.size() || oldCacheHeader.Version != currentHeader.Version)
        {
            std::vector<std::filesystem::path> graphicsPaths;
            std::vector<std::filesystem::path> computePaths;
            std::vector<std::string> vertexEntryPoints;
            std::vector<std::string> pixelEntryPoints;
            std::vector<std::string> computeEntryPoints;

            for (const auto& shaderInfo : desc.ShaderInfos)
            {
                if (shaderInfo.IsGraphics)
                {
                    graphicsPaths.push_back(shaderInfo.Path);

                    vertexEntryPoints.push_back(shaderInfo.VertexOrComputeEntryPoint);
                    pixelEntryPoints.push_back(shaderInfo.PixelEntryPoint);
                }
                else
                {
                    computePaths.push_back(shaderInfo.Path);
                    computeEntryPoints.push_back(shaderInfo.VertexOrComputeEntryPoint);
                }
            }

            if (graphicsPaths.empty() && computePaths.empty())
                return {};
            
            ShaderCache graphicsCache = ShaderCompiler::createShaderCacheHLSL(graphicsPaths, vertexEntryPoints, pixelEntryPoints);
            ShaderCache computeCache = ShaderCompiler::createShaderCacheHLSL(computePaths, computeEntryPoints);

            ShaderCache cache = combineShaderCaches(graphicsCache, computeCache);
            cache.outputToFile(desc.CachePath);

            return cache;
        }

        ShaderCache oldCache = createFromFile(desc.CachePath);

        std::vector<uint32_t> toRecreate;
        std::vector<uint32_t> toRemove;

        for (uint32_t i = 0; i < desc.ShaderInfos.size(); i++)
        {
            const auto& shaderInfo = desc.ShaderInfos[i];
            std::string shaderName = shaderInfo.Path.filename().string();

            uint32_t cacheIndex = -1;
            for (uint32_t j = 0; j < oldCache.m_Groups.size(); j++)
            {
                if (oldCache.m_Groups[j].Name == shaderName)
                {
                    cacheIndex = j;
                    break;
                }
            }
            if (cacheIndex == -1) // shader was not in cache
            {
                toRecreate.push_back(i);
                continue;
            }

            constexpr ShaderConfiguration currentConfig =
#ifdef WR_DEBUG
                ShaderConfiguration::Debug;
#else
                ShaderConfiguration::Release;
#endif
            if (oldCache.m_Groups[cacheIndex].Config != currentConfig)
            {
                toRecreate.push_back(i);
                toRemove.push_back(cacheIndex);
                continue;
            }

            std::ifstream file(shaderInfo.Path);
            std::stringstream ss;
            ss << file.rdbuf();
            file.close();

            std::string shaderSource = ss.str();
            std::array<uint32_t, 8> shaderHash = generateSHA256(shaderSource);

            if (std::memcmp(oldCache.m_Groups[cacheIndex].SHA256, shaderHash.data(), sizeof(uint32_t) * 8) != 0)
            {
                toRecreate.push_back(i);
                toRemove.push_back(cacheIndex);
                continue;
            }
        }

        // sort in descending order
        std::sort(toRemove.rbegin(), toRemove.rend());

        for (uint32_t i : toRemove)
        {
            if (i >= 0 && i < oldCache.m_Groups.size())
                oldCache.m_Groups.erase(oldCache.m_Groups.begin() + i);
        }

        std::vector<std::filesystem::path> graphicsPaths;
        std::vector<std::filesystem::path> computePaths;
        std::vector<std::string> vertexEntryPoints;
        std::vector<std::string> pixelEntryPoints;
        std::vector<std::string> computeEntryPoints;

        for (uint32_t i : toRecreate)
        {
            if (desc.ShaderInfos[i].IsGraphics)
            {
                graphicsPaths.push_back(desc.ShaderInfos[i].Path);

                vertexEntryPoints.push_back(desc.ShaderInfos[i].VertexOrComputeEntryPoint);
                pixelEntryPoints.push_back(desc.ShaderInfos[i].PixelEntryPoint);
            }
            else
            {
                computePaths.push_back(desc.ShaderInfos[i].Path);
                computeEntryPoints.push_back(desc.ShaderInfos[i].VertexOrComputeEntryPoint);
            }
        }

        if (graphicsPaths.empty() && computePaths.empty())
            return oldCache;

        ShaderCache newGraphicsCache = ShaderCompiler::createShaderCacheHLSL(graphicsPaths, vertexEntryPoints, pixelEntryPoints);
        ShaderCache newComputeCache = ShaderCompiler::createShaderCacheHLSL(computePaths, computeEntryPoints);

        ShaderCache newCache = combineShaderCaches(newGraphicsCache, newComputeCache);

        ShaderCache result = combineShaderCaches(oldCache, newCache);
        result.outputToFile(desc.CachePath);

        return result;
    }

    ShaderCache ShaderCache::combineShaderCaches(const ShaderCache& lhs, const ShaderCache& rhs)
    {
        std::unordered_map<std::string, ShaderGroup> mergedMap;

        auto insertOrMerge = [&mergedMap](const std::vector<ShaderGroup>& groups)
        {
            for (const auto& group : groups)
            {
                auto it = mergedMap.find(group.Name);
                if (it != mergedMap.end())
                {
                    // Merge ShaderObjects if group name already exists
                    it->second.Objects.insert(it->second.Objects.end(), group.Objects.begin(), group.Objects.end());
                }
                else
                {
                    mergedMap[group.Name] = group;
                }
            }
        };

        insertOrMerge(lhs.m_Groups);
        insertOrMerge(rhs.m_Groups);

        std::vector<ShaderGroup> result;
        result.reserve(mergedMap.size());
        for (auto& pair : mergedMap)
        {
            result.push_back(std::move(pair.second));
        }

        return result;
    }

    void WriteGroup(StreamWriter& stream, const ShaderGroup& group)
    {
        stream.writeString(group.Name);
        stream.writeRaw(group.SHA256);
        stream.writeRaw((uint32_t)group.Config);
        stream.writeArray<ShaderObject>(group.Objects, WriteObject, true);
    }

    void WriteObject(StreamWriter& stream, const ShaderObject& object)
    {
        stream.writeRaw((uint32_t)object.API);
        stream.writeRaw((uint32_t)object.Type);
        stream.writeString(object.EntryPoint);
        stream.writeArray(object.Bytecode);
    }

    void ReadGroup(StreamReader& stream, ShaderGroup& group)
    {
        stream.readString(group.Name);
        stream.readRaw(group.SHA256);

        uint32_t config;
        stream.readRaw(config);
        group.Config = (ShaderConfiguration)config;

        stream.readArray<ShaderObject>(group.Objects, ReadObject);
    }

    void ReadObject(StreamReader& stream, ShaderObject& object)
    {
        uint32_t api;
        stream.readRaw(api);
        object.API = (RendererAPI)api;

        uint32_t type;
        stream.readRaw(type);
        object.Type = (ShaderType)type;

        stream.readString(object.EntryPoint);
        stream.readArray(object.Bytecode);
    }

}
