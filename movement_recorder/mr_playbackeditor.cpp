#include "bg/bg_pmove_simulation.hpp"

#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "cg/cg_client.hpp"

#include "mr_playbackeditor.hpp"
#include "mr_playback.hpp"

#include "r/backend/rb_endscene.hpp"

#include "shared/sv_shared.hpp"

#include "utils/typedefs.hpp"

#include <algorithm>
#include <ranges>
#include <cassert>

CPlaybackEditor::CPlaybackEditor(CPlayerStatePlayback& pb) 
	: m_oRefPlayback(pb), m_uPlayerStateToCmdRatio(pb.m_objExtraHeader.m_uPlayerStateToCmdRatio){



}
CPlaybackEditor::~CPlaybackEditor() = default;

void CPlaybackEditor::Advance(std::int32_t amount)
{

	auto newOffset = int(m_iCurrentPlayerStateOffsetFromBeginning) + amount;
	
	if (newOffset < 0)
		newOffset = 0;

	m_iCurrentPlayerStateOffsetFromBeginning = std::clamp(size_t(newOffset), 0u, m_oRefPlayback.playerStates.size()-1u);

}
const playerState_s* CPlaybackEditor::GetCurrentState() const
{
	return &m_oRefPlayback.playerStates[m_iCurrentPlayerStateOffsetFromBeginning];
}
size_t CPlaybackEditor::GetCurrentCmdOffset() const noexcept
{
	return m_iCurrentPlayerStateOffsetFromBeginning * m_uPlayerStateToCmdRatio;
}
size_t CPlaybackEditor::GetCurrentPlayerStateOffset() const noexcept
{
	return m_iCurrentPlayerStateOffsetFromBeginning;
}
void CPlaybackEditor::Merge(const CPlayerStatePlayback& playback)
{

	std::unique_lock<std::mutex> lock(mtx);

	auto& ref = m_oRefPlayback;

	ref.cmds.erase(ref.cmds.begin() + GetCurrentCmdOffset(), ref.cmds.end());
	ref.playerStates.erase(ref.playerStates.begin() + GetCurrentPlayerStateOffset(), ref.playerStates.end());

	ref.cmds.insert(ref.cmds.end(), playback.cmds.begin(), playback.cmds.end());
	ref.playerStates.insert(ref.playerStates.end(), playback.playerStates.begin(), playback.playerStates.end());

	Advance(0);
}
/***********************************************************************
 > 
***********************************************************************/



CPlaybackEditorRenderer::CPlaybackEditorRenderer(CPlaybackEditor& editor) : m_oRefPlaybackEditor(editor)
{
	//UpdateVertices();
}
CPlaybackEditorRenderer::~CPlaybackEditorRenderer() = default;

std::vector<playerState_s> CPlaybackEditorRenderer::GetPlayerStates() const noexcept
{

	//only render a max of 100 lines to avoid crashing
	const auto& playerStates = m_oRefPlaybackEditor.m_oRefPlayback.playerStates;
	
	if (playerStates.empty())
		return{};



	auto firstIndex = std::clamp(m_oRefPlaybackEditor.m_iCurrentPlayerStateOffsetFromBeginning - 100u, 0u, playerStates.size()-1u);

	if ((int(m_oRefPlaybackEditor.m_iCurrentPlayerStateOffsetFromBeginning) - 100) < 0)
		firstIndex = 0;

	const auto lastIndex = std::clamp(m_oRefPlaybackEditor.m_iCurrentPlayerStateOffsetFromBeginning + 100u, 0u, playerStates.size()-1u);

	return std::vector<playerState_s>(playerStates.begin() + firstIndex, playerStates.begin() + lastIndex);
}

void CPlaybackEditorRenderer::RB_Render([[maybe_unused]]GfxViewParms* vParms) const
{
	std::unique_lock<std::mutex> lock(m_oRefPlaybackEditor.mtx);


	const auto currentState = m_oRefPlaybackEditor.GetCurrentState();
	const auto playerStates = GetPlayerStates();

	assert(playerStates.size());

	fvec3 oldOrigin = playerStates.front().origin;

	for (const auto& v : playerStates | std::views::drop(1)) {
		CG_DebugLine(oldOrigin, (fvec3&)v.origin, vec4_t{ 0,1,0,0.7f }, true, cls->frametime * 2);
		oldOrigin = v.origin;
	}

	const fvec3 mins = fvec3{ -15.f,-15.f, 0.f } + currentState->origin;
	const fvec3 maxs = fvec3{ 15.f, 15.f, (float)CG_GetViewHeight(currentState) } + currentState->origin;
	
#if(DEBUG_SUPPORT)
	RB_DrawBoxEdges(mins, maxs, true, vec4_t{ 1,0,0,0.7f });
#else

	const auto func = CMain::Shared::GetFunctionSafe("CM_IsRenderingAsPolygons");

	if(!func || !func->As<bool>()->Call())
		RB_DrawBoxPolygons(mins, maxs, true, vec4_t{ 1,0,0,0.7f });
	else
		RB_DrawBoxEdges(mins, maxs, true, vec4_t{ 1,0,0,0.7f });

#endif
}

void CPlaybackEditorRenderer::UpdateVertices()
{

	//const auto& playerStates = m_oRefPlaybackEditor.m_oRefPlayback.playerStates;

	//m_oPathVertices.resize(playerStates.size());

	//assert(playerStates.size());

	//auto oldOrigin = playerStates.front().origin;
	//for (const auto& v : playerStates | std::views::drop(1)) {
	//	m_iVertCount = RB_AddDebugLine(m_oPathVertices.data(), true, oldOrigin, v.origin, vec4_t{ 0,1,0,0.7f }, m_iVertCount);
	//	oldOrigin = v.origin;
	//}

}

