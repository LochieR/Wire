include "./vendor/premake/premake_customization/solution_items.lua"

workspace "Wire"
	architecture "x64"
	startproject "Wire-Designer"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	
	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Wire/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Wire/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Wire/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Wire/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Wire/vendor/stb_image"
IncludeDir["rtaudio"] = "%{wks.location}/Wire/vendor/rtaudio"
IncludeDir["entt"] = "%{wks.location}/Wire/vendor/entt/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Wire/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Wire/vendor/ImGuizmo"

group "Dependencies"
	include "Wire/vendor/GLFW"
	include "Wire/vendor/Glad"
	include "Wire/vendor/imgui"
	include "Wire/vendor/rtaudio"
	include "Wire/vendor/yaml-cpp"
group ""

include "Wire"
include "Sandbox"
include "Wire-Designer"
