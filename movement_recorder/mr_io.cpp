#include "bg/bg_pmove_simulation.hpp"
#include "cg/cg_local.hpp"
#include "cl/cl_utils.hpp"
#include "com/com_channel.hpp"
#include "dvar/dvar.hpp"
#include "fs/fs_globals.hpp"
#include "mr_main.hpp"
#include "mr_playback.hpp"
#include <cassert>

bool CMovementRecorderIO::SaveToDisk(const std::string& name, const std::vector<playback_cmd>& cmds)
{
	if (!fs::valid_file_name(name)) {
		Com_Printf("^1invalid name\n");
		return false;
	}

	const CPlayback pb(cmds, {});
	const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
	const auto writer = std::make_unique<CPlaybackIOWriter>(&pb, mapname + "\\" + name);

	if (writer->Write()) {
		Com_Printf("^2saved\n");

		//now refresh everything in case it overwrote an existing file
		RefreshAllLevelPlaybacks();
		return true;
	}


	Com_Printf("^1failed to save\n");
	return false;

}
bool CMovementRecorderIO::SavePlayerStatePlaybackToDisk(const std::string& name, const std::vector<playback_cmd>& cmds, const std::vector<playerState_s>& ps)
{

	if (!fs::valid_file_name(name)) {
		Com_Printf("^1invalid name\n");
		return false;
	}

	assert((cmds.size() / ps.size() == PLAYERSTATE_TO_CMD_RATIO));

	const CPlayerStatePlayback pb(cmds, ps, {});
	const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
	const auto writer = std::make_unique<CPlayerStatePlaybackIOWriter>(&pb, mapname + "\\Long\\" + name);

	if (writer->Write()) {
		Com_Printf("^2saved\n");

		//now refresh everything in case it overwrote an existing file
		RefreshAllLevelPlaybacks();
		return true;
	}


	Com_Printf("^1failed to save\n");
	return false;

}
bool CMovementRecorderIO::LoadFromDisk(const std::string& path)
{
	if (CL_ConnectionState() != CA_ACTIVE)
		return false;



	const auto reader = std::make_unique<CPlaybackIOReader>(path);

	if (reader->Read()) {
		const auto name = fs::get_file_name_no_extension(path);

		m_oRefMovementRecorder.LevelPlaybacks[name] = std::move(reader->m_objResult);
		return true;
	}

	const auto error = fs::get_last_error();
	Com_Printf("^1failed to load '%s' - %s\n", path.c_str(), error.c_str());
	return false;
}
bool CMovementRecorderIO::LoadPlayerStatePlaybackFromDisk(const std::string& path)
{
	if (CL_ConnectionState() != CA_ACTIVE)
		return false;

	const auto reader = std::make_unique<CPlayerStatePlaybackIOReader>(path);

	if (reader->Read()) {
		const auto name = fs::get_file_name_no_extension(path);

		m_oRefMovementRecorder.LevelPlaybacks[name] = std::move(reader->m_objResult);
		return true;
	}

	Com_Printf("^1failed to load '%s'\n", path.c_str());
	return false;
}
bool CMovementRecorderIO::DeleteFileFromDisk(const std::string& name)
{
	const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
	const auto file = (AGENT_DIRECTORY() + "\\Playbacks\\" + mapname + "\\" + name);

	if (!fs::file_exists(file)) {
		Com_Printf("^1the file doesn't exist");
		return false;
	}

	return std::filesystem::remove(file);
}
bool CMovementRecorderIO::DeletePlayerStatePlaybackFileFromDisk(const std::string& name)
{
	const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
	const auto file = (AGENT_DIRECTORY() + "\\Playbacks\\" + mapname + "\\Long\\" + name);

	if (!fs::file_exists(file)) {
		Com_Printf("^1the file doesn't exist");
		return false;
	}

	return std::filesystem::remove(file);
}
bool CMovementRecorderIO::RefreshAllLevelPlaybacks()
{
	m_oRefMovementRecorder.OnDisconnect();

	const std::string mapname = Dvar_FindMalleableVar("mapname")->current.string;
	auto path = AGENT_DIRECTORY() + "\\Playbacks\\" + mapname;

	auto directory = fs::files_in_directory(path);

	for (const auto& file : directory){

		//recording files don't have a filetype
		if(fs::get_extension(file).empty())
			LoadFromDisk(mapname + '\\' + fs::get_file_name(file));
	
	}

	const auto longPlaybackDirectory = path + "\\Long";
	directory = fs::files_in_directory(longPlaybackDirectory);

	for (const auto& file : directory) {

		//recording files don't have a filetype
		if (fs::get_extension(file).empty())
			LoadPlayerStatePlaybackFromDisk(mapname + "\\Long\\" + fs::get_file_name(file));

	}

	return true;
}
