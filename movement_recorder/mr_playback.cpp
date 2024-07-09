#include <bg/bg_pmove_simulation.hpp>
#include "cg/cg_angles.hpp"
#include "mr_playback.hpp"
#include <dvar/dvar.hpp>
#include <ranges>
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "mr_record.hpp"
#include <cg/cg_client.hpp>
#include <com/com_channel.hpp>
#include <iostream>

CPlayback::CPlayback(std::vector<playback_cmd>&& _data, int g_speed, char jump_slowdownEnable)
	: cmds(std::forward<std::vector<playback_cmd>&&>(_data)) {
	m_iCmd = 0u;

	

	m_objHeader = { .m_iSpeed = g_speed, .m_bJumpSlowdownEnable = slowdown_t(jump_slowdownEnable) };
}
CPlayback::CPlayback(const std::vector<playback_cmd>& _data, int g_speed, char jump_slowdownEnable)
	: cmds(_data) {
	m_iCmd = 0u;

	m_objHeader = { .m_iSpeed = g_speed, .m_bJumpSlowdownEnable = slowdown_t(jump_slowdownEnable) };
}
CPlayback::~CPlayback() = default;
void CPlayback::TryFixingTime(usercmd_s* cmd, [[maybe_unused]]usercmd_s* oldcmd)
{
	cmd->serverTime = m_iFirstServerTime + (cmds[m_iCmd].serverTime - cmds.front().serverTime);
	clients->serverTime = cmd->serverTime;
	cgs->snap->serverTime = cmd->serverTime;

	const int ft = cmds[m_iCmd].serverTime - cmds[m_iCmd].oldTime;
	Dvar_FindMalleableVar("com_maxfps")->current.integer = 1000 / ft;

}

void CPlayback::DoPlayback(usercmd_s* cmd, [[maybe_unused]]usercmd_s* oldcmd)
{
	if (!IsPlayback())
		return;

	if (!m_iCmd) {

		//any slowdown is going to cause issues so just don't start
		
		//UPDATE: sometimes this causes serious issues with lineups!
		//if (cgs->predictedPlayerState.pm_time)
		//	return;

		m_iFirstServerTime = cmd->serverTime;
		m_iFirstOldServerTime = oldcmd->serverTime;

	}
	auto icmd = &cmds[m_iCmd];

	const auto deltas = icmd->viewangles.angle_delta(cgs->predictedPlayerState.delta_angles);

	(ivec3&)cmd->angles = deltas.to_short();
	(fvec3&)clients->viewangles = deltas;


	TryFixingTime(cmd, oldcmd);

	//cmd->serverTime = StartTime + (icmd->serverTime - cmds.begin()->serverTime);




	cmd->offHandIndex = icmd->offhand;
	cmd->weapon = icmd->weapon;

	cmd->forwardmove = icmd->forwardmove;
	cmd->rightmove = icmd->rightmove;
	cmd->buttons = icmd->buttons;

	m_iCmd++;

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
CPlayback::operator std::vector<playback_cmd>() { return cmds; }

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

	m_objResult = std::make_unique<CPlayback>(cmds, header.m_iSpeed, header.m_bJumpSlowdownEnable);
	return true;
}