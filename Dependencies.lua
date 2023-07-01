VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Wire/vendor/stb_image"
IncludeDir["yaml_cpp"] = "%{wks.location}/Wire/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/Wire/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Wire/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Wire/vendor/ImGui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Wire/vendor/ImGuizmo"
IncludeDir["glm"] = "%{wks.location}/Wire/vendor/glm"
IncludeDir["entt"] = "%{wks.location}/Wire/vendor/entt/include"
IncludeDir["mono"] = "%{wks.location}/Wire/vendor/mono/include"
IncludeDir["filewatch"] = "%{wks.location}/Wire/vendor/filewatch"
IncludeDir["msdfgen"] = "%{wks.location}/Wire/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Wire/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["OpenAL_Soft1"] = "%{wks.location}/Wire/vendor/OpenAL-Soft/include"
IncludeDir["OpenAL_Soft2"] = "%{wks.location}/Wire/vendor/OpenAL-Soft/src"
IncludeDir["OpenAL_Soft3"] = "%{wks.location}/Wire/vendor/OpenAL-Soft/src/common"
IncludeDir["alhelpers"] = "%{wks.location}/Wire/vendor/alhelpers"
IncludeDir["libogg"] = "%{wks.location}/Wire/vendor/libogg/include"
IncludeDir["Vorbis"] = "%{wks.location}/Wire/vendor/Vorbis/include"
IncludeDir["minimp3"] = "%{wks.location}/Wire/vendor/minimp3"
IncludeDir["shaderc"] = "%{wks.location}/Wire/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Wire/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["mono"] = "%{wks.location}/Wire/vendor/mono/lib/%{cfg.buildcfg}"
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["mono"] = "%{LibraryDir.mono}/libmono-static-sgen.lib"

Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["Bcrypt"] = "Bcrypt.lib"
