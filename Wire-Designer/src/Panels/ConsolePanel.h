#pragma once

#include "Panel.h"

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

	class ConsolePanel : public Panel
	{
	public:
		ConsolePanel();

		virtual void OnImGuiRender() override;
		virtual void SetContext(const Ref<Scene>& context) override;

		virtual bool* GetOpen() override;

		void Log(const ConsoleMessage& message);
		void Log(LogLevel level, std::string message);
		
		void Clear();
		void OnSceneStart();
	private:
		std::vector<ConsoleMessage> m_Messages;

		bool m_ClearOnPlay = false;

		bool m_Open;
	};

}
