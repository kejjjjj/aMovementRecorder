#include "mr_record.hpp"
#include <cg/cg_angles.hpp>
#include "cg/cg_local.hpp"
#include <bg/bg_pmove_simulation.hpp>
#include <algorithm>
#include <com/com_channel.hpp>
#include <iostream>
#include <ranges>
#include <dvar/dvar.hpp>


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

	//const bool hasVelocity = fvec3(ps->velocity).mag_sq() != 0.000000f;
	//if ((hasVelocity || cmd->forwardmove == 0 && cmd->rightmove == 0) && m_bStartFromMove && data.empty()) {
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
	rcmd.viewangles = CG_GetClientAngles();
	rcmd.delta_angles = ps->delta_angles;
	rcmd.cmd_angles = cmd->angles;

	rcmd.delta_angles.y = (ps->delta_angles[YAW]);



	data.push_back(std::move(rcmd));
}
std::vector<playback_cmd>&& CRecorder::StopRecording() noexcept {
	return std::move(data);
}
playback_cmd* CRecorder::GetCurrentCmd()
{
	return data.empty() ? nullptr :  &data.back();
}
