#include "bg/bg_pmove_simulation.hpp"
#include "cg/cg_angles.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "com/com_channel.hpp"
#include "dvar/dvar.hpp"
#include "mr_main.hpp"
#include "mr_record.hpp"

//#include "sv/sv_client.hpp"

#include <algorithm>
#include <ranges>

CRecorder::~CRecorder() = default;
void CRecorder::Record(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd) noexcept
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
	rcmd.offhand = cmd->offHandIndex;
	rcmd.origin = ps->origin;
	rcmd.velocity = ps->velocity;
	rcmd.weapon = cmd->weapon;
	rcmd.viewangles = CG_GetClientAngles();
	rcmd.delta_angles = ps->delta_angles;
	rcmd.cmd_angles = cmd->angles;
	rcmd.serverTime = cmd->serverTime;
	rcmd.oldTime = oldcmd->serverTime;

	data.push_back(std::move(rcmd));
}
std::vector<playback_cmd>&& CRecorder::StopRecording() noexcept {
	return std::move(data);
}
playback_cmd* CRecorder::GetCurrentCmd()
{
	return data.empty() ? nullptr :  &data.back();
}
