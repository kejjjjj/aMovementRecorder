#include "mr_main.hpp"
#include "main.hpp"
#include "mr_playback.hpp"
#include "mr_record.hpp"
#include "dvar/dvar.hpp"
#include "bg/bg_pmove_simulation.hpp"

CPlaybackGui::CPlaybackGui(CPlayback& owner, const std::string name) : m_refOwner(owner), m_sName(name) {
	m_iCurrentSlowdown = static_cast<int>(m_refOwner.m_objHeader.m_bJumpSlowdownEnable);
}
CPlaybackGui::~CPlaybackGui() = default;

void CPlaybackGui::Render()
{
	ImGui::TextCentered(m_sName.c_str());
	ImGui::Separator();

	ImGui::Text("frames: %u", m_refOwner.cmds.size());
	ImGui::Text("speed: %i", m_refOwner.m_objHeader.m_iSpeed);

	constexpr const char* arr[] = { "Disabled", "Enabled", "Both"};

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
				CStaticMovementRecorder::m_bPlaybacksLoaded = false;
			}



		}

	}

}
CGuiMovementRecorder::~CGuiMovementRecorder() = default;
void CGuiMovementRecorder::Gui_RenderLocals()
{
	auto n = size_t(0);
	for (auto& [name, playback] : LevelPlaybacks) {
		ImGui::SetNextItemWidth(100);
		if (ImGui::Selectable(name.c_str(), n == m_uSelectedIndex)) {
			m_uSelectedIndex = n;
			m_pItem = std::make_unique<CPlaybackGui>(*playback, name);
		}
		n++;
	}

	if (m_pItem)
		m_pItem->Render();
	else
		ImGui::TextCentered("Much empty... :(");

}
void CGuiMovementRecorder::OnDisconnect()
{
	m_pItem = {};
	m_uSelectedIndex = {};
	return CMovementRecorder::OnDisconnect();
}
