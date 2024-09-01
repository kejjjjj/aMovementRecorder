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

class CPlaybackSegmenterImpl
{
public:
	CPlaybackSegmenterImpl(const CPlayback& source)
		: m_oOriginal(source)
	{
	}

	success_t Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd)
	{

		if (!m_oRecorder && (WASD_PRESSED() || m_bExternalPartyWantsToStartSegmenting)) {

			if ((oldcmd->buttons & cmdEnums::crouch) != 0) {
				cmd->buttons |= (cmdEnums::crouch_hold);
				CBuf_Addtext("gocrouch\n");
				
			}

			m_oRecorder = std::make_unique<CRecorder>();
			m_iNewSegment = m_oOriginal.GetIteratorIndex();

		}

		if (m_oRecorder)
			return m_oRecorder->Record(ps, cmd, oldcmd), true;


		m_oOriginal.DoPlayback(cmd, oldcmd);

		if (!m_oOriginal.IsPlayback()) {
			return false;
		}

		return true;
	}
	void StartSegmenting() noexcept {
		m_bExternalPartyWantsToStartSegmenting = true;
	}
	std::optional<CPlayback> GetResult() const
	{
		if (!m_iNewSegment || m_iNewSegment >= m_oOriginal.cmds.size())
			return {};

		auto&& result = m_oRecorder->StopRecording();

		std::vector<playback_cmd> newdata;
		newdata.insert(newdata.end(), m_oOriginal.cmds.begin(), m_oOriginal.cmds.begin() + m_iNewSegment);



		int CurrentTime = newdata.back().serverTime;



		//Fix servertime between the original and the new segment
		std::ranges::for_each(result, [&CurrentTime](playback_cmd& cmd) {
			const int delta = cmd.serverTime - cmd.oldTime;
			cmd.serverTime = CurrentTime + delta;
			cmd.oldTime = cmd.serverTime - delta;
			CurrentTime += delta;
			});

		newdata.insert(newdata.end(), result.begin(), result.end());

		return std::optional<CPlayback>(CPlayback(newdata, 
			{
				.m_iGSpeed = CG_GetSpeed(&cgs->predictedPlayerState),
				.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
				.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
				.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),

			}));
	}
	inline bool ResultExists() const noexcept {
		return m_oRecorder.get();
	}
	CPlayback* GetPlayback()
	{
		return &m_oOriginal;
	}
private:
	std::unique_ptr<CRecorder> m_oRecorder;
	CPlayback m_oOriginal;
	std::size_t m_iNewSegment = 0u;
	bool m_bExternalPartyWantsToStartSegmenting = false;
};


CPlaybackSegmenter::CPlaybackSegmenter(const CPlayback& source)
	: m_oImpl(std::make_unique<CPlaybackSegmenterImpl>(source))
{
}
CPlaybackSegmenter::~CPlaybackSegmenter() = default;

success_t CPlaybackSegmenter::Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd)
{
	return m_oImpl->Update(ps, cmd, oldcmd);
}
void CPlaybackSegmenter::StartSegmenting() noexcept
{
	return m_oImpl->StartSegmenting();
}
std::optional<CPlayback> CPlaybackSegmenter::GetResult() const
{
	return m_oImpl->GetResult();
}
bool CPlaybackSegmenter::ResultExists() const noexcept
{ 
	return m_oImpl->ResultExists();
}
CPlayback* CPlaybackSegmenter::GetPlayback()
{
	return m_oImpl->GetPlayback();
}