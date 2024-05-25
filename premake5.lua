VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}

IncludeDir["GLFW"] = "%{wks.location}/Wire/deps/GLFW/include"
IncludeDir["glm"] = "%{wks.location}/Wire/deps/glm"
IncludeDir["entt"] = "%{wks.location}/Wire/deps/entt/include"
IncludeDir["imgui"] = "%{wks.location}/Wire/deps/imgui"
IncludeDir["implot"] = "%{wks.location}/Wire/deps/implot"
IncludeDir["stb_image"] = "%{wks.location}/Wire/deps/stb_image"
IncludeDir["msdfgen"] = "%{wks.location}/Wire/deps/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Wire/deps/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["Coral"] = "%{wks.location}/Wire/deps/Coral/Coral.Native/Include"
IncludeDir["PortAudio"] = "%{wks.location}/Wire/deps/portaudio/include"
IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["Vulkan"] = "%{VULKAN_SDK}/Lib"

Library = {}

Library["Vulkan"] = "%{LibraryDir.Vulkan}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.Vulkan}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.Vulkan}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.Vulkan}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.Vulkan}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-glsl.lib"

workspace "Wire"
	architecture "x86_64"
	startproject "Wire-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Wire/deps/GLFW"
	include "Wire/deps/ImGui"
	include "Wire/deps/portaudio"
	include "Wire/deps/msdf-atlas-gen"

	group "Dependencies/Coral"
		include "Wire/deps/Coral/Coral.Native"
		include "Wire/deps/Coral/Coral.Managed"
	group "Dependencies"
group ""

group "Core"

	project "Wire"
		location "Wire"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		staticruntime "off"

		targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
		objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

		pchheader "wrpch.h"
		pchsource "%{prj.location}/src/wrpch.cpp"

		files
		{
			"%{prj.location}/src/**.h",
			"%{prj.location}/src/**.cpp",
			"%{prj.location}/deps/stb_image/**.h",
			"%{prj.location}/deps/stb_image/**.cpp",
			"%{prj.location}/deps/implot/**.h",
			"%{prj.location}/deps/implot/**.cpp",
			"%{prj.location}/deps/glm/glm/**.hpp",
			"%{prj.location}/deps/glm/glm/**.inl",
		}

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"GLFW_INCLUDE_NONE"
		}

		includedirs
		{
			"%{prj.location}/src"
		}

		externalincludedirs
		{
			"%{IncludeDir.GLFW}",
			"%{IncludeDir.glm}",
			"%{IncludeDir.entt}",
			"%{IncludeDir.imgui}",
			"%{IncludeDir.implot}",
			"%{IncludeDir.stb_image}",
			"%{IncludeDir.msdfgen}",
			"%{IncludeDir.msdf_atlas_gen}",
			"%{IncludeDir.Coral}",
			"%{IncludeDir.PortAudio}",
			"%{IncludeDir.Vulkan}",
		}

		links
		{
			"GLFW",
			"ImGui",
			"Coral.Native",
			"PortAudio",
			"msdf-atlas-gen",
			"%{Library.Vulkan}"
		}

		filter "files:%{prj.location}/deps/implot/**.cpp"

		filter "system:windows"
			systemversion "latest"

		filter "system:macosx"
			systemversion "10.13"

		filter "configurations:Debug"
			defines "WR_DEBUG"
			runtime "Debug"
			symbols "on"

			links
			{
				"%{Library.ShaderC_Debug}",
				"%{Library.SPIRV_Cross_Debug}",
				"%{Library.SPIRV_Cross_GLSL_Debug}"
			}

		filter "configurations:Release"
			defines "WR_RELEASE"
			runtime "Release"
			optimize "on"

			links
			{
				"%{Library.ShaderC_Release}",
				"%{Library.SPIRV_Cross_Release}",
				"%{Library.SPIRV_Cross_GLSL_Release}"
			}

		filter "configurations:Dist"
			defines "WR_DIST"
			runtime "Release"
			optimize "on"

			links
			{
				"%{Library.ShaderC_Release}",
				"%{Library.SPIRV_Cross_Release}",
				"%{Library.SPIRV_Cross_GLSL_Release}"
			}


	project "Wire-ScriptCore"
		location "Wire-ScriptCore"
		kind "SharedLib"
		language "C#"
		dotnetframework "net7.0"
		namespace "Wire"
		clr "Unsafe"

		propertytags {
			{ "AppendTargetFrameworkToOutputPath", "false" },
			{ "Nullable", "enable" }
		}

		targetdir ("%{wks.location}/Wire-Editor/Resources/Scripts")
		objdir ("%{wks.location}/Wire-Editor/Resources/Scripts/intermediates")

		files
		{
			"%{prj.location}/Source/**.cs",
			"%{prj.location}/Properties/**.cs"
		}

		links
		{
			"Coral.Managed"
		}

		filter "configurations:Debug"
			optimize "off"
			symbols "Default"

		filter "configurations:Release"
			optimize "on"
			symbols "Default"

		filter "configurations:Dist"
			optimize "full"
			symbols "off"

group ""

group "Tools"

	project "Wire-Editor"
		location "Wire-Editor"
		language "C++"
		cppdialect "C++20"
		staticruntime "off"

		targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
		objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

		files
		{
			"%{prj.location}/src/**.h",
			"%{prj.location}/src/**.cpp"
		}

		includedirs
		{
			"%{wks.location}/Wire/src"
		}

		externalincludedirs
		{
			"%{IncludeDir.glm}",
			"%{IncludeDir.GLFW}",
			"%{IncludeDir.entt}",
			"%{IncludeDir.imgui}",
			"%{IncludeDir.implot}",
			"%{IncludeDir.Vulkan}"
		}

		links
		{
			"Wire"
		}

		filter "system:windows"
			systemversion "latest"

			defines { "NOMINMAX" }

		filter "system:macosx"
			systemversion "10.13"

		filter "configurations:Debug"
			kind "ConsoleApp"
			defines "WR_DEBUG"
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			kind "ConsoleApp"
			defines "WR_RELEASE"
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			kind "WindowedApp"
			defines "WR_DIST"
			runtime "Release"
			optimize "on"

group ""
