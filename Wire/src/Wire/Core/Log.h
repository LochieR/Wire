#pragma once

#include <iostream>
#include <ostream>
#include <string>
#include <format>
#include <vector>
#include <fstream>
#include <mutex>

namespace wire {

	class LogConfig
	{
	public:
		static LogConfig& instance()
		{
			static LogConfig config;
			return config;
		}

		const std::string& getFormat() const { return m_Format; }
		void setFormat(const std::string& fmt) { m_Format = fmt; }

		void clearOutputs()
		{
			m_Outputs.clear();
			m_FileStreams.clear();
		}

		void addOutput(std::ostream& os) { m_Outputs.push_back(&os); }
		const std::vector<std::ostream*> getOutputs() const { return m_Outputs; }

		void addFileOutput(const std::string& filename);
	private:
		std::vector<std::ostream*> m_Outputs;
		std::string m_Format = "%c[%H:%M:%S]%m%c";

		std::vector<std::unique_ptr<std::ofstream>> m_FileStreams;
	};

	class LogFormatter
	{
	public:
		static std::string formatMessage(const std::string& message, const std::string& formatString, const std::string& colorCode);
	};

	class Logger
	{
	public:
		static void setupLog(const std::vector<std::ostream*>& streams, const std::vector<std::string>& files, const std::string& format);

		template<typename... Args>
		static void logInfoFormat(std::format_string<Args...> fmt, Args&&... args)
		{
			log("\033[1;34m", std::format(fmt, std::forward<Args>(args)...));
		}

		template<typename... Args>
		static void logWarningFormat(std::format_string<Args...> fmt, Args&&... args)
		{
			log("\033[1;33m", std::format(fmt, std::forward<Args>(args)...));
		}

		template<typename... Args>
		static void logErrorFormat(std::format_string<Args...> fmt, Args&&... args)
		{
			log("\033[1;31m", std::format(fmt, std::forward<Args>(args)...));
		}
	private:
		static void log(const std::string& color, const std::string& msg);
	private:
		inline static std::mutex s_LogMutex;
	};

}

#define WR_SETUP_LOG(outStreams, outFiles, format) ::wire::Logger::setupLog(outStreams, outFiles, format);
#define WR_INFO(msg, ...) ::wire::Logger::logInfoFormat(msg, ##__VA_ARGS__)
#define WR_WARN(msg, ...) ::wire::Logger::logWarningFormat(msg, ##__VA_ARGS__)
#define WR_ERROR(msg, ...) ::wire::Logger::logErrorFormat(msg, ##__VA_ARGS__)
