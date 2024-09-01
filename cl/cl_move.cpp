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
#include <sv/sv_client.hpp>
#include <bg/bg_pmove_simulation.hpp>
#include <bg/bg_pmove.hpp>

void CL_CreateNewCommands([[maybe_unused]]int localClientNum)
{
	auto ps = &cgs->predictedPlayerState;
	auto cmd = &clients->cmds[clients->cmdNumber & 0x7F];
	auto oldcmd = &clients->cmds[(clients->cmdNumber - 1) & 0x7F];

	if (ps->pm_type == PM_NORMAL) {
		CStaticMovementRecorder::Instance->Update(ps, cmd, oldcmd);
	}

}
