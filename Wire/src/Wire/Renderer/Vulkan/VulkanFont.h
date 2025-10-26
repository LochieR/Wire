#pragma once

#include "Wire/Renderer/Device.h"

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <vector>
#include <filesystem>

namespace wire {

    class VulkanFont : public Font
    {
    public:
        VulkanFont() = default;
        VulkanFont(Device* device, const std::filesystem::path& path, std::string_view debugName = {}, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
        VulkanFont(Device* device, const NaiveFont& naive);
        virtual ~VulkanFont();

        virtual const std::shared_ptr<Texture2D>& getAtlasTexture() const override { return m_AtlasTexture; }
        virtual const MSDFData& getMSDFData() const override { return *m_Data; }
    protected:
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
        void createFontData(const NaiveFont& naive);
    private:
        Device* m_Device = nullptr;

        std::string m_DebugName;

        MSDFData* m_Data = nullptr;
        std::shared_ptr<Texture2D> m_AtlasTexture = nullptr;
    };

}
