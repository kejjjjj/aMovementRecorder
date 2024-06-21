#include "bg/bg_pmove_simulation.hpp"
#include "cg/cg_client.hpp"
#include "movement_recorder/mr_record.hpp"
#include "mr_lineup.hpp"
#include "mr_main.hpp"
#include "mr_playback.hpp"
#include "utils/functions.hpp"
#include <algorithm>
#include <bg/bg_pmove.hpp>
#include <cg/cg_angles.hpp>
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>
#include <cl/cl_input.hpp>
#include <dvar/dvar.hpp>
#include <net/nvar_table.hpp>
#include <Windows.h>
#include <cg/cg_trace.hpp>

constexpr int LINEUP_FPS = 333;

CLineup::CLineup(const fvec3& target, const fvec3& destination_angles, float lineup_distance= NVar_FindMalleableVar<float>("Lineup distance")->Get()):
	m_vecDestination(target), 
	m_vecDestinationAngles(destination_angles.normalize180()), 
	m_fLineupAccuracy(std::clamp(lineup_distance, 0.000000f, lineup_distance))
{

	m_vecOldOrigin = cgs->predictedPlayerState.origin;
	m_vecTargetAngles = cgs->predictedPlayerState.viewangles;

	m_fTotalDistance = m_vecOldOrigin.dist(target);
	
	//50% chance to lineup while crouched
	m_iCmdButtons = cmdEnums::crouch * bool(std::round(random(1.f)));
}


bool CLineup::Update(usercmd_s* cmd, usercmd_s* oldcmd) noexcept
{
	if (!CanPathfind() || WASD_PRESSED()) {
		m_eState = gave_up;
		return false;
	}

	if (m_uAccuracyTestFrames > 0)
		return --m_uAccuracyTestFrames, true;

	if (m_vecDestination.dist(cgs->predictedPlayerState.origin) <= m_fLineupAccuracy
		&& fvec2(cgs->predictedPlayerState.velocity).MagSq() == 0.f) {
		m_eState = finished;
		return false;
	}

	if (Finished())
		return false;


	UpdateViewangles();
	UpdateOrigin();

	m_fYawToTarget = AngleNormalize180((m_vecDestination - fvec3(cgs->predictedPlayerState.origin)).toangles().y);

	MoveCloser(cmd, oldcmd);

	CreatePlayback(cmd, oldcmd);

	return true;

}
void CLineup::UpdateViewangles()
{
	const fvec3& viewangles = (fvec3&)cgs->predictedPlayerState.viewangles;

	if (viewangles.dist(m_vecDestinationAngles) < 0.01f) {
		return;
	}

	fvec3 angle_delta = m_vecDestinationAngles - viewangles;
	fvec3 destination_angles = m_vecDestinationAngles;
	
	for(int i = 0; i < 3; i++){
		if (abs(angle_delta[i]) > 180) {
			destination_angles[i] += 360;
		}
	}

	angle_delta = destination_angles - viewangles;
	fvec3 new_deltas = angle_delta / abs(m_fTotalDistance - m_fDistanceMoved);
	m_vecTargetAngles = viewangles + new_deltas;
}
void CLineup::UpdateOrigin()
{
	fvec3& origin = (fvec3&)cgs->predictedPlayerState.origin;
	fvec3 move_delta = (fvec3(origin) - m_vecOldOrigin).abs();

	m_fDistanceMoved += move_delta.mag();
	m_vecOldOrigin = origin;
}
void CLineup::MoveCloser(usercmd_s* cmd, usercmd_s* oldcmd)
{
	auto ps = &cgs->predictedPlayerState;
	const fvec3& viewangles = (fvec3&)ps->viewangles;
	const fvec3& origin = (fvec3&)ps->origin;


	clientInput_t cl = CL_GetInputsFromAngle(AngleNormalize180(m_fYawToTarget - viewangles[YAW]));
	const float dist = m_vecDestination.dist(origin);

	if (dist < CG_GetSpeed(&cgs->predictedPlayerState) / 1.5f) {
		cmd->buttons |= m_iCmdButtons;
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		const auto cmds = PredictStopPosition(ps, cmd, oldcmd);
		const fvec3 predicted_pos = cmds.back().origin;

		const float xy_dist = fvec2(m_vecDestination).dist(fvec2(predicted_pos));
		const float z_dist = std::abs(predicted_pos.z - m_vecDestination.z);

		if (xy_dist <= m_fLineupAccuracy && z_dist < 100) {
			m_uAccuracyTestFrames = cmds.size();
			CStaticMovementRecorder::PushPlaybackCopy(cmds, CG_GetSpeed(&cgs->predictedPlayerState), CG_HasJumpSlowdown());
			return;
		}

		//use the next position to see how we can decelerate
		const float moveDirection = (predicted_pos - m_vecDestination).toangles().y;
		cl = CL_GetInputsFromAngle(AngleNormalize180(moveDirection - viewangles[YAW]));

		//move to the opposite direction for deceleration
		cmd->forwardmove = -cl.forwardmove;
		cmd->rightmove = -cl.rightmove;
		return;
	}

	cmd->forwardmove = cl.forwardmove;
	cmd->rightmove = cl.rightmove;


	
}

