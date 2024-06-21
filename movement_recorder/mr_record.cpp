#include "mr_record.hpp"
#include <cg/cg_angles.hpp>
#include "cg/cg_local.hpp"
#include <bg/bg_pmove_simulation.hpp>


CRecorder::~CRecorder() = default;
void CRecorder::Record(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd) noexcept
{
	if ((cmd->forwardmove == 0 && cmd->rightmove == 0) && m_bStartFromMove && data.empty()) {
		return;
	}

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
	rcmd.viewangles = ps->viewangles;

	for (int i = 0; i < 3; i++) {

		float delta = AngleDelta(ps->delta_angles[i], SHORT2ANGLE(cmd->angles[i]));
		float real_delta = AngleDelta(delta, ps->delta_angles[i]);

		rcmd.deltas[i] = ps->delta_angles[i] - real_delta;
	}

	data.push_back(std::move(rcmd));
}
std::vector<playback_cmd>&& CRecorder::StopRecording() noexcept {
	return std::move(data);
}
