#include "bg/bg_pmove_simulation.hpp"

#include "cg/cg_angles.hpp"
#include "cg/cg_client.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"

#include "com/com_channel.hpp"

#include "dvar/dvar.hpp"

#include "mr_playback.hpp"
#include "mr_main.hpp"

#include "net/nvar_table.hpp"

#include <ranges>

CPlayback::CPlayback(std::vector<playback_cmd>&& _data, const CPlaybackSettings& init, bool eraseIdle)
	: cmds(std::forward<std::vector<playback_cmd>&&>(_data)), m_oSettings(init) {
	
	m_iCmd = 0u;

	

	m_objHeader = { 
		.m_iSpeed = init.m_iGSpeed, 
		.m_bJumpSlowdownEnable = init.m_eJumpSlowdownEnable
	};

	CStaticMovementRecorder::Instance->ClearDebugPlayback();

	if (m_oSettings.m_bRenderExpectationVsReality) {
		CStaticMovementRecorder::Instance->AddDebugPlayback(cmds);
	}

	if(eraseIdle)
		EraseDeadFrames();

}
CPlayback::CPlayback(const std::vector<playback_cmd>& _data, const CPlaybackSettings& init, bool eraseIdle)
	: cmds(_data), m_oSettings(init) {

	m_iCmd = 0u;

	m_objHeader = {
		.m_iSpeed = init.m_iGSpeed,
		.m_bJumpSlowdownEnable = init.m_eJumpSlowdownEnable
	};

	CStaticMovementRecorder::Instance->ClearDebugPlayback();

	if (m_oSettings.m_bRenderExpectationVsReality) {
		CStaticMovementRecorder::Instance->AddDebugPlayback(cmds);
	}

	if (eraseIdle)
		EraseDeadFrames();
}
CPlayback::~CPlayback() = default;

std::int32_t CPlayback::GetCurrentTimeFromIndex(const std::int32_t cmdIndex) const
{
	return m_iFirstServerTime + (cmds[cmdIndex].serverTime - cmds.front().serverTime);
}

void CPlayback::EditUserCmd(usercmd_s* cmd, const std::int32_t index) const
{
	auto ps = &cgs->predictedPlayerState;
	
	if(m_oSettings.m_bSetComMaxfps)
		SyncClientFPS(index);

	cmd->serverTime = GetCurrentTimeFromIndex(index);

	auto icmd = &cmds[index];

	const auto iAngleDeltas = fvec3(ps->delta_angles).to_short();
	const auto expectation = icmd->cmd_angles + icmd->delta_angles.to_short();
	const auto netAng = expectation - iAngleDeltas;
	
	if (m_oSettings.m_bIgnorePitch) {
		cmd->angles[YAW] = netAng.y;
		clients->viewangles[YAW] = SHORT2ANGLE(netAng.y);

	}
	else {
		(fvec3&)clients->viewangles = netAng.from_short();
		(ivec3&)cmd->angles = netAng;
	}

	if (!m_oSettings.m_bIgnoreWeapon) {
		cmd->offHandIndex = icmd->offhand;
		cmd->weapon = icmd->weapon;
	}

	cmd->forwardmove = icmd->forwardmove;
	cmd->rightmove = icmd->rightmove;
	cmd->buttons = icmd->buttons;
}
void CPlayback::SyncClientFPS(const std::int32_t index) const noexcept
{
	auto ft = cmds[index].serverTime - cmds[index].oldTime;

	if (!ft) {
		Com_Printf("^3Warning: playback with 0fps, replacing with 333fps\n");
		ft = 3;
	}

	if (m_oSettings.m_bSetComMaxfps)
		Dvar_FindMalleableVar("com_maxfps")->current.integer = 1000 / ft;
}
void CPlayback::TryFixingTime(usercmd_s* currentCmd, const usercmd_s* uoldcmd)
{

	if (auto pb = CStaticMovementRecorder::Instance->GetDebugPlayback()) {
		pb->m_oRealityPlayback.emplace_back(playback_cmd::FromPlayerState(&cgs->predictedPlayerState, currentCmd, uoldcmd));
	}

	if (m_oSettings.m_bNoLag)
		return EditUserCmd(currentCmd, m_iCmd++);

	auto numUnsent = 0u;
	auto targetTime = GetCurrentTimeFromIndex(m_iCmd);

	while (targetTime <= currentCmd->serverTime && (m_iCmd + numUnsent < cmds.size()) ) {
		targetTime = GetCurrentTimeFromIndex(m_iCmd + (++numUnsent));
	}

	if (numUnsent > 0) {
		EditUserCmd(currentCmd, m_iCmd++);

		for (size_t i = 1; i < numUnsent; i++)
			EditUserCmd(&clients->cmds[++clients->cmdNumber & 0x7F], m_iCmd++);
		
	} else {
		--clients->cmdNumber;
		currentCmd->serverTime = GetCurrentTimeFromIndex(m_iCmd);
	}

	if (auto pb = CStaticMovementRecorder::Instance->GetDebugPlayback()) {
		pb->m_oRealityPlayback.emplace_back(playback_cmd::FromPlayerState(&cgs->predictedPlayerState, currentCmd, uoldcmd));
	}


}

