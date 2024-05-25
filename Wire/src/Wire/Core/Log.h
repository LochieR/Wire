#pragma once

#include <vector>
#include <string>
#include <ostream>
#include <sstream>

namespace Coral {

	class String;

}

namespace Wire {

	class Logger
	{
	public:
		Logger(const std::string& name, std::ostream& stream);
		Logger(const std::string& name, std::ostream& stream, std::ostream& error);

		void LogStr(const std::string& message);
		void LogErrorStr(const std::string& message);
		template<typename... Args>
		void Log(Args&&... args)
		{
			std::string str;
			((str += ToStr(args)), ...);
			if (str[str.size() - 1] == '\n')
				str.pop_back();
			LogStr(str);
		}
		template<typename... Args>
		void LogError(Args&&... args)
		{
			std::string str;
			((str += ToStr(args)), ...);
			if (str[str.size() - 1] == '\n')
				str.pop_back();
			LogErrorStr(str);
		}

		const std::string& GetName() const { return m_Name; }
	private:
		template<typename T>
		static typename std::enable_if<true == std::is_convertible<T, std::string>::value, std::string>::type ToStr(const T& value)
		{
			std::ostringstream oss;
			oss << value;
			std::string val = oss.str();
			return val;
		}
	private:
		std::string m_Name;
		std::ostream& m_Stream;
		std::ostream& m_Error;
	};

	class Log
	{
	public:
		static void Init();

		static Logger* GetLogger() { return s_Logger.get(); }

		static void LogInfoStr(const std::string& message);
		static void LogWarningStr(const std::string& message);
		static void LogErrorStr(const std::string& message);

		template<typename... Args>
		static void LogInfo(Args&&... args)
		{
			std::string str;
			((str += ToStr(args)), ...);
			if (str[str.size() - 1] == '\n')
				str.pop_back();
			LogInfoStr(str);
		}

		template<typename... Args>
		static void LogWarning(Args&&... args)
		{
			std::string str;
			((str += ToStr(args)), ...);
			if (str[str.size() - 1] == '\n')
				str.pop_back();
			LogWarningStr(str);
		}

		template<typename... Args>
		static void LogError(Args&&... args)
		{
			std::string str;
			((str += ToStr(args)), ...);
			if (str[str.size() - 1] == '\n')
				str.pop_back();
			LogErrorStr(str);
		}

		static void PushTag(const std::string& tag);
		static void PopTag();

		static void CacheAndClearTags();
		static void PushAndClearCache();

		static const std::vector<std::string>& GetTags() { return s_Tags; }
	private:
		static std::string GetFormat();

		template<typename T>
		static std::string ToStr(const T& value)
		{
			std::ostringstream oss;
			oss << value;
			std::string val = oss.str();
			return val;
		}
	private:
		static std::shared_ptr<Logger> s_Logger;
		
		static std::vector<std::string> s_Tags;
		static std::vector<std::string> s_TagCache;
	};

	class Tag
	{
	public:
		Tag(const std::string& name);
		Tag(const std::string& name, int);
		~Tag();
	private:
		bool m_IsTemp;
	};

	std::ostream& operator<<(std::ostream& stream, const Coral::String& coralString);

}

#define COMBINE1(x, y) x##y
#define COMBINE(x, y) COMBINE1(x, y)
#define WR_TAG(tagName) ::Wire::Tag COMBINE(tagL, __LINE__)(tagName)
#define WR_TEMP_TAG(tagName) ::Wire::Tag COMBINE(tagTL, __LINE__)(tagName, 0)
#define WR_INFO(...) ::Wire::Log::LogInfo(__VA_ARGS__)
#define WR_WARNING(...) ::Wire::Log::LogWarning(__VA_ARGS__)
#define WR_ERROR(...) ::Wire::Log::LogError(__VA_ARGS__)
