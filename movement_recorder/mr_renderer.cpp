#include "movement_recorder/mr_main.hpp"
#include <r/r_drawtools.hpp>
#include <com/com_channel.hpp>
#include <net/nvar_table.hpp>
#include "movement_recorder/mr_segmenter.hpp"
#include "movement_recorder/mr_record.hpp"
#include "movement_recorder/mr_playback.hpp"
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>
#include <bg/bg_pmove_simulation.hpp>

void CRMovementRecorder::CG_Render() const
{
	if (NVar_FindMalleableVar<bool>("Show Origins")->Get())
		CG_RenderOrigins();

	if (NVar_FindMalleableVar<bool>("Status Text")->Get()) {
		CG_RenderPrecision();
	}

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

#define RGBA(r,g,b,a) vec4_t{r,g,b,a}

constexpr float x_pos = 320.f;
constexpr float y_pos = 400.f;
constexpr char font[] = "fonts/objectivefont";
constexpr float font_scale = 0.25f;

static void CG_RenderStatusText(const char* string, const vec4_t color, const vec4_t glow_col = 0)
{
	const float x = R_GetTextCentered(string, font, x_pos, font_scale);
	R_AddCmdDrawTextWithEffects((char*)string, font, x, y_pos, font_scale, font_scale, 0.f, color, 3, glow_col, nullptr, nullptr, 0, 0, 0, 0);

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