void CPlayback::DoPlayback(usercmd_s* cmd, const usercmd_s* oldcmd)
{
	if (!IsPlayback())
		return;




	if (!m_iCmd) {
		const auto getSign = [](float v) { return v < 0 ? 1 : 0; };
		if (Dvar_FindMalleableVar("sv_running")->current.enabled) {
			if (getSign(cgs->predictedPlayerState.delta_angles[YAW]) != getSign(GetIterator()->delta_angles[YAW])) {
				ps_loc->delta_angles[YAW] = GetIterator()->delta_angles[YAW];
				return;
			}
		}
		m_iFirstServerTime = oldcmd->serverTime + (cmds[m_iCmd].serverTime - cmds[m_iCmd].oldTime);

	}

	//if (m_iCmd == 0) {
	//	m_iFirstServerTime = oldcmd->serverTime + (cmds[m_iCmd].serverTime - cmds[m_iCmd].oldTime);

	//}

	TryFixingTime(cmd, oldcmd);
}
playback_cmd* CPlayback::GetIterator() 
{ 
	return IsPlayback() ? &cmds[m_iCmd] : nullptr;
}
std::size_t CPlayback::GetIteratorIndex() const
{
	return m_iCmd;
}

bool CPlayback::IsPlayback() const noexcept 
{ 
	return m_iCmd < cmds.size();
}
void CPlayback::StopPlayback() 
{ 
	m_iCmd = cmds.size();
}
bool CPlayback::IsCompatibleWithState(const playerState_s* ps) const noexcept 
{
	return CG_GetSpeed(ps) == m_objHeader.m_iSpeed &&
		(m_objHeader.m_bJumpSlowdownEnable == both || 
			static_cast<slowdown_t>(Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled) == m_objHeader.m_bJumpSlowdownEnable);
}
fvec3 CPlayback::GetOrigin() const noexcept { return cmds.front().origin; }
fvec3 CPlayback::GetAngles() const noexcept { return cmds.front().viewangles; }

void CPlayback::EraseDeadFrames()
{
	const auto it = std::find_if(cmds.begin(), cmds.end(), [](const playback_cmd& cmd){
		return fvec3(cmd.velocity) != 0.f || cmd.forwardmove != 0 || cmd.rightmove != 0;
	});

	if (it == cmds.begin() || it == cmds.end())
		return;

	cmds.erase(cmds.begin(), it - 1);
}

/**
 * .
 * 
 * \return 
 */
CPlayerStatePlayback::CPlayerStatePlayback(std::vector<playback_cmd>&& data, std::vector<playerState_s>&& ps, const CPlaybackSettings& init)
	: CPlayback(std::move(data), init, false), playerStates(std::move(ps)) {

	m_objExtraHeader.m_uNumCmds = cmds.size();
	m_objExtraHeader.m_uNumPlayerStates = playerStates.size();

	const auto ratio = static_cast<std::size_t>(cmds.size() / playerStates.size()) + 1u;

	m_objExtraHeader.m_uPlayerStateToCmdRatio = ratio;

};
CPlayerStatePlayback::CPlayerStatePlayback(const std::vector<playback_cmd>& data, const std::vector<playerState_s>& ps, const CPlaybackSettings& init) 
	: CPlayback(data, init, false), playerStates(ps){

	m_objExtraHeader.m_uNumCmds = cmds.size();
	m_objExtraHeader.m_uNumPlayerStates = playerStates.size();

	const auto ratio = static_cast<std::size_t>(cmds.size() / playerStates.size()) + 1u;

	m_objExtraHeader.m_uPlayerStateToCmdRatio = ratio;
}

CPlayerStatePlayback::~CPlayerStatePlayback() = default;


