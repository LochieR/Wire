#include "wrpch.h"
#include "Project.h"

#include <yaml-cpp/yaml.h>

namespace Wire {

	const std::string g_CSharpProjectTemplate =
		"local WireRootDir = os.getenv(\"WIRE_ENV\")\n"
		"\n"
		"workspace \"{WR_PRJ_NAME}\"\n"
		"	architecture \"x86_64\"\n"
		"	startproject \"Wire-ScriptRuntime\"\n"
		"\n"
		"	configurations\n"
		"	{"
		"		\"Debug\",\n"
		"		\"Release\",\n"
		"		\"Dist\"\n"
		"	}\n"
		"\n"
		"	flags\n"
		"	{\n"
		"		\"MultiProcessorCompile\"\n"
		"	}\n"
		"\n"
		"project \"Wire-ScriptRuntime\"\n"
		"	kind \"SharedLib\"\n"
		"	language \"C#\"\n"
		"	dotnetframework \"4.7.2\"\n"
		"	namespace \"{WR_PRJ_NAME}\"\n"
		"\n"
		"	targetdir (\"Binaries\")\n"
		"	objdir (\"Intermediates\")\n"
		"\n"
		"	files\n"
		"	{\n"
		"		\"Assets/**.cs\",\n"
		"	}\n"
		"\n"
		"	links\n"
		"	{\n"
		"		\"Wire-ScriptCore\"\n"
		"	}\n"
		"\n"
		"	filter \"configurations:Debug\"\n"
		"		optimize \"off\"\n"
		"		symbols \"Default\"\n"
		"\n"
		"	filter \"configurations:Release\"\n"
		"		optimize \"on\"\n"
		"		symbols \"Default\"\n"
		"\n"
		"	filter \"configurations:Dist\"\n"
		"		optimize \"full\"\n"
		"		symbols \"off\"\n"
		"\n"
		"\n"
		"group \"Wire\"\n"
		"\n"
		"include WireRootDir .. \"/Wire-ScriptCore\"\n"
		"\n"
		"group \"\"\n";

	Project::Project(const std::string& name, const std::filesystem::path& path)
		: m_Name(name), m_Path(path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value << name;
		out << YAML::EndMap;

		std::filesystem::path usePath;

		if (!path.has_extension())
			usePath = std::filesystem::path(path.string() + ".wrpj");
		else
			usePath = path;

		std::ofstream fout(usePath);
		fout << out.c_str();

		std::filesystem::create_directory(usePath.remove_filename() / "Assets");

		std::ofstream csproj(usePath.remove_filename() / "premake5.lua");
		std::string premake = g_CSharpProjectTemplate;
		size_t startPos = 0;
		while ((startPos = premake.find("{WR_PRJ_NAME}", startPos)) != std::string::npos)
		{
			premake.replace(startPos, std::string("{WR_PRJ_NAME}").length(), name);
			startPos += name.length();
		}
		csproj << premake << std::endl;
		csproj.close();

#ifdef WR_PLATFORM_WINDOWS
		std::ofstream gen(usePath.remove_filename() / "Win-GenProjects.bat");
		gen << "@echo off" << std::endl << "pushd %~dp0\\" << std::endl << "call ..\\vendor\\premake\\bin\\premake5.exe vs2022" << std::endl << "popd" << std::endl;
		gen.close();
		system(((usePath.remove_filename() / "Win-GenProjects.bat").string() + " > nul").c_str());
#else
		#error "Platform is not currently supported!"
#endif
	}

	Ref<Project> Project::OpenProject(const std::filesystem::path& path)
	{
		std::ifstream stream(path);
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Project"])
			return CreateNullProject();

		std::string name = data["Project"].as<std::string>();
		Ref<Project> project = CreateRef<Project>();
		project->m_Path = path;
		project->m_Name = name;
		project->m_Null = false;

		return project;
	}

}
