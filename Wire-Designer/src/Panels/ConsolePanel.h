#pragma once

#include <vector>
#include <string>

namespace Wire {

	enum class LogLevel
	{
		Info = 0,
		Warn,
		Error,
	};

	struct ConsoleMessage
	{
		LogLevel Level;
		std::string Time;
		std::string Message;
	};

	class ConsolePanel
	{
	public:
		ConsolePanel();

		void OnImGuiRender(bool* open);

		void Log(const ConsoleMessage& message);
		void Log(LogLevel level, std::string message);
		
		void Clear();
		void OnSceneStart();
	private:
		std::vector<ConsoleMessage> m_Messages;

		bool m_ClearOnPlay = false;
	};

}
