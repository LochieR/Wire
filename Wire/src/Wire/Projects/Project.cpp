#include "wrpch.h"
#include "Project.h"

#include <yaml-cpp/yaml.h>

namespace Wire {

	Project::Project(const std::string& name, const std::filesystem::path& path)
		: m_Name(name), m_Path(path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value << name;

		std::ofstream fout(path);
		fout << out.c_str();
	}

	Project Project::OpenProject(const std::filesystem::path& path)
	{
		std::ifstream stream(path);
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Project"])
			return CreateNullProject();

		std::string name = data["Project"].as<std::string>();
		Project project(name, path);

		return project;
	}

}