void CLineup::CreatePlayback(usercmd_s* cmd, usercmd_s* oldcmd) const
{
	if (m_uAccuracyTestFrames)
		return;

	auto ps = &cgs->predictedPlayerState;

	VectorCopy(m_vecTargetAngles, ps->viewangles);
	std::vector<playback_cmd> cmds = { playback_cmd::FromPlayerState(ps, cmd, oldcmd) };
	cmds[0].serverTime = cmds[0].oldTime + (1000 / LINEUP_FPS);
	CStaticMovementRecorder::PushPlaybackCopy(cmds, CG_GetSpeed(&cgs->predictedPlayerState), CG_HasJumpSlowdown());
}
bool CLineup::CanPathfind() const noexcept
{
	auto ps = &cgs->predictedPlayerState;
	if (m_vecDestination.dist_sq(ps->origin) < 10 * 10)
		return true;

	fvec3 o = ps->origin;
	o.z += ps->viewHeightTarget;
	trace_t trace;
	CG_TracePoint(vec3_t{ 1,1,1 }, &trace, o, vec3_t{ -1,-1,-1 }, &m_vecDestination.x, cgs->clientNum, MASK_PLAYERSOLID, 0, 0);
	return trace.fraction >= 0.98f;
}
std::vector<playback_cmd> CLineup::PredictStopPosition(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd) const
{
	std::vector<playback_cmd> cmds;

	pmove_t pm = PM_Create(ps, cmd, oldcmd);

	CSimulationController c;
	c.buttons = cmd->buttons;
	c.forwardmove = 0;
	c.rightmove = 0;
	c.FPS = LINEUP_FPS;
	c.weapon = cmd->weapon;
	c.viewangles = CSimulationController::CAngles{ .viewangles = {0,0,0}, .angle_enum = EViewAngle::FixedTurn, .smoothing = 0.f };

	CPmoveSimulation simulation(&pm, c);

	while (pm.ps->velocity[0] != 0.f || pm.ps->velocity[1] != 0.f || pm.ps->velocity[2] != 0.f) {
		simulation.Simulate();
		c.forwardmove = 0;
		c.rightmove = 0;

		cmds.emplace_back(playback_cmd::FromPlayerState(pm.ps, &pm.cmd, &pm.oldcmd));
	}

	if(cmds.empty())
		cmds.emplace_back(playback_cmd::FromPlayerState(pm.ps, &pm.cmd, &pm.oldcmd));


	return cmds;
}

CLineupPlayback::CLineupPlayback(const CPlayback& playback, const fvec3& destination, const fvec3& destination_angles)
	: CLineup(destination, destination_angles, NVar_FindMalleableVar<float>("Lineup distance")->Get()),
	m_pPlayback(std::make_unique<CPlayback>(playback))
{

}
CPlayback& CLineupPlayback::GetPlayback()
{
	return *m_pPlayback;
}
