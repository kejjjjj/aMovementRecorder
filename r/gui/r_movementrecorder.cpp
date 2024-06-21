#include "main.hpp"
#include "r_movementrecorder.hpp"
#include "net/nvar_table.hpp"
#include "movement_recorder/mr_main.hpp"
#include <r/gui/r_main_gui.hpp>


CMovementRecorderWindow::CMovementRecorderWindow(const std::string& name)
	: CGuiElement(name) {

}

void CMovementRecorderWindow::Render()
{
	auto table = NVarTables::Get(NVAR_TABLE_NAME);

	for (auto& v : *table) {

		auto& [key, value] = v;

		if (value->IsImNVar()) {
			std::string _v_ = key + "_menu";
			ImGui::BeginChild(_v_.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
			value->RenderImNVar();
			ImGui::EndChild();
		}

	}

	ImGui::Separator();

	ImGui::BeginChild("MovementRecorderCategories",
		ImVec2(ImGui::GetWindowSize().x - ImGui::GetCursorPos().x - ImGui::GetStyle().FramePadding.x, 0),
		ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
		
	if (ImGui::BeginTabBar("playbacks")) {

		if (ImGui::BeginTabItem("Local")) {

			CStaticMovementRecorder::Get()->Gui_RenderLocals();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Online")) {
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}


	ImGui::EndChild();


}

