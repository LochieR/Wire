workspace "Sandbox"
	architecture "x86_64"
	startproject "Wire-ScriptRuntime"

	configurations
	{		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

project "Wire-ScriptRuntime"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"
	namespace "Sandbox"

	targetdir ("Binaries")
	objdir ("Intermediates")

	files
	{
		"Assets/**.cs",
	}

	links
	{
		"Wire-ScriptCore"
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


group "Wire"

include "../../Wire-ScriptCore"

group ""

