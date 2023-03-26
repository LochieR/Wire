include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Wire"
	architecture "x86_64"
	startproject "Wire-Designer"

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
	include "vendor/premake"
	include "Wire/vendor/GLFW"
	include "Wire/vendor/Glad"
	include "Wire/vendor/imgui"
	include "Wire/vendor/yaml-cpp"
	include "Wire/vendor/rtaudio"
group ""

group "Core"
	include "Wire"
	include "Wire-ScriptCore"
group ""

group "Misc"
	include "Sandbox"
group ""

group "Tools"
	include "Wire-Designer"
group ""
