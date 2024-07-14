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
	m_vecTargetAngles = CG_GetClientAngles();

	m_fTotalDistance = m_vecOldOrigin.xy().dist(target);

	//50% chance to lineup while crouched
	m_iCmdButtons = cmdEnums::crouch * bool(std::round(random(1.f)));
}


bool CLineup::Update(usercmd_s* cmd, [[maybe_unused]]usercmd_s* oldcmd) noexcept
{
	if (!CanPathfind() || WASD_PRESSED()) {
		m_eState = State::gave_up;
		return false;
	}

	if (m_uAccuracyTestFrames > 0)
		return --m_uAccuracyTestFrames, true;

	if (m_vecDestination.xy().dist(cgs->predictedPlayerState.origin) <= m_fLineupAccuracy
		&& fvec2(cgs->predictedPlayerState.velocity).MagSq() == 0.f) {
		m_eState = State::finished;
	}

	if (m_eState == finished && m_eAngleState == finished) {
		return false;
	}

	UpdateViewangles(cmd);
	UpdateOrigin();
	
	m_fYawToTarget = AngleNormalize180((m_vecDestination - fvec3(cgs->predictedPlayerState.origin)).toangles().y);

	MoveCloser(cmd, oldcmd);
	CreatePlayback(cmd, oldcmd);

	


	return true;

}
float GetShorterPath(float source, float destination)
{
	const float src = AngleNormalize180(source);
	const float dst = AngleNormalize180(destination);

	return AngleNormalize180(dst - src);
}

void CLineup::UpdateViewangles([[maybe_unused]]usercmd_s* cmd)
{
	auto ps = &cgs->predictedPlayerState;

	const fvec3 viewangles = CG_GetClientAngles();

	const bool ignorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get();


	const bool goodViewangles = viewangles.every([this, ignorePitch, i = 0](float v) mutable -> bool {
		const auto index = i++;
		return (ignorePitch && index != YAW) ? true : std::fabsf(v - m_vecDestinationAngles[index]) < 0.01f;
	});


	if (goodViewangles) {
		m_eAngleState = finished;
		return;
	}

	fvec3 destination_deltas = m_vecDestinationAngles.for_each([this, &viewangles, i = 0](float v) mutable {
		return GetShorterPath(viewangles[i++], v); });
	
	//don't turn too quickly
	destination_deltas = fvec3().smooth(destination_deltas, m_fDistanceMoved / m_fTotalDistance + 0.001f);

	//clamp and randomize to avoid suspicious deltas
	const float scale = random(0.5f, 1.f);
	destination_deltas = destination_deltas.clamp(-2.f * scale, 2.f * scale);

	m_vecTargetAngles = viewangles + destination_deltas;
	m_ivecTargetCmdAngles = m_vecTargetAngles.angle_delta(ps->delta_angles).to_short();

	if (m_eState == finished) {

		if (ignorePitch)
			cmd->angles[YAW] = m_ivecTargetCmdAngles[YAW];
		else
			(ivec3&)cmd->angles = m_ivecTargetCmdAngles;
	}

}
void CLineup::UpdateOrigin()
{
	fvec3& origin = (fvec3&)cgs->predictedPlayerState.origin;
	m_fDistanceMoved = m_fTotalDistance - origin.xy().dist(m_vecDestination);

	m_vecOldOrigin = origin;
}
void CLineup::MoveCloser(usercmd_s* cmd, usercmd_s* oldcmd)
{
	if (m_eState == finished)
		return;

	auto ps = &cgs->predictedPlayerState;
	const fvec3 viewangles = CG_GetClientAngles();
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
			CStaticMovementRecorder::PushPlayback(cmds, 
				{
					.g_speed = CG_GetSpeed(&cgs->predictedPlayerState), 
					.jump_slowdownEnable = CG_HasJumpSlowdown(),
					.ignorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
				});

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

void CLineup::CreatePlayback(usercmd_s* cmd, [[maybe_unused]]usercmd_s* oldcmd) const
{
	if (m_uAccuracyTestFrames)
		return;

	auto ps = &cgs->predictedPlayerState;

	playback_cmd pcmd;
	pcmd.buttons = cmd->buttons;
	pcmd.forwardmove = cmd->forwardmove;
	pcmd.offhand = cmd->offHandIndex;
	pcmd.origin = ps->origin;
	pcmd.rightmove = cmd->rightmove;
	pcmd.velocity = ps->velocity;
	pcmd.weapon = cmd->weapon;

	pcmd.oldTime = cmd->serverTime;
	pcmd.serverTime = pcmd.oldTime + (1000 / LINEUP_FPS);

	pcmd.cmd_angles = m_vecTargetAngles.angle_delta(ps->delta_angles).to_short();
	pcmd.delta_angles = ps->delta_angles;


	CStaticMovementRecorder::PushPlayback(
		{ 
			pcmd 
		},
		{
			.g_speed = CG_GetSpeed(&cgs->predictedPlayerState),
			.jump_slowdownEnable = CG_HasJumpSlowdown(),
			.ignorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
		});

}
bool CLineup::CanPathfind() const noexcept
{
	auto ps = &cgs->predictedPlayerState;
	if (m_vecDestination.dist_sq(ps->origin) < 300 * 300)
		return true;
	
	return false;
	//fvec3 o = ps->origin;
	//o.z += ps->viewHeightTarget;
	//trace_t trace;
	//CG_TracePoint(vec3_t{ 1,1,1 }, &trace, o, vec3_t{ -1,-1,-1 }, &m_vecDestination.x, cgs->clientNum, MASK_PLAYERSOLID, 0, 0);
	//return trace.fraction >= 0.98f;
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
		std::int32_t delta = pm.cmd.serverTime - pm.oldcmd.serverTime;
		simulation.Simulate();
		c.forwardmove = 0;
		c.rightmove = 0;

		cmds.emplace_back(playback_cmd::FromPlayerState(pm.ps, &pm.cmd, &pm.oldcmd));

		memcpy(&pm.oldcmd, &pm.cmd, sizeof(pm.oldcmd));
		pm.cmd.serverTime += delta;
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
