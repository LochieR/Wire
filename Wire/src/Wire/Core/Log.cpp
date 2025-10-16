#include "Log.h"

#include <chrono>

namespace wire {

	void LogConfig::addFileOutput(const std::string& filename)
	{
		auto fs = std::make_unique<std::ofstream>(filename, std::ios::app);
		if (fs->is_open())
		{
			m_Outputs.push_back(fs.get());
			m_FileStreams.push_back(std::move(fs));
		}
	}

	std::string LogFormatter::formatMessage(const std::string& message, const std::string& formatString, const std::string& colorCode)
	{
		auto now = std::chrono::system_clock::now();
		auto time_t_now = std::chrono::system_clock::to_time_t(now);
		std::tm tm_now;
#ifdef _MSC_VER
		localtime_s(&tm_now, &time_t_now);
#else
		localtime_r(&time_t_now, &tm_now);
#endif

		bool color = false;

		std::ostringstream output;
		for (size_t i = 0; i < formatString.size(); ++i)
		{
			if (formatString[i] == '%' && i + 1 < formatString.size())
			{
				switch (formatString[i + 1])
				{
				case 'H':
					output << std::setw(2) << std::setfill('0') << tm_now.tm_hour;
					break;
				case 'M':
					output << std::setw(2) << std::setfill('0') << tm_now.tm_min;
					break;
				case 'S':
					output << std::setw(2) << std::setfill('0') << tm_now.tm_sec;
					break;
				case 'm':
					output << message;
					break;
				case 'c':
					if (!color)
						output << colorCode;
					else
						output << "\033[0m";
					color = !color;
					break;
				default:
					output << '%' << formatString[i + 1];
					break;
				}
				i++;
			}
			else
				output << formatString[i];
		}
		return output.str();
	}

	void Logger::setupLog(const std::vector<std::ostream*>& streams, const std::vector<std::string>& files, const std::string& format)
	{
		auto& config = LogConfig::instance();
		config.clearOutputs();

		for (auto* stream : streams)
			config.addOutput(*stream);

		for (const auto& file : files)
			config.addFileOutput(file);

		config.setFormat(format);
	}

	void Logger::log(const std::string& color, const std::string& msg)
	{
		std::lock_guard<std::mutex> lock(s_LogMutex);
		const auto& config = LogConfig::instance();
		std::string formatted = LogFormatter::formatMessage(msg, config.getFormat(), color);
		for (auto* out : config.getOutputs())
		{
			(*out) << formatted << "\033[0m" << std::endl;
		}
	}

}
