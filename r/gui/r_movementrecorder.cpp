#include "main.hpp"
#include "r_movementrecorder.hpp"
#include "net/nvar_table.hpp"
#include "movement_recorder/mr_main.hpp"
#include <r/gui/r_main_gui.hpp>
#include <iostream>
#include <shared/sv_shared.hpp>


CMovementRecorderWindow::CMovementRecorderWindow(const std::string& name)
	: CGuiElement(name) {

}

void CMovementRecorderWindow::Render()
{
#if(!DEBUG_SUPPORT)
	static auto func = CMain::Shared::GetFunctionSafe("GetContext");

	if (!func) {
		func = CMain::Shared::GetFunctionSafe("GetContext");
		return;
	}

	ImGui::SetCurrentContext(func->As<ImGuiContext*>()->Call());

#endif

	const auto table = NVarTables::Get(NVAR_TABLE_NAME);

	if (!table)
		return;

	for (const auto& v : *table) {

		const auto& [key, value] = v;

		if (value && value->IsImNVar()) {
			std::string _v_ = key + "_menu";
			ImGui::BeginChild(_v_.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
			value->RenderImNVar();
			ImGui::EndChild();
		}

	}

	ImGui::Separator();

	const auto window_length = ImGui::GetWindowSize().x - ImGui::GetCursorPos().x - ImGui::GetStyle().FramePadding.x;

	ImGui::BeginChild("MovementRecorderCategories",
		ImVec2(window_length, 0),
		ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
		
	if (ImGui::BeginTabBar("playbacks")) {

		if (ImGui::BeginTabItem("Local")) {

			if (CStaticMovementRecorder::Instance.get()) {
				static auto gui = CGuiMovementRecorder(CStaticMovementRecorder::Instance.get());

				if(!gui.m_oRefMovementRecorder)
					gui = CGuiMovementRecorder(CStaticMovementRecorder::Instance.get());

				gui.RenderLevelRecordings();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}


	ImGui::EndChild();


}

