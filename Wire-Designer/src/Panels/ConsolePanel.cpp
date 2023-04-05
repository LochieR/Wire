#include "ConsolePanel.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Wire {

	ConsolePanel::ConsolePanel()
	{
	}

	static void AlignText(float offset = 1.0f)
	{
		using namespace ImGui;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		window->DC.CurrLineSize.y = ImMax(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
		window->DC.CurrLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, g.Style.FramePadding.y) + offset;
	}

	static void ScrollToBottom()
	{
		using namespace ImGui;

		float center_y_ratio = 1.0f;

		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		float spacing_y = ImMax(window->WindowPadding.y, g.Style.ItemSpacing.y);
		float target_pos_y = ImLerp(window->DC.CursorPosPrevLine.y - spacing_y, window->DC.CursorPosPrevLine.y + window->DC.PrevLineSize.y + spacing_y, center_y_ratio) + 100.0f;
		SetScrollFromPosY(window, target_pos_y - window->Pos.y, center_y_ratio); // Convert from absolute to local pos

		// Tweak: snap on edges when aiming at an item very close to the edge
		window->ScrollTargetEdgeSnapDist.y = ImMax(0.0f, window->WindowPadding.y - spacing_y);
	}

	void ConsolePanel::OnImGuiRender(bool* open)
	{		
		ImGui::Begin("Console", open);

		if (ImGui::Button("Clear"))
		{
			Clear();
		}

		ImGui::SameLine();

		ImGui::Checkbox("Clear On Play", &m_ClearOnPlay);

		// TODO: Add buttons which hide all of one type of message

		ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX;
		flags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuter;

		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(48.0f / 255.0f, 48.0f / 255.0f, 51.0f / 255.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(48.0f / 255.0f, 48.0f / 255.0f, 51.0f / 255.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGuiTableFlags headerFlags = ImGuiTableFlags_ScrollY;
		headerFlags |= ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV;
		headerFlags |= ImGuiTableFlags_BordersOuterV;

		if (ImGui::BeginTable("log_table_headers", 3, headerFlags, ImVec2(0.0f, ImGui::CalcTextSize("Text").y * 2.0f)))
		{
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Warning").x + 30.0f);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("00:00:00").x + 30.0f);
			ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthFixed,
				(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x) -
				(ImGui::CalcTextSize("Warning").x + 30.0f + ImGui::CalcTextSize("00:00:00").x + 30.0f));

			ImGui::TableNextRow(0, ImGui::CalcTextSize("Text").y * 2.0f);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(66.0f / 255.0f, 66.0f / 255.0f, 74.0f / 255.0f, 1.0f)));
			for (int column = 0; column < 3; column++)
			{
				ImGui::TableSetColumnIndex(column);
				if (column == 0)
				{
					AlignText(2.0f);
					ImGui::Text("Type");
				}
				if (column == 1)
				{
					AlignText(2.0f);
					ImGui::Text("Time");
				}
				if (column == 2)
				{
					AlignText();
					ImGui::Text("Message");
				}
			}

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("log_table", 4, flags))
		{
			ImGui::TableSetupColumn("Colour", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth, 0.1f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Warning").x + 32.0f);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("00:00:00").x + 39.0f);
			ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthFixed, 
				(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x) - 
				(ImGui::CalcTextSize("Warning").x + 32.0f + ImGui::CalcTextSize("00:00:00").x + 39.0f));

			for (ConsoleMessage msg : m_Messages)
			{
				ImGui::TableNextRow(0, ImGui::CalcTextSize("Example").y * 2.0f);
				for (int column = 0; column < 4; column++)
				{
					ImGui::TableSetColumnIndex(column);
					if (column == 0)
					{
						ImU32 red = ImGui::GetColorU32(ImVec4(255.0f / 255.0f, 79.0f / 255.0f, 79.0f / 255.0f, 1.0f));
						ImU32 yellow = ImGui::GetColorU32(ImVec4(255.0f / 255.0f, 227.0f / 255.0f, 15.0f / 255.0f, 1.0f));
						ImU32 blue = ImGui::GetColorU32(ImVec4(0.0f / 255.0f, 110.0f / 255.0f, 254.0f / 255.0f, 1.0f));
						ImU32 colour = msg.Level == LogLevel::Info ? blue : msg.Level == LogLevel::Warn ? yellow : red;

						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, colour);
					}
					if (column == 1)
					{
						AlignText();

						switch (msg.Level)
						{
						case LogLevel::Info:
							ImGui::Text("  Info");
							break;
						case LogLevel::Warn:
							ImGui::Text("  Warning");
							break;
						case LogLevel::Error:
							ImGui::Text("  Error");
							break;
						default:
							break;
						}
					}
					if (column == 2)
					{
						AlignText();
						ImGui::Text((" " + msg.Time).c_str());
					}
					if (column == 3)
					{
						std::string paddedString = "  " + msg.Message;

						float columnWidth = (ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x) - (ImGui::CalcTextSize("Warning").x + 30.0f + ImGui::CalcTextSize("00:00:00").x + 30.0f);
						float oneChar = ImGui::CalcTextSize(paddedString.c_str()).x / paddedString.size();
						if (oneChar * paddedString.size() > columnWidth - ImGui::GetStyle().ScrollbarSize)
						{
							float msgWidth = ImGui::CalcTextSize(paddedString.c_str()).x;
							float widthOver = msgWidth - columnWidth;
							int charsOver = (int)(widthOver / oneChar);
							std::string newMsg;
							for (int i = 0; i < paddedString.size() - charsOver - 7; i++)
							{
								if (i < paddedString.size() - 1)
									newMsg += paddedString[i];
								else
									break;
							}
							newMsg += "...";
							AlignText();
							ImGui::Text(newMsg.c_str());
						}
						else
						{
							AlignText();
							ImGui::Text(paddedString.c_str());
						}
					}
				}
			}
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ScrollToBottom();

			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(2);

		ImGui::End();
	}

	void ConsolePanel::Log(const ConsoleMessage& message)
	{
		m_Messages.push_back(message);
	}

	void ConsolePanel::Clear()
	{
		m_Messages.clear();
	}

	void ConsolePanel::Log(LogLevel level, std::string message)
	{
		auto now = std::chrono::system_clock::now();
		auto timer = std::chrono::system_clock::to_time_t(now);
		std::tm bt = *std::localtime(&timer);
		std::ostringstream oss;
		oss << std::put_time(&bt, "%H:%M:%S");
		std::string time = oss.str();
		Log({ level, time, message });
	}

	void ConsolePanel::OnSceneStart()
	{
		if (m_ClearOnPlay)
			Clear();
	}

}
