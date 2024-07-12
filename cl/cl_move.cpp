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

void CL_FinishMove(usercmd_s* cmd)
{
#if(DEBUG_SUPPORT)
	hooktable::find<void, usercmd_s*>(HOOK_PREFIX(__func__))->call(cmd);
#endif
	auto ps = &cgs->predictedPlayerState;
	auto oldcmd = CL_GetUserCmd(clients->cmdNumber - 1);
	if (ps->pm_type == PM_NORMAL) {
		CStaticMovementRecorder::Instance->Update(ps, cmd, oldcmd);
	}

	return;
}