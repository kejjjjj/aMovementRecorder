#include "bg/bg_pmove_simulation.hpp"
#include "cg/cg_angles.hpp"
#include "cg/cg_client.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "cmd/cmd.hpp"
#include "dvar/dvar.hpp"
#include "global_macros.hpp"
#include "mr_playback.hpp"
#include "mr_record.hpp"
#include "mr_segmenter.hpp"
#include "net/nvar_table.hpp"

#include <algorithm>
#include <ranges>
#include <Windows.h>
#include <com/com_channel.hpp>


CPlaybackSegmenter::CPlaybackSegmenter(const CPlayback& source)
{
	if (source.AmIDerived())
		m_pOriginal = std::make_unique<CPlayerStatePlayback>(dynamic_cast<const CPlayerStatePlayback&>(source));
	else
		m_pOriginal = std::make_unique<CPlayback>(source);
}
CPlaybackSegmenter::~CPlaybackSegmenter() = default;



success_t CPlaybackSegmenter::Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd)
{

	if ((!m_oRecorder && (WASD_PRESSED() || m_bExternalPartyWantsToStartSegmenting))) {

		if ((oldcmd->buttons & cmdEnums::crouch) != 0) {
			cmd->buttons |= (cmdEnums::crouch_hold);
			CBuf_Addtext("gocrouch\n");
				
		}

		if (m_pOriginal->AmIDerived()) {
			const auto& psPlayback = dynamic_cast<const CPlayerStatePlayback&>(*m_pOriginal);
			m_oRecorder = std::make_unique<CPlayerStateRecorder>(false, 0, 0, psPlayback.m_objExtraHeader.m_uPlayerStateToCmdRatio);
		}
		else {
			m_oRecorder = std::make_unique<CRecorder>();
		}

		//make sure that the segment begins at the correct time
		const auto canSegment = !m_oRecorder->m_iDivisibleBy || m_pOriginal->GetIteratorIndex() % m_oRecorder->m_iDivisibleBy == 0;

		if (canSegment)
			m_iNewSegment = m_pOriginal->GetIteratorIndex();
		else
			m_oRecorder.reset();

	}

	if (m_oRecorder) {
		
		if (m_oRecorder->WaitAndStopRecording())
			return false;
		
		return m_oRecorder->Record(ps, cmd, oldcmd), true;
	}

	m_pOriginal->DoPlayback(cmd, oldcmd);

	if (!m_pOriginal->IsPlayback()) {
		return false;
	}

	return true;
}
void CPlaybackSegmenter::StartSegmenting() noexcept {
	m_bExternalPartyWantsToStartSegmenting = true;
}
void CPlaybackSegmenter::StopSegmenting() noexcept
{
	if (m_oRecorder)
		m_oRecorder->StopRecording();
}
std::unique_ptr<CPlayback> CPlaybackSegmenter::GetResult() const
{
	if (!m_iNewSegment || m_iNewSegment >= m_pOriginal->cmds.size())
		return nullptr;

	auto&& result = m_oRecorder->GiveResult();

	std::vector<playback_cmd> newdata;
	newdata.insert(newdata.end(), m_pOriginal->cmds.begin(), m_pOriginal->cmds.begin() + m_iNewSegment);



	auto CurrentTime = newdata.back().serverTime;


	//Fix servertime between the original and the new segment
	std::ranges::for_each(result, [&CurrentTime](playback_cmd& cmd) {
		const auto delta = cmd.serverTime - cmd.oldTime;
		cmd.serverTime = CurrentTime + delta;
		cmd.oldTime = cmd.serverTime - delta;
		CurrentTime += delta;
		});

	newdata.insert(newdata.end(), result.begin(), result.end());

	const CPlaybackSettings settings{
			.m_iGSpeed = CG_GetSpeed(&cgs->predictedPlayerState),
			.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
			.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
			.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),
	};

	if (m_pOriginal->AmIDerived()) {
		const auto& psPlayback = dynamic_cast<const CPlayerStatePlayback&>(*m_pOriginal);
		const auto& psRecorder = dynamic_cast<const CPlayerStateRecorder&>(*m_oRecorder);

		const auto ratio = static_cast<std::size_t>(result.size() / psRecorder.playerState.size());
		assert(ratio == psPlayback.m_objExtraHeader.m_uPlayerStateToCmdRatio);
		assert(m_iNewSegment % ratio == 0);

		//TODOOOOO: start recording ONLY when m_iNewSegment % ratio == 0

		std::vector<playerState_s> playerStates(psPlayback.playerStates.begin(), psPlayback.playerStates.begin() + m_iNewSegment / ratio);

		//add the new playerstates
		playerStates.insert(playerStates.end(), psRecorder.playerState.begin(), psRecorder.playerState.end());
		return std::make_unique<CPlayerStatePlayback>(newdata, playerStates, settings);
	}

	return std::make_unique<CPlayback>(CPlayback(newdata, settings, false));
}
bool CPlaybackSegmenter::ResultExists() const noexcept {
	return m_oRecorder.get();
}
CPlayback* CPlaybackSegmenter::GetPlayback()
{
	return m_pOriginal.get();
}