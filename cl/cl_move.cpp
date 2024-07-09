#include "cl_move.hpp"
#include "utils/hook.hpp"

#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>

#include "movement_recorder/mr_main.hpp"
#include <dvar/dvar.hpp>
#include <com/com_channel.hpp>
#include <cl/cl_utils.hpp>
#include <cg/cg_angles.hpp>
#include <utils/typedefs.hpp>

//static void DoDebugging(usercmd_s* cmd, usercmd_s* oldcmd)
//{
//	auto PlaybackActive = CStaticMovementRecorder::Instance->GetActive();
//
//	if (!PlaybackActive)
//		return;
//
//	//auto ps = &cgs->predictedPlayerState;
//	const auto& riter = std::prev(PlaybackActive->GetActualIterator());
//
//	if (riter == PlaybackActive->cmds.end())
//		return;
//
//	const auto iter = &*riter;
//	if (!iter) return;
//	//const auto dist = iter->origin.dist_sq(ps->origin);
//
//	//if (dist > 0.000000f) {
//
//	//	Com_Printf("^1precision failure!\n");
//
//	//	Com_Printf("^1precision failure!\n");
//	//	char buff[512]{};
//
//	//	sprintf_s(buff,
//	//		"TimeDelta:     %i/%i\n"
//	//		"Viewangles[0]: %.6f/%.6f\n"
//	//		"Viewangles[1]: %.6f/%.6f\n"
//	//		"Viewangles[2]: %.6f/%.6f\n",
//	//		cmd->serverTime - oldcmd->serverTime, iter->serverTime - p_iter->serverTime,
//	//		ps->viewangles[0], iter->viewangles[0],
//	//		ps->viewangles[1], iter->viewangles[1], 
//	//		ps->viewangles[2], iter->viewangles[2]
//	//	);
//
//	//	Com_Printf(buff);
//
//	//	//PlaybackActive->StopPlayback();
//
//	//}
//}

void CL_FinishMove(usercmd_s* cmd)
{
#if(DEBUG_SUPPORT)
	hooktable::find<void, usercmd_s*>(HOOK_PREFIX(__func__))->call(cmd);
#endif
	auto ps = &cgs->predictedPlayerState;
	auto oldcmd = CL_GetUserCmd(clients->cmdNumber - 1);

	if (ps->pm_type == PM_NORMAL) {


		//ivec3 angles = fvec3(0, i, 0).angle_delta(cgs->predictedPlayerState.delta_angles).to_short();

		//fvec3 viewangles = (angles.from_short() + ps->delta_angles).normalize180();

		//(ivec3&)cmd->angles = viewangles.angle_delta(cgs->predictedPlayerState.delta_angles).to_short();


		CStaticMovementRecorder::Instance->Update(ps, cmd, oldcmd);


//#if(DEBUG_SUPPORT)
//		DoDebugging(cmd, oldcmd);
//#endif
	}

	return;
}