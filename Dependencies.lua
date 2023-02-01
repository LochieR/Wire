VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Wire/vendor/stb_image"
IncludeDir["yaml_cpp"] = "%{wks.location}/Wire/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/Wire/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Wire/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Wire/vendor/ImGui"
IncludeDir["rtaudio"] = "%{wks.location}/Wire/vendor/rtaudio"
IncludeDir["ImGuizmo"] = "%{wks.location}/Wire/vendor/ImGuizmo"
IncludeDir["glm"] = "%{wks.location}/Wire/vendor/glm"
IncludeDir["entt"] = "%{wks.location}/Wire/vendor/entt/include"
IncludeDir["shaderc"] = "%{wks.location}/Wire/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Wire/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"
