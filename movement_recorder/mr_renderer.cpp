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

CRMovementRecorder::~CRMovementRecorder() = default;
void CRMovementRecorder::CG_Render()
{
	CStaticMovementRecorder::Update();

	CG_RenderOrigins();
	CG_RenderPrecision();
	CG_RenderStatus();
}

void CRMovementRecorder::CG_RenderOrigins() const
{
	if (!NVar_FindMalleableVar<bool>("Show Origins")->Get())
		return;

	using playback_t = std::remove_reference<decltype(*LevelPlaybacks.begin())>::type;

	std::vector<CPlayback*> relevant_playbacks;
	std::ranges::for_each(LevelPlaybacks, [ps = &cgs->predictedPlayerState, &relevant_playbacks](const playback_t& pb) {
		if (pb.second->IsCompatibleWithState(ps))
			relevant_playbacks.push_back(pb.second.get());
		});


	std::ranges::for_each(relevant_playbacks, [ps = &cgs->predictedPlayerState](const CPlayback* pb) {
		if (auto xyo = WorldToScreen(pb->GetOrigin())) {
			auto& xy = xyo.value();

			float dist = R_ScaleByDistance(pb->GetOrigin().dist(ps->origin)) * 20.f;
			CG_DrawRotatedPic(0, 0, CG_GetScreenPlacement(), xy.x - dist / 2, xy.y - dist / 2, dist, dist, 180.f
				, vec4_t{ 1,1,1,1 }, "compassping_friendly_mp");

		}});
}
void CRMovementRecorder::CG_RenderPrecision() const
{
	const auto Active = GetActive();
	if (!Active)
		return;

	if (IsSegmenting() && Segmenter->ResultExists())
		return;

	constexpr float col[4] = { 1,1,1,1 };
	constexpr float glowCol[4] = { 1,0,0,1 };


	const auto iter = Active->GetIterator();
	if (!iter) return;
	const auto dist = iter->origin.dist(cgs->predictedPlayerState.origin);

	char buff[64];
	sprintf_s(buff, "Precision: %.6f\n", dist);

	float x = R_GetTextCentered(buff, "fonts/normalfont", 320.f, 0.5f);
	R_AddCmdDrawTextWithEffects(buff, "fonts/normalfont", x, 260.f, 0.5f, 0.5f, 0.f, col, 3, glowCol, nullptr, nullptr, 0, 0, 0, 0);


}
void CRMovementRecorder::CG_RenderStatus() const
{
	constexpr float col[4] = { 0,1,0,1 };
	constexpr float glowCol[4] = { 0,1,0,1 };


	if (IsRecording()) {
		float x = R_GetTextCentered("recording", "fonts/normalfont", 320.f, 0.5f);
		R_AddCmdDrawTextWithEffects((char*)"recording", "fonts/normalfont", x, 260, 0.5f, 0.5f, 0.f, col, 3, glowCol, nullptr, nullptr, 0, 0, 0, 0);
		return;
	}

	else if (IsSegmenting() && Segmenter->ResultExists()) {
		float x = R_GetTextCentered("segmenting", "fonts/normalfont", 320.f, 0.5f);
		R_AddCmdDrawTextWithEffects((char*)"segmenting", "fonts/normalfont", x, 260, 0.5f, 0.5f, 0.f, col, 3, glowCol, nullptr, nullptr, 0, 0, 0, 0);
	}


}
