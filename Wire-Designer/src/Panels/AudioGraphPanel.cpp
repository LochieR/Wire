#include "AudioGraphPanel.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace Wire {

	struct Delegate : public GraphEditor::Delegate
	{
		virtual bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override
		{
			return true;
		}

		virtual void SelectNode(GraphEditor::NodeIndex index, bool selected) override
		{
			m_Nodes[index].Selected = selected;
		}

		virtual void MoveSelectedNodes(const ImVec2 delta) override
		{
			for (auto& node : m_Nodes)
			{
				if (!node.Selected)
					continue;

				node.X += delta.x;
				node.Y += delta.y;
			}
		}

		virtual void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override
		{
			m_Links.push_back({ inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex });
		}

		virtual void DelLink(GraphEditor::LinkIndex linkIndex) override
		{
			m_Links.erase(m_Links.begin() + linkIndex);
		}

		virtual void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override
		{
			drawList->AddLine(rectangle.Min, rectangle.Max, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f)));
			drawList->AddText(rectangle.Min, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f)), "Draw");
		}

		virtual void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override
		{
		}

		virtual const size_t GetTemplateCount() override
		{
			return m_Templates.size();
		}

		virtual const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override
		{
			return m_Templates[index];
		}

		virtual const size_t GetNodeCount() override
		{
			return m_Nodes.size();
		}

		virtual const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override
		{
			const auto& node = m_Nodes[index];
			return GraphEditor::Node
			{
				node.Name,
				node.TemplateIndex,
				ImRect(ImVec2(node.X, node.Y), ImVec2(node.X + 200, node.Y + 200)),
				node.Selected
			};
		}

		virtual const size_t GetLinkCount() override
		{
			return m_Links.size();
		}

		virtual const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override
		{
			return m_Links[index];
		}
	private:
		struct Node
		{
			const char* Name;
			GraphEditor::TemplateIndex TemplateIndex;
			float X, Y;
			bool Selected;
		};
	private:
		std::vector<GraphEditor::Template> m_Templates = 
		{
			{
				ImGui::GetColorU32(ImVec4(160.0f / 255.0f, 160.0f / 255.0f, 180.0f / 255.0f, 1.0f)), // Header colour
				ImGui::GetColorU32(ImVec4(100.0f / 255.0f, 100.0f / 255.0f, 140.0f / 255.0f, 1.0f)), // Background colour
				ImGui::GetColorU32(ImVec4(110.0f / 255.0f, 110.0f / 255.0f, 150.0f / 255.0f, 1.0f)), // Background colour over?
				1,																					 // Input count
				nullptr,																			 // Input names
				nullptr,																			 // Input colours
				2,																					 // Output count
				nullptr,																			 // Output names
				nullptr																				 // Output colours
			},
			{
				ImGui::GetColorU32(ImVec4(160.0f / 255.0f, 160.0f / 255.0f, 180.0f / 255.0f, 1.0f)), // Header colour
				ImGui::GetColorU32(ImVec4(100.0f / 255.0f, 100.0f / 255.0f, 140.0f / 255.0f, 1.0f)), // Background colour
				ImGui::GetColorU32(ImVec4(110.0f / 255.0f, 110.0f / 255.0f, 150.0f / 255.0f, 1.0f)), // Background colour over?
				6,																					 // Input count
				nullptr,																			 // Input names
				nullptr,																			 // Input colours
				3,																					 // Output count
				nullptr,																			 // Output names
				nullptr																				 // Output colours
			}
		};

		std::vector<Node> m_Nodes = {
			{
				"Example Node 1",
				0, -400, -220, false
			},
			{
				"Example Node 2",
				1, 0, 0, false
			}
		};

		std::vector<GraphEditor::Link> m_Links = { { 0, 0, 1, 0} };
	};

	AudioGraphPanel::AudioGraphPanel()
	{
		m_Open = true;
	}

	AudioGraphPanel::~AudioGraphPanel()
	{
	}

	void AudioGraphPanel::OnImGuiRender()
	{
		static GraphEditor::Options options;
		static Delegate delegate;
		static GraphEditor::ViewState viewState;
		static GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;

		if (m_Open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4, 4 });
			ImGui::Begin("Audio Graph", &m_Open, 0);
			GraphEditor::Show(delegate, options, viewState, true, &fit);
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}

	void AudioGraphPanel::SetContext(const Ref<Scene>& context)
	{
	}

	bool* AudioGraphPanel::GetOpen()
	{
		return &m_Open;
	}

}
