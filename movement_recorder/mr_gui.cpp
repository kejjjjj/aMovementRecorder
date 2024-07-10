#include "mr_main.hpp"
#include "main.hpp"
#include "mr_playback.hpp"
#include "mr_record.hpp"
#include "dvar/dvar.hpp"
#include "bg/bg_pmove_simulation.hpp"
#include <utils/resolution.hpp>

CPlaybackGui::CPlaybackGui(CPlayback& owner, const std::string name) : m_refOwner(owner), m_sName(name) {
	m_iCurrentSlowdown = static_cast<int>(m_refOwner.m_objHeader.m_bJumpSlowdownEnable);
}
CPlaybackGui::~CPlaybackGui() = default;

CGuiMovementRecorder::CGuiMovementRecorder(CMovementRecorder& recorder)
	: m_oRefMovementRecorder(recorder) {}
CGuiMovementRecorder::~CGuiMovementRecorder() = default;


bool CPlaybackGui::Render()
{
	ImGui::TextCentered(m_sName.c_str());
	ImGui::Separator();

	ImGui::Text("Frames: %u", m_refOwner.cmds.size());
	ImGui::Text("Speed: %i", m_refOwner.m_objHeader.m_iSpeed);

	constexpr const char* arr[] = { "Disabled", "Enabled", "Both"};

	ImGui::SetNextItemWidth(100);
	if (ImGui::Combo("Jump Slowdown", &m_iCurrentSlowdown, arr, 3)) {
		m_refOwner.m_objHeader.m_bJumpSlowdownEnable = CPlayback::slowdown_t(m_iCurrentSlowdown);
		m_uChanges++;
	}
	ImGui::Tooltip("Choose whether this jump can be performed with or without jump slowdown (or with both?)");
	

	if (m_uChanges) {
		if (ImGui::ButtonCentered("Apply changes")) {
			const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
			const auto writer = CPlaybackIOWriter(&m_refOwner, mapname + "\\" + m_sName);

			if (!writer.Write()) {
				Com_Printf("^1failed!\n");
			}
			else {
				Com_Printf("^2saved!\n");
				m_uChanges = {};

				//refresh all playbacks
				CMovementRecorderIO io(*CStaticMovementRecorder::Instance);
				return io.RefreshAllLevelPlaybacks();
			}
		}

	}

	return false;
}

void CGuiMovementRecorder::RenderLevelRecordings()
{

	const auto window_length = ImGui::GetWindowSize().x - ImGui::GetCursorPos().x - ImGui::GetStyle().FramePadding.x;
	ImGui::BeginChild("LevelPlaybacks", { window_length, static_cast<float>(adjust_from_480(480 / 4)) });

	const auto& movementRecorder = m_oRefMovementRecorder;
	size_t n = 0u;
	for (const auto& [name, playback] : movementRecorder.LevelPlaybacks) {

		ImGui::SetNextItemWidth(100);
		if (ImGui::Selectable(name.c_str(), n == m_uSelectedIndex)) {
			m_uSelectedIndex = n;
			m_pItem = std::make_unique<CPlaybackGui>(*playback, name);
		}

		n++;
	}

	ImGui::EndChild();

	if (m_pItem) {

		if (m_pItem->Render()) {
			m_pItem.reset();
			m_uSelectedIndex = 0;
		}
	}
	else
		ImGui::TextCentered("Nothing selected... :(");

}
