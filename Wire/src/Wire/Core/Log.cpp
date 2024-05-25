#include "wrpch.h"
#include "Log.h"

#include <ctime>
#include <iomanip>
#include <iostream>

#include <Coral/String.hpp>

namespace Wire {

	Logger::Logger(const std::string& name, std::ostream& stream)
		: m_Name(name), m_Stream(stream), m_Error(stream)
	{
	}

	Logger::Logger(const std::string& name, std::ostream& stream, std::ostream& error)
		: m_Name(name), m_Stream(stream), m_Error(error)
	{
	}

	void Logger::LogStr(const std::string& message)
	{
		m_Stream << message << std::endl;
	}

	void Logger::LogErrorStr(const std::string& message)
	{
		m_Error << message << std::endl;
	}

	std::shared_ptr<Logger> Log::s_Logger = nullptr;
	std::vector<std::string> Log::s_Tags;
	std::vector<std::string> Log::s_TagCache;

	void Log::Init()
	{
		s_Logger = std::make_shared<Logger>("Wire", std::cout, std::cerr);
	}

	void Log::LogInfoStr(const std::string& message)
	{
		s_Logger->Log(GetFormat(), " ", message);
	}

	void Log::LogWarningStr(const std::string& message)
	{
		s_Logger->Log("\u001b[38;5;172m", GetFormat(), " ", message, "\u001b[0m");
	}

	void Log::LogErrorStr(const std::string& message)
	{
		s_Logger->LogError("\u001b[38;5;196m", GetFormat(), " ", message, "\u001b[0m");
	}

	void Log::PushTag(const std::string& tag)
	{
		s_Tags.push_back(tag);
	}

	void Log::PopTag()
	{
		s_Tags.pop_back();
	}

	void Log::CacheAndClearTags()
	{
		s_TagCache.clear();
		s_TagCache = s_Tags;
		s_Tags.clear();
	}

	void Log::PushAndClearCache()
	{
		s_Tags.clear();
		s_Tags = s_TagCache;
		s_TagCache.clear();
	}

	std::string Log::GetFormat()
	{
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);

		std::ostringstream oss;
		oss << "[" << s_Logger->GetName() << "@";
		oss << std::put_time(&tm, "%H:%M:%S");

		if (s_Tags.size())
			oss << " ";

		int i = 0;
		for (const auto& tag : s_Tags)
		{
			oss << tag;
			if (i != s_Tags.size() - 1)
				oss << ":";
			i++;
		}

		oss << "]";

		return oss.str();
	}

	Tag::Tag(const std::string& name)
		: m_IsTemp(false)
	{
		Log::PushTag(name);
	}

	Tag::Tag(const std::string& name, int)
		: m_IsTemp(true)
	{
		Log::CacheAndClearTags();
		Log::PushTag(name);
	}

	Tag::~Tag()
	{
		Log::PopTag();

		if (m_IsTemp && Log::GetTags().size() == 0)
		{
			Log::PushAndClearCache();
		}
	}

	std::ostream& operator<<(std::ostream& stream, const Coral::String& coralString)
	{
		return stream << std::string(coralString);
	}

}
