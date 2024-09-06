#include "dvar/dvar.hpp"
#include "movement_recorder/mr_main.hpp"
#include "net/im_defaults.hpp"
#include "net/nvar_table.hpp"
#include "r/gui/r_main_gui.hpp"
#include "r_movementrecorder.hpp"
#include "shared/sv_shared.hpp"

using namespace std::string_literals;

CMovementRecorderWindow::CMovementRecorderWindow(const std::string& name)
	: CGuiElement(name) {

	m_oKeybinds.emplace_back(std::make_unique<ImKeybind>("id_mr_record", "mr_record", "toggle recording"));
	m_oKeybinds.emplace_back(std::make_unique<ImKeybind>("id_mr_recordps", "mr_recordPlayerState", "toggle big file size recordings (for long segmented recordings)"));

	m_oKeybinds.emplace_back(std::make_unique<ImKeybind>("id_mr_playback", "mr_playback", "start playback"));
	m_oKeybinds.emplace_back(std::make_unique<ImKeybind>("id_mr_clear", "mr_clear", "clear temporary recording (unsaved recording)"));


}
CMovementRecorderWindow::~CMovementRecorderWindow() = default;

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

	if (m_oKeybinds.size()) {
		ImGui::Text("Keybinds");
		for (auto& keybind : m_oKeybinds) {
			keybind->Render();
		}

		ImGui::Text("use mr_save <filename> to save a temporary recording");
		ImGui::Text("use mr_teleportTo <filename> to teleport to the playback's origin");
		ImGui::Text("use mr_advance <frame count> to change the starting position (while editing)");

		ImGui::Separator();
		ImGui::NewLine();
	}

	ImGui::BeginGroup();
	GUI_RenderNVars();
	ImGui::EndGroup();


	//ImGui::Separator();

	ImGui::SameLine();

	ImGui::BeginGroup();

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
	ImGui::EndGroup();

	if (const auto promod_mode = Dvar_FindMalleableVar("promod_mode")) {
		
		if ("strat"s == promod_mode->current.string) {
			ImGui::Separator();
			ImGui::Text("it should be noted that the recorder is unable to detect\n"
				"position loading if this mod doesn't use \"openscriptmenu cj load\"");
		}
	}

}

