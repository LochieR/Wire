project "Wire"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "wrpch.h"
	pchsource "src/wrpch.cpp"

	files
	{
		"%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp",
		"%{prj.location}/vendor/stb_image/**.h",
		"%{prj.location}/vendor/stb_image/**.cpp",
		"%{prj.location}/vendor/glm/glm/**.hpp",
		"%{prj.location}/vendor/glm/glm/**.inl",
		"%{prj.location}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.location}/vendor/ImGuizmo/ImGuizmo.cpp",
		"%{prj.location}/vendor/alhelpers/alhelpers.h",
		"%{prj.location}/vendor/alhelpers/alhelpers.cpp",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
		"AL_LIBTYPE_STATIC"
	}

	includedirs
	{
		"%{prj.location}/src",
		"%{prj.location}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.mono}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.OpenAL_Soft1}",
		"%{IncludeDir.OpenAL_Soft2}",
		"%{IncludeDir.OpenAL_Soft3}",
		"%{IncludeDir.alhelpers}",
		"%{IncludeDir.libogg}",
		"%{IncludeDir.Vorbis}",
		"%{IncludeDir.minimp3}",
		"%{IncludeDir.VulkanSDK}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"msdf-atlas-gen",
		"yaml-cpp",
		"OpenAL-Soft",
		"Vorbis",

		"%{Library.mono}"
	}

	filter "files:vendor/ImGuizmo/**.cpp"
		flags { "NoPCH" }

	filter "files:vendor/alhelpers/**.cpp"
		flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		links
		{
			"%{Library.WinSock}",
			"%{Library.WinMM}",
			"%{Library.WinVersion}",
			"%{Library.Bcrypt}",
			"opengl32.lib"
		}

	filter "system:macosx"
		systemversion "10.14"

		links
		{
			"OpenGL.framework",
			"Cocoa.framework",
			"IOKit.framework"
		}

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
