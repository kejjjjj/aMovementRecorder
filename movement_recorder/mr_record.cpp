#include "mr_record.hpp"
#include <cg/cg_angles.hpp>
#include "cg/cg_local.hpp"
#include <bg/bg_pmove_simulation.hpp>
#include <algorithm>
#include <com/com_channel.hpp>
#include <iostream>
#include <ranges>


CRecorder::~CRecorder() = default;
void CRecorder::Record(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd) noexcept
{
	if (IsWaiting()) {
		m_iStartTimer = std::clamp(m_iStartTimer - (cmd->serverTime - oldcmd->serverTime), 0, m_iStartTimer);
		
		//don't allow any inputs while waiting
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->buttons = 0;
		
		return;
	}

	//if ((cmd->forwardmove == 0 && cmd->rightmove == 0) && m_bStartFromMove && data.empty()) {
	//	return;
	//}

	playback_cmd rcmd;

	rcmd.buttons = cmd->buttons;
	rcmd.forwardmove = cmd->forwardmove;
	rcmd.rightmove = cmd->rightmove;
	rcmd.oldTime = oldcmd->serverTime;
	rcmd.offhand = cmd->offHandIndex;
	rcmd.origin = ps->origin;
	rcmd.serverTime = cmd->serverTime;
	rcmd.velocity = ps->velocity;
	rcmd.weapon = cmd->weapon;
	rcmd.viewangles = CG_AnglesFromCmd(cmd);

	data.push_back(std::move(rcmd));
}
std::vector<playback_cmd>&& CRecorder::StopRecording() noexcept {

	//for ([[maybe_unused]]const auto i : std::views::iota(0u, 10u)) 
	//	InsertDummyCmd();

	return std::move(data);
}

void CRecorder::InsertDummyCmd()
{
	if (data.empty())
		return;

	const auto& front = data.front();
	playback_cmd copy = front;

	copy.serverTime -= 3;
	copy.oldTime -= 3;
	copy.forwardmove = 0;
	copy.rightmove = 0;

	data.insert(data.begin(), copy);
}