bool CPlaybackIOWriter::Write() const
{
	if (m_pTarget->cmds.empty())
		return false;

	//Write the first index so that it overwrites all previous data
	const std::string header_content = std::string(
		reinterpret_cast<const char*>(&m_pTarget->m_objHeader), 
		reinterpret_cast<const char*>(&m_pTarget->m_objHeader) + sizeof(CPlayback::Header));

	if (!IO_Write(header_content))
		return false;

	for (const auto Frame : std::views::iota(0u, m_pTarget->cmds.size())) {

		const std::string content = std::string(
			reinterpret_cast<const char*>(&m_pTarget->cmds[Frame]),
			reinterpret_cast<const char*>(&m_pTarget->cmds[Frame]) + sizeof(playback_cmd));

		if (!IO_Append(content))
			return false;
	}



	return true;
}
bool CPlaybackIOReader::Read()
{
	auto buff = IO_Read();

	if (!buff)
		return false;

	auto& data = buff.value();
	CPlayback::Header header;
	if (data.length() < sizeof(header))
		return false;

	memcpy(&header, data.c_str(), sizeof(header));
	data = data.erase(0, sizeof(header));

	std::vector<playback_cmd> cmds;

	while (data.length() >= sizeof(playback_cmd)) {
		playback_cmd cmd;

		memcpy(&cmd, data.c_str(), sizeof(playback_cmd));
		data = data.erase(0, sizeof(playback_cmd));
		
		cmds.emplace_back(cmd);

	}

	if (cmds.empty())
		return false;

	m_objResult = std::make_unique<CPlayback>(cmds, 
		CPlaybackSettings{
			.m_iGSpeed = header.m_iSpeed,
			.m_eJumpSlowdownEnable = header.m_bJumpSlowdownEnable,
		});

	return true;
}
/***********************************************************************
 > 
***********************************************************************/

bool CPlayerStatePlaybackIOWriter::Write() const
{
	if (m_pTarget->cmds.empty())
		return false;

#pragma pack(push, 1)
	struct FullHeader {
		CPlayback::Header original;
		CPlayerStatePlayback::HeaderExtra extra;
	}header{ .original = m_pTarget->m_objHeader, .extra = m_pTarget->m_objExtraHeader };
#pragma pack(pop)


	//Write the first index so that it overwrites all previous data
	const std::string header_content = std::string(
		reinterpret_cast<const char*>(&header),
		reinterpret_cast<const char*>(&header) + sizeof(FullHeader));

	if (!IO_Write(header_content))
		return false;


	for (const auto Frame : std::views::iota(0u, m_pTarget->m_objExtraHeader.m_uNumCmds)) {

		const std::string content = std::string(
			reinterpret_cast<const char*>(&m_pTarget->cmds[Frame]),
			reinterpret_cast<const char*>(&m_pTarget->cmds[Frame]) + sizeof(playback_cmd));

		if (!IO_Append(content))
			return false;
	}

	for (const auto Frame : std::views::iota(0u, m_pTarget->m_objExtraHeader.m_uNumPlayerStates)) {

		const std::string content = std::string(
			reinterpret_cast<const char*>(&m_pTarget->playerStates[Frame]),
			reinterpret_cast<const char*>(&m_pTarget->playerStates[Frame]) + sizeof(playerState_s));

		if (!IO_Append(content))
			return false;
	}

	return true;
}
bool CPlayerStatePlaybackIOReader::Read()
{
	auto buff = IO_Read();

	if (!buff)
		return false;

	auto& data = buff.value();

#pragma pack(push, 1)
	struct FullHeader {
		CPlayback::Header original;
		CPlayerStatePlayback::HeaderExtra extra;
	}header;
#pragma pack(pop)

	if (data.length() < sizeof(header))
		return false;

	memcpy(&header, data.c_str(), sizeof(header));
	data = data.erase(0, sizeof(header));

	std::vector<playback_cmd> cmds;
	std::vector<playerState_s> states;

	//read cmds
	for([[maybe_unused]]auto i : std::views::iota(0u, header.extra.m_uNumCmds)) {
		playback_cmd cmd;

		memcpy(&cmd, data.c_str(), sizeof(playback_cmd));
		data = data.erase(0, sizeof(playback_cmd));

		cmds.emplace_back(cmd);

		if (data.length() < sizeof(playback_cmd))
			return false;
	}

	if (cmds.empty())
		return false;

	//read playerstates
	for ([[maybe_unused]] auto i : std::views::iota(0u, header.extra.m_uNumPlayerStates)) {
		playerState_s state;

		memcpy(&state, data.c_str(), sizeof(playerState_s));
		data = data.erase(0, sizeof(playerState_s));

		states.emplace_back(state);

		if (data.empty())
			break;
	}

	if (states.empty())
		return false;

	m_objResult = std::make_unique<CPlayerStatePlayback>(cmds, states,
		CPlaybackSettings{
			.m_iGSpeed = header.original.m_iSpeed,
			.m_eJumpSlowdownEnable = header.original.m_bJumpSlowdownEnable,
		});

	return true;
}

/***********************************************************************
 > 
***********************************************************************/
CDebugPlayback::CDebugPlayback(const std::vector<playback_cmd>& cmds) : m_oExpectedPlayback(cmds) {



}




