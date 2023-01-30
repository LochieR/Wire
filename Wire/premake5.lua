project "Wire"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.location}/src",
		"%{prj.location}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.rtaudio}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"rtaudio",
		"yaml-cpp",
		"opengl32.lib"
	}

	filter "files:%{wks.location}/Wire/vendor/ImGuizmo/**.cpp"
		flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
		}

	filter "configurations:Debug"
		defines "WR_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "WR_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "WR_DIST"
		runtime "Release"
		optimize "on"
