#pragma once

#include "IResource.h"
#include "Texture2D.h"

#include <string>
#include <vector>
#include <filesystem>

namespace wire {

    struct MSDFData;

    class Font : public IResource
    {
    public:
        virtual ~Font() = default;

        virtual const std::shared_ptr<Texture2D>& getAtlasTexture() const = 0;
        virtual const MSDFData& getMSDFData() const = 0;
    };

    struct NaiveFont
    {
        std::string Name;
        uint32_t MinChar = 0x0020, MaxChar = 0x00FF;
        size_t FontFileSize;
        uint8_t* FontFileData;
        size_t AtlasSize;
        uint32_t* AtlasData;
        uint32_t Width, Height;

        static NaiveFont create(const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
        void release();
    };

}
