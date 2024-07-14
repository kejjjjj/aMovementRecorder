#include <ranges>
#include <algorithm>
#include "mr_tests.hpp"
#include "mr_playback.hpp"
#include "bg/bg_pmove_simulation.hpp"
#include "bg/bg_pmove.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "cl/cl_utils.hpp"
#include "utils/typedefs.hpp"
#include <r/r_drawtools.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include <mutex>
#include "mr_main.hpp"

CPlaybackSimulation::CPlaybackSimulation(const CPlayback& pb) : m_oRefPlayback(pb){}
CPlaybackSimulation::~CPlaybackSimulation() = default;
using FrameDecl = CPlaybackSimulationAnalysis::FrameData;

bool CPlaybackSimulation::Simulate([[maybe_unused]]const fvec3& origin, [[maybe_unused]] const fvec3& angles)
{

	std::lock_guard<std::mutex> lock(SimulationMutex);

	auto& oldcmd = m_oRefPlayback.cmds.front();

	usercmd_s oldUsercmd =oldcmd.ToUserCmd();

	auto& pm = analysis.pm;
	auto& ps = analysis.ps;

	ps = cgs->predictedPlayerState;
	ps.commandTime = oldUsercmd.serverTime;
	ps.jumpTime = 0;
	ps.sprintState = {};
	ps.pm_time = 0;
	ps.pm_type = PM_NORMAL;


	pm = PM_Create(&ps, CL_GetUserCmd(clients->cmdNumber-1), CL_GetUserCmd(clients->cmdNumber - 2));
	CPmoveSimulation sim(&pm);

	oldUsercmd.serverTime -= 3;

	for (const auto& cmd : m_oRefPlayback.cmds) {
		
		analysis.frames.emplace_back(FrameDecl{ 
			.origin = pm.ps->origin, 
			.angles = pm.ps->viewangles 
		});

		auto usercmd = cmd.ToUserCmd();

		const auto i_yaw_deltas = ANGLE2SHORT(ps.delta_angles[YAW]);
		const auto iexpectation = short(cmd.cmd_angles.y + ANGLE2SHORT(cmd.delta_angles.y));
		const auto netAng = iexpectation - i_yaw_deltas;

		usercmd.angles[YAW] = netAng;


		sim.Simulate(&usercmd, &oldUsercmd);
		oldUsercmd = usercmd;

		

	}
	return true;


}

void CPlaybackSimulation::RenderPath() const 
{
	std::lock_guard<std::mutex> lock(SimulationMutex);

	if (analysis.frames.empty())
		return;

	const FrameDecl* oldFrame = &analysis.frames.front();
	fvec2 oldOrigin = WorldToScreenReal(oldFrame->origin).value_or(fvec2());

	for (const auto & frame : analysis.frames | std::views::drop(1)) {

		const FrameDecl *currentFrame = &frame;

		if (const auto optionalPos = WorldToScreenReal(currentFrame->origin)) {
			const auto& origin = optionalPos.value();

			ImGui::GetBackgroundDrawList()->AddLine({ oldOrigin.x, oldOrigin.y }, { origin.x, origin.y }, IM_COL32(0, 255, 0, 150));

			oldOrigin = origin;
		}
		
		oldFrame = currentFrame;
	}


}
