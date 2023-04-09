#pragma once

namespace Wire {

	class Project
	{
	public:
		Project() {}
		Project(const std::string& name, const std::filesystem::path& path);
		
		std::filesystem::path GetPath() { return m_Path; }
		std::filesystem::path GetPath() const { return m_Path; }

		std::filesystem::path GetDir() { return m_Path.remove_filename(); }

		std::string GetName() { return m_Name; }
		std::string GetName() const { return m_Name; }

		static Ref<Project> OpenProject(const std::filesystem::path& path);
		static Ref<Project> CreateNullProject() { Ref<Project> project = CreateRef<Project>(); project->m_Null = true; project->m_Path = "|nopath|"; return project; }
	private:
		bool m_Null = false;

		std::string m_Name = std::string();
		std::filesystem::path m_Path;
	};

}
