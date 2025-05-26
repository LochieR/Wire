module;

#include "Wire/Core/Assert.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

module wire.ui.renderer:shaderCache;

import :renderer;
import :shaderCompiler;
import wire.serialization;

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
        const uint32_t Version = HEADER_VER(1, 0, 0, 0); // 1.0.0.0

        // cache data
        size_t GroupCount;
        size_t ObjectPerGroupCount;
    };

    ShaderCache::ShaderCache(const std::vector<ShaderGroup>& groups)
        : m_Groups(groups)
    {
    }

    static void WriteGroup(StreamWriter& stream, const ShaderGroup& group);
    static void WriteObject(StreamWriter& stream, const ShaderObject& object);
    static void ReadGroup(StreamReader& stream, ShaderGroup& group);
    static void ReadObject(StreamReader& stream, ShaderObject& object);

    void ShaderCache::outputToFile(const std::filesystem::path& path)
    {
        for (const auto& group : m_Groups)
        {
            if (group.Objects.size() != m_Groups[0].Objects.size())
            {
                WR_ASSERT(false, "Each group in a shader cache must have the same number of objects");
                return;
            }
        }

        ShaderCacheHeader header{};
        header.GroupCount = m_Groups.size();
        header.ObjectPerGroupCount = m_Groups[0].Objects.size();

        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);

        WR_ASSERT(file.good(), "Failed to open file!");

        StreamWriter stream(file);

        stream.writeRaw(header);
        stream.writeArray<ShaderGroup>(m_Groups, WriteGroup, false);

        file.close();
    }

    ShaderResult ShaderCache::getShaderFromURL(const std::filesystem::path& path, RendererAPI api)
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
                        if (object.Type == ShaderType::Vertex)
                            result.Vertex = object;
                        if (object.Type == ShaderType::Pixel)
                            result.Pixel = object;
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

        std::vector<ShaderGroup>& groups = cache.m_Groups;

        stream.readArray<ShaderGroup>(groups, ReadGroup, header.GroupCount);

        return cache;
    }

    ShaderCache ShaderCache::createOrGetShaderCache(const ShaderCacheDesc& desc)
    {
        if (!std::filesystem::exists(desc.CachePath))
        {
            std::vector<std::filesystem::path> paths;
            std::vector<std::string> vertexEntryPoints;
            std::vector<std::string> pixelEntryPoints;

            for (const auto& shaderInfo : desc.ShaderInfos)
            {
                paths.push_back(shaderInfo.Path);
                vertexEntryPoints.push_back(shaderInfo.VertexEntryPoint);
                pixelEntryPoints.push_back(shaderInfo.PixelEntryPoint);
            }

            ShaderCache cache = ShaderCompiler::createShaderCacheHLSL(paths, vertexEntryPoints, pixelEntryPoints);
            cache.outputToFile(desc.CachePath);

            return cache;
        }

        ShaderCache oldCache = createFromFile(desc.CachePath);

        if (oldCache.m_Groups.size() != desc.ShaderInfos.size())
        {
            std::vector<std::filesystem::path> paths;
            std::vector<std::string> vertexEntryPoints;
            std::vector<std::string> pixelEntryPoints;

            for (const auto& shaderInfo : desc.ShaderInfos)
            {
                paths.push_back(shaderInfo.Path);
                vertexEntryPoints.push_back(shaderInfo.VertexEntryPoint);
                pixelEntryPoints.push_back(shaderInfo.PixelEntryPoint);
            }

            ShaderCache cache = ShaderCompiler::createShaderCacheHLSL(paths, vertexEntryPoints, pixelEntryPoints);
            cache.outputToFile(desc.CachePath);

            return cache;
        }

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

            std::ifstream file(shaderInfo.Path);
            std::stringstream ss;
            ss << file.rdbuf();
            file.close();

            std::string shaderSource = ss.str();
            std::string shaderHash = generateSHA256(shaderSource);

            char sha256[32] = {};
            std::memcpy(sha256, shaderHash.data(), sizeof(char) * 32);

            if (std::memcmp(oldCache.m_Groups[cacheIndex].SHA256, sha256, sizeof(char) * 32) != 0)
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

        std::vector<std::filesystem::path> paths;
        std::vector<std::string> vertexEntryPoints;
        std::vector<std::string> pixelEntryPoints;

        for (uint32_t i : toRecreate)
        {
            paths.push_back(desc.ShaderInfos[i].Path);
            vertexEntryPoints.push_back(desc.ShaderInfos[i].VertexEntryPoint);
            pixelEntryPoints.push_back(desc.ShaderInfos[i].PixelEntryPoint);
        }

        ShaderCache newCache = ShaderCompiler::createShaderCacheHLSL(paths, vertexEntryPoints, pixelEntryPoints);

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
