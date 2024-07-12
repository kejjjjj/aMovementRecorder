#include "mr_tests.hpp"
#include "mr_playback.hpp"
#include "bg/bg_pmove_simulation.hpp"
#include "bg/bg_pmove.hpp"
#include "cg/cg_offsets.hpp"
#include "cl/cl_utils.hpp"
#include "utils/typedefs.hpp"


CPlaybackSimulation::CPlaybackSimulation(const CPlayback& pb) : m_oRefPlayback(pb){}
CPlaybackSimulation::~CPlaybackSimulation() = default;

bool CPlaybackSimulation::Simulate(const fvec3& origin, const fvec3& angles)
{
	auto& pm = analysis.pm;
	auto& ps = analysis.ps;

	ps = cgs->predictedPlayerState;

	(fvec3&)ps.origin = origin;
	(fvec3&)ps.viewangles = angles;

	pm = PM_Create(&ps, CL_GetUserCmd(clients->cmdNumber-1), CL_GetUserCmd(clients->cmdNumber - 2));
	CPmoveSimulation sim(&pm);

	//temp
	//(fvec3&)pm.ps->delta_angles = cgs->predictedPlayerState.delta_angles;

	usercmd_s oldUsercmd = m_oRefPlayback.cmds.front().ToUserCmd();
	oldUsercmd.serverTime -= 3;
	(fvec3&)pm.ps->delta_angles = m_oRefPlayback.cmds.front().delta_angles;

	using FrameDecl = CPlaybackSimulationAnalysis::FrameData;

	for (const auto& cmd : m_oRefPlayback.cmds) {
		
		analysis.frames.emplace_back(FrameDecl{ 
			.origin = pm.ps->origin, 
			.angles = pm.ps->viewangles 
		});

		//if (cmd.origin.dist_sq(pm.ps->origin) != 0.000000f)
		//	return false;

		const auto usercmd = cmd.ToUserCmd();

		sim.Simulate(&usercmd, &oldUsercmd);
		oldUsercmd = usercmd;

		

	}
	return true;


}
