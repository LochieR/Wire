local WireRootDir = os.getenv("WIRE_ENV")

project "Wire-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"
	namespace "Wire"

	targetdir (WireRootDir .. "/Wire-Designer/Resources/Scripts")
	objdir (WireRootDir .. "/Wire-Designer/Resources/Scripts/Intermediates")

	files
	{
		"Source/**.cs",
		"Properties/**.cs"
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