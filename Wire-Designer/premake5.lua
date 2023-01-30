project "Wire-Designer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.location}/src/**.h",
		"%{prj.location}/src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Wire/vendor/spdlog/include",
		"%{wks.location}/Wire/src",
		"%{wks.location}/Wire/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}",
	}

	links
	{
		"Wire",
		-- "opengl32.lib",
	}

	filter "system:windows"
		systemversion "latest"

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