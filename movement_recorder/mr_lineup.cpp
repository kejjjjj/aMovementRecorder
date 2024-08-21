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

CLineup::CLineup(const playerState_s* ps, const fvec3& target, const fvec3& destination_angles, float lineup_distance= NVar_FindMalleableVar<float>("Lineup distance")->Get()):
	m_vecDestination(target), 
	m_vecDestinationAngles(destination_angles.normalize180()), 
	m_fLineupAccuracy(std::clamp(lineup_distance, 0.000000f, lineup_distance))
{

	m_vecOldOrigin = ps->origin;
	m_vecTargetAngles = CG_GetClientAngles();

	m_fTotalDistance = m_vecOldOrigin.xy().dist(target);

	//50% chance to lineup while crouched
	m_iCmdButtons = cmdEnums::crouch * bool(std::round(random(1.f)));
}


bool CLineup::Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd) noexcept
{
	if (!CanPathfind(ps) || WASD_PRESSED()) {
		m_eState = State::gave_up;
		return false;
	}

	if (m_uAccuracyTestFrames > 0)
		return --m_uAccuracyTestFrames, true;

	if (m_vecDestination.xy().dist(ps->origin) <= m_fLineupAccuracy
		&& fvec2(ps->velocity).MagSq() == 0.f) {
		m_eState = State::finished;
	}

	if (m_eState == finished && m_eAngleState == finished) {
		return false;
	}

	UpdateViewangles(ps, cmd);
	UpdateOrigin(ps);
	
	m_fYawToTarget = AngleNormalize180((m_vecDestination - fvec3(ps->origin)).toangles().y);

	MoveCloser(ps, cmd, oldcmd);
	CreatePlayback(ps, cmd);

	return true;

}
float GetShorterPath(float source, float destination)
{
	const float src = AngleNormalize180(source);
	const float dst = AngleNormalize180(destination);

	return AngleNormalize180(dst - src);
}

void CLineup::UpdateViewangles(const playerState_s* ps, [[maybe_unused]]usercmd_s* cmd)
{

	const fvec3& viewangles = ps->viewangles;

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
void CLineup::UpdateOrigin(const playerState_s* ps)
{
	fvec3& origin = (fvec3&)ps->origin;
	m_fDistanceMoved = m_fTotalDistance - origin.xy().dist(m_vecDestination);

	m_vecOldOrigin = origin;
}
void CLineup::MoveCloser(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd)
{
	if (m_eState == finished)
		return;

	const fvec3& viewangles = ps->viewangles;

	clientInput_t cl = CL_GetInputsFromAngle(AngleNormalize180(m_fYawToTarget - viewangles[YAW]));
	const float dist = m_vecDestination.dist(ps->origin);

	if (dist < CG_GetSpeed(ps) / 1.5f) {
		cmd->buttons |= m_iCmdButtons;
		cmd->forwardmove = 0;
		cmd->rightmove = 0;

		playerState_s ps_local = *ps;
		const auto cmds = CPmoveSimulation::PredictStopPosition(&ps_local, cmd, oldcmd, LINEUP_FPS);
		
		const fvec3 predicted_pos = cmds.back().origin;

		const float xy_dist = fvec2(m_vecDestination).dist(predicted_pos.xy());
		const float z_dist = std::abs(predicted_pos.z - m_vecDestination.z);

		if (xy_dist <= m_fLineupAccuracy && z_dist < 100) {
			m_uAccuracyTestFrames = cmds.size();
			CStaticMovementRecorder::PushPlayback(cmds, 
				{
					.m_iGSpeed = CG_GetSpeed(ps),
					.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
					.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
					.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),
					.m_bNoLag = true, //lagging will ruin the accuracy
				});

			return;
		}

		//use the next position to see how we can decelerate
		const float moveDirection = (predicted_pos - m_vecDestination).toangles().y;
		cl = CL_GetInputsFromAngle(AngleNormalize180(moveDirection - viewangles[YAW]));

		//cl = GetNextDirection(&ps_local, cmd, oldcmd),

		//move to the opposite direction for deceleration
		cmd->forwardmove = -cl.forwardmove;
		cmd->rightmove = -cl.rightmove;
		return;
	}

	cmd->forwardmove = cl.forwardmove;
	cmd->rightmove = cl.rightmove;


	
}
clientInput_t CLineup::GetNextDirection(const playerState_s* ps, const usercmd_s* cmd, const usercmd_s* oldcmd) const
{
	clientInput_t bestInputs{};
	auto bestDistance = std::numeric_limits<float>::max();


	const std::int8_t possibleValues[] = { -127, 0, 127 };

	for (auto fm : possibleValues) {
		for (auto rm : possibleValues) {

			playerState_s ps_local = *ps;
			auto ccmd = *cmd;
			ccmd.forwardmove = fm;
			ccmd.rightmove = rm;


			auto result = CPmoveSimulation::PredictStopPositionAdvanced(&ps_local, &ccmd, oldcmd, { .iFPS = LINEUP_FPS, .uNumRepetitions = 1});

			if (!result)
				continue;

			const auto distance = fvec2(result->back().origin).dist(m_vecDestination);

			if (distance < bestDistance) {
				bestInputs.forwardmove = fm;
				bestInputs.rightmove = rm;
				bestDistance = distance;
			}

		}

	}
	return bestInputs;
}
void CLineup::CreatePlayback(const playerState_s* ps, usercmd_s* cmd) const
{
	if (m_uAccuracyTestFrames)
		return;

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


	CStaticMovementRecorder::PushPlayback({ pcmd },
		{
			.m_iGSpeed = CG_GetSpeed(ps),
			.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
			.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
			.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),
			.m_bNoLag = true, //lagging will ruin the accuracy
		});

}
bool CLineup::CanPathfind(const playerState_s* ps) const noexcept
{
	if (m_vecDestination.dist_sq(ps->origin) < 300 * 300)
		return true;
	
	return false;
	//fvec3 o = ps->origin;
	//o.z += ps->viewHeightTarget;
	//trace_t trace;
	//CG_TracePoint(vec3_t{ 1,1,1 }, &trace, o, vec3_t{ -1,-1,-1 }, &m_vecDestination.x, cgs->clientNum, MASK_PLAYERSOLID, 0, 0);
	//return trace.fraction >= 0.98f;
}

CLineupPlayback::CLineupPlayback(const playerState_s* ps, const CPlayback& playback, const fvec3& destination, const fvec3& destination_angles)
	: CLineup(ps, destination, destination_angles, NVar_FindMalleableVar<float>("Lineup distance")->Get()),
	m_pPlayback(std::make_unique<CPlayback>(playback))
{

}
CPlayback& CLineupPlayback::GetPlayback()
{
	return *m_pPlayback;
}
