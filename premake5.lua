VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}

IncludeDir["GLFW"] = "%{wks.location}/external/GLFW/include"
IncludeDir["glm"] = "%{wks.location}/external/glm"
IncludeDir["msdfgen"] = "%{wks.location}/external/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/external/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["Vulkan"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["Vulkan"] = "%{VULKAN_SDK}/Lib"

Library = {}

Library["Vulkan"] = "%{LibraryDir.Vulkan}/vulkan-1.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.Vulkan}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-glsld.lib"
Library["SPIRV_Cross_HLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-hlsld.lib"
Library["SPIRV_Cross_MSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-msld.lib"

Library["ShaderC_Release"] = "%{LibraryDir.Vulkan}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-glsl.lib"
Library["SPIRV_Cross_HLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-hlsl.lib"
Library["SPIRV_Cross_MSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-msl.lib"

workspace "wire"
    architecture "x86_64"
    startproject "bloom"

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
    include "external/GLFW"
	include "external/msdf-atlas-gen"
group ""

project "wire"
    location "wire"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp",
		"%{prj.location}/src/**.ixx",
		"external/stb_image/stb_image.h",
		"external/stb_image/stb_image.cpp"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE",
        "NOMINMAX",
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING"
    }

    includedirs
	{
		"%{prj.location}/src",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.msdfgen}",
	}

    externalincludedirs
	{
		"external/stb_image",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
        "%{IncludeDir.Vulkan}"
    }

    links
    {
        "GLFW",
		"msdf-atlas-gen",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
		systemversion "latest"

		links
		{
			"dwmapi.lib"
		}

    filter "configurations:Debug"
		defines "WR_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"%{Library.SPIRV_Cross_HLSL_Debug}",
			"%{Library.SPIRV_Cross_MSL_Debug}"
		}

    filter "configurations:Release"
		defines "WR_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.SPIRV_Cross_HLSL_Release}",
			"%{Library.SPIRV_Cross_MSL_Release}",
		}

    filter "configurations:Dist"
		defines "WR_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.SPIRV_Cross_HLSL_Release}",
			"%{Library.SPIRV_Cross_MSL_Release}",
		}

project "bloom"
    location "bloom"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp",
		"%{prj.location}/src/**.ixx",
	}

    externalincludedirs
	{
		"%{IncludeDir.glm}",
    }

    links
    {
        "wire"
    }

    filter "system:windows"
		systemversion "latest"

		defines { "NOMINMAX" }

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