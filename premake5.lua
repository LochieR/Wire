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
	include "Wire/vendor/GLFW"
	include "Wire/vendor/Glad"
	include "Wire/vendor/imgui"
	include "Wire/vendor/yaml-cpp"
	include "Wire/vendor/msdf-atlas-gen"
	
	group "Dependencies/Audio"
		include "Wire/vendor/libogg"
		include "Wire/vendor/OpenAL-Soft"
		include "Wire/vendor/Vorbis"
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
