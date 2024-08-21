#include "bg/bg_pmove_simulation.hpp"

#include "cg/cg_angles.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"

#include "cl/cl_move.hpp"
#include "cl/cl_utils.hpp"

#include "com/com_channel.hpp"

#include "dvar/dvar.hpp"

#include "geo/geo_shapes.hpp"

#include "movement_recorder/mr_main.hpp"
#include "movement_recorder/mr_playback.hpp"
#include "movement_recorder/mr_record.hpp"
#include "movement_recorder/mr_segmenter.hpp"

#include "net/nvar_table.hpp"

#include "r/r_drawtools.hpp"
#include "r/backend/rb_endscene.hpp"
#include "r/r_utils.hpp"

#include "sys/sys_main.hpp"

#include "utils/hook.hpp"
#include "utils/functions.hpp"


#include <windows.h>


#define RGBA(r,g,b,a) vec4_t{r,g,b,a}

constexpr float x_pos = 320.f;
constexpr float y_pos = 400.f;
constexpr char font[] = "fonts/objectivefont";
constexpr float font_scale = 0.25f;

static void CG_RenderStatusText(const char* string, const vec4_t color, const vec4_t glow_col = 0, float y_offs = 0)
{
	const float x = R_GetTextCentered(string, font, x_pos, font_scale);
	R_AddCmdDrawTextWithEffects((char*)string, font, x, y_pos + y_offs, font_scale, font_scale, 0.f, color, 3, glow_col, nullptr, nullptr, 0, 0, 0, 0);

}
void CRMovementRecorder::CG_Render() const
{
	//if (NVar_FindMalleableVar<bool>("Show Origins")->Get())
	//	CG_RenderOrigins();

	if (NVar_FindMalleableVar<bool>("Status Text")->Get()) {
		CG_RenderPrecision();
	}

#if(DEBUG_SUPPORT)
	const std::string text = std::format(
			"x:      {:.6f}\n"
			"y:      {:.6f}\n"
			"z:      {:.6f}\n"
			"yaw: {:.6f}\n",
			clients->cgameOrigin[0],
			clients->cgameOrigin[1],
			clients->cgameOrigin[2],
			clients->cgameViewangles[YAW]);

		R_AddCmdDrawTextWithEffects(text, "fonts/normalFont", fvec2{ 310, 420 }, { 0.4f, 0.5f }, 0.f, 3, vec4_t{ 1,1,1,1 }, vec4_t{ 1,0,0,0 });
#endif

	//this is useful to have
	CG_RenderStatus();

}

void CRMovementRecorder::CG_RenderOrigins() const
{
	const auto& movementRecorder = m_oRefMovementRecorder;
	const auto ps = &cgs->predictedPlayerState;
	std::vector<CPlayback*> relevantPlaybacks;

	for (const auto& [name, playback] : movementRecorder.LevelPlaybacks) {

		if (playback->IsCompatibleWithState(ps))
			relevantPlaybacks.push_back(playback.get());

	}

	std::ranges::for_each(relevantPlaybacks, [&](const CPlayback* pb) {

		if (const auto xyo = WorldToScreen(pb->GetOrigin())) {
			const auto& xy = xyo.value();

			const float dist = R_ScaleByDistance(pb->GetOrigin().dist(ps->origin)) * 20.f;
			CG_DrawRotatedPic(0, 0, CG_GetScreenPlacement(), xy.x - dist / 2, xy.y - dist / 2, dist, dist, 180.f
				, vec4_t{ 1,1,1,1 }, "compassping_friendly_mp");

		}
	});


}


void CRMovementRecorder::CG_RenderPrecision() const
{
	const auto& movementRecorder = m_oRefMovementRecorder;

	const auto Active = movementRecorder.GetActivePlayback();
	if (!Active)
		return;

	if (movementRecorder.IsSegmenting() && movementRecorder.Segmenter->ResultExists())
		return;

	const auto iter = Active->GetIterator();

	if (!iter) 
		return;

	const auto dist = iter->origin.dist(clients->cgameOrigin);

	char buff[64];
	sprintf_s(buff, "Precision: %.6f\n", dist);
	CG_RenderStatusText(buff, RGBA(1,1,1,1), RGBA(1,0,0,1));
}

void CRMovementRecorder::CG_RenderStatus() const
{
	const auto& movementRecorder = m_oRefMovementRecorder;

	if (movementRecorder.IsRecording()) {
		if (movementRecorder.Recorder->IsWaiting())
			return CG_RenderStatusText("waiting", RGBA(1,0,0,1));
		
		CG_RenderStatusText("recording", RGBA(0, 1, 0, 1));

	}

	else if (movementRecorder.IsSegmenting() && movementRecorder.Segmenter->ResultExists())
		CG_RenderStatusText("segmenting", RGBA(1, 1, 0, 1));
	

}



void RB_DrawDebug([[maybe_unused]]GfxViewParms* viewParms)
{

	if (R_NoRender())
#if(DEBUG_SUPPORT)
		return hooktable::find<void, GfxViewParms*>(HOOK_PREFIX(__func__))->call(viewParms);
#else
		return;
#endif

#if(DEBUG_SUPPORT)
	hooktable::find<void, GfxViewParms*>(HOOK_PREFIX(__func__))->call(viewParms);
#endif


#if(MOVEMENT_RECORDER)
	auto renderer = CRBMovementRecorder(*CStaticMovementRecorder::Instance);
	renderer.RB_Render(viewParms);
#endif



}
CRBMovementRecorder::CRBMovementRecorder(CMovementRecorder& recorder)
	: m_oRefMovementRecorder(recorder) {}
CRBMovementRecorder::~CRBMovementRecorder() = default;

void CRBMovementRecorder::RB_Render(GfxViewParms* viewParms) const
{
	if (NVar_FindMalleableVar<bool>("Show Origins")->Get())
		RB_RenderOrigins();

	if (m_oRefMovementRecorder.m_pDebugPlayback)
		m_oRefMovementRecorder.m_pDebugPlayback->RB_Render(viewParms);

}

void CRBMovementRecorder::RB_RenderOrigins() const
{
	const auto& movementRecorder = m_oRefMovementRecorder;
	const auto ps = &cgs->predictedPlayerState;
	std::vector<CPlayback*> relevantPlaybacks;

	for (const auto& [name, playback] : movementRecorder.LevelPlaybacks) {
		if (playback && playback->IsCompatibleWithState(ps))
			relevantPlaybacks.push_back(playback.get());
	}

	for (const auto& playback : relevantPlaybacks) {

		const fvec3& origin = playback->cmds.front().origin;
		const auto points = Geom_CreatePyramid(origin, { 7,7,14 }, fmodf((Sys_MilliSeconds() / 60.f), 360.f));
		const bool good_deltas = 
			(ps->delta_angles[YAW] < 0 && playback->cmds.front().delta_angles[YAW] < 0) ||
			(ps->delta_angles[YAW] >= 0 && playback->cmds.front().delta_angles[YAW] >= 0);

		const auto color = good_deltas ? 
			vec4_t{0.f, 1.f, 0.f, 0.3f} : //green
			vec4_t{1.f, 0.f, 0.f, 0.3f};  //red

		RB_DrawPolyInteriors(points, color, true, true);
	}

}

