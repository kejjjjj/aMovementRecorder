#include "mr_lineup.hpp"
#include "mr_main.hpp"
#include "mr_playback.hpp"
#include "mr_record.hpp"
#include "mr_segmenter.hpp"
#include <cg/cg_angles.hpp>
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>
#include <cl/cl_move.hpp>
#include <com/com_channel.hpp>
#include <dvar/dvar.hpp>
#include <net/nvar_table.hpp>
#include <cg/cg_client.hpp>
#include <fs/fs_globals.hpp>
#include <Windows.h>
#include "bg/bg_pmove_simulation.hpp"
#include <cl/cl_utils.hpp>
#include "mr_tests.hpp"

std::unique_ptr<CMovementRecorder> CStaticMovementRecorder::Instance = std::make_unique<CMovementRecorder>();

CMovementRecorder::CMovementRecorder()
{

}

CMovementRecorder::~CMovementRecorder() = default;
void CMovementRecorder::Update(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd)
{

	UpdateLineup(cmd, oldcmd);
	UpdatePlaybackQueue(cmd, oldcmd);

	//Moved these below so that the recorder&segmenter gets the newest cmds
	if (Recorder) {
		Recorder->Record(ps, cmd, oldcmd);
	}

	if (Segmenter && !Segmenter->Update(cmd, oldcmd, ps)) {
		//this means that the playback ended without any user inputs
		Segmenter.reset();

	}
}

void CMovementRecorder::StartRecording(bool start_from_movement) {
	PendingRecording.reset();

	//wait 300 server frames before starting the recoding so that SetOrigin() has enough time to set the position 
	Recorder = std::make_unique<CRecorder>(start_from_movement, 300);
}
void CMovementRecorder::StopRecording() {
	if (!Recorder)
		return;

	PendingRecording = std::make_unique<std::vector<playback_cmd>>(Recorder->StopRecording());
	Recorder.reset();

}
bool CMovementRecorder::IsRecording() const noexcept { return Recorder != nullptr; }
void CMovementRecorder::StopSegmenting() {
	if (!Segmenter)
		return;

	if (Segmenter->ResultExists()) {

		if (auto result = Segmenter->GetResult()) {
			PendingRecording = std::make_unique<std::vector<playback_cmd>>(result.value());
		}
		else {
			Com_Printf("the recording failed\n");
		}
	}

	Segmenter.reset();
}
bool CMovementRecorder::IsSegmenting() const noexcept { return Segmenter != nullptr; }
void CMovementRecorder::OnPositionLoaded()
{
	if (Recorder) {
		StopRecording();

		StartRecording(true);
	}

	//loading position resets the segmenter
	if (Segmenter) {
		Segmenter.reset();
	}

}
CPlayback* CMovementRecorder::GetActivePlayback() {

	
	if (Segmenter && !Segmenter->ResultExists())
		return Segmenter->GetPlayback();


	if (Lineup)
		return &Lineup->GetPlayback();

	return PlaybackActive;

}
CPlayback* CMovementRecorder::GetActivePlayback() const {
	if (Segmenter)
		return Segmenter->GetPlayback();

	if (Lineup)
		return &Lineup->GetPlayback();

	return PlaybackActive;

}
void CMovementRecorder::SelectPlayback()
{
	const is_segment_t segmenting_allowed = static_cast<is_segment_t>(NVar_FindMalleableVar<bool>("Segmenting")->Get());

	if (PendingRecording) {
		PushPlayback(CPlayback(
			std::vector<playback_cmd>(*PendingRecording),
			{ 
				.g_speed = CG_GetSpeed(&cgs->predictedPlayerState),
				.jump_slowdownEnable = Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
				.ignorePitch = 	NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
			}), 

			segmenting_allowed, lineup);

		return;
	}
	
	else if (LevelPlaybacks.size()) {
		
		using playback_t = std::remove_reference<decltype(*LevelPlaybacks.begin())>::type;

		//Get all playbacks with the same g_speed and jump_slowdownEnable
		std::vector<CPlayback*> relevant_playbacks;
		std::ranges::for_each(LevelPlaybacks, [ps=&cgs->predictedPlayerState, &relevant_playbacks](const playback_t& pb) {
			if (pb.second->IsCompatibleWithState(ps))
				relevant_playbacks.push_back(pb.second.get());
			});

		//guess there's nothing :(
		if (relevant_playbacks.empty())
			return;

		//get closest playback
		const auto closest = std::min_element(relevant_playbacks.begin(), relevant_playbacks.end(),
			[ps = &cgs->predictedPlayerState](const CPlayback* left, const CPlayback* right) {
				return left->GetOrigin().dist_sq(ps->origin) < right->GetOrigin().dist_sq(ps->origin);
			});

		(*closest)->IgnorePitch(NVar_FindMalleableVar<bool>("Ignore Pitch")->Get());

		return PushPlayback(**closest, segmenting_allowed, lineup);

	}

	return Com_Printf("^1there is nothing to playback\n");


}
void CMovementRecorder::OnDisconnect()
{
	PendingRecording.reset();
	Recorder.reset();
	PlaybackActive = {};
	LevelPlaybacks.clear();
	Lineup.reset();

	while (PlaybackQueue.size())
		PlaybackQueue.pop();


}

void CMovementRecorder::PushPlayback(CPlayback&& playback, is_segment_t segmenting_allowed, is_lineup_t do_lineup) 
{
	if (playback.cmds.empty())
		return;

	if (do_lineup == lineup) {
		Lineup = std::make_unique<CLineupPlayback>(playback, playback.GetOrigin(), playback.GetAngles());
		return;
	}

	if (segmenting_allowed == segmenting) {
		Segmenter = std::make_unique<CPlaybackSegmenter>(playback);
		return;
	}

	PlaybackQueue.emplace(std::make_unique<CPlayback>(std::forward<CPlayback&&>(playback)));
}
void CMovementRecorder::PushPlayback(const CPlayback& playback, is_segment_t segmenting_allowed, is_lineup_t do_lineup) 
{
	if (playback.cmds.empty())
		return;

	if (do_lineup == lineup) {
		Lineup = std::make_unique<CLineupPlayback>(playback, playback.GetOrigin(), playback.GetAngles());
		return;
	}

	if (segmenting_allowed == segmenting) {
		Segmenter = std::make_unique<CPlaybackSegmenter>(playback);
		return;
	}

	PlaybackQueue.emplace(std::make_unique<CPlayback>(playback));
}
void CMovementRecorder::UpdateLineup(usercmd_s* cmd, usercmd_s* oldcmd)
{
	if (!Lineup)
		return;

	if (!Lineup->Update(cmd, oldcmd)) {
		const is_segment_t segmenting_allowed = static_cast<is_segment_t>(NVar_FindMalleableVar<bool>("Segmenting")->Get());

		if (Lineup->Finished()) {
			PushPlayback(Lineup->GetPlayback(), segmenting_allowed, no_lineup);
		}

		Lineup.reset();
		return;
	}



}
void CMovementRecorder::UpdatePlaybackQueue(usercmd_s* cmd, [[maybe_unused]]usercmd_s* oldcmd)
{
	if (PlaybackQueue.empty())
		return;

	const auto ps = &cgs->predictedPlayerState;

	//set a new active
	if (!PlaybackActive) {
		PlaybackActive = PlaybackQueue.front().get();
	}

	if (ps->pm_type != PM_NORMAL || WASD_PRESSED())
		PlaybackActive->StopPlayback();

	PlaybackActive->DoPlayback(cmd, oldcmd);

	if (!PlaybackActive->IsPlayback()) {
		PlaybackActive = {};
		PlaybackQueue.pop();
		return;
	}



}
void CStaticMovementRecorder::SelectPlayback()
{
	return Instance->SelectPlayback();
}
void CStaticMovementRecorder::ToggleRecording()
{
	if (cgs->predictedPlayerState.pm_type != PM_NORMAL)
		return;

	if (Instance->IsRecording())
		return Instance->StopRecording();

	if (Instance->IsSegmenting())
		return Instance->StopSegmenting();


	Instance->StartRecording();
}
void CStaticMovementRecorder::PushPlayback(std::vector<playback_cmd>&& cmds, const PlaybackInitializer& init) {
	Instance->PushPlayback(CPlayback(std::move(cmds), init));
}
void CStaticMovementRecorder::PushPlayback(const std::vector<playback_cmd>& cmds, const PlaybackInitializer& init) {
	Instance->PushPlayback(CPlayback(cmds, init), no_segmenting, no_lineup);
}

void CStaticMovementRecorder::OnDisconnect() {
	m_bPlaybacksLoaded = false;

	return Instance->OnDisconnect();
}
void CStaticMovementRecorder::Save() {

	if (CL_ConnectionState() != CA_ACTIVE)
		return;

	const int num_args = cmd_args->argc[cmd_args->nesting];

	if (num_args != 2) {
		return Com_Printf("usage: mr_save <filename>\n");
	}

	if (!Instance->PendingRecording)
		return Com_Printf("^1there is nothing to save\n");

	const char* filename = *(cmd_args->argv[cmd_args->nesting] + 1);

	CMovementRecorderIO io(*Instance);

	if (io.SaveToDisk(filename, *Instance->PendingRecording))
		Instance->PendingRecording.reset();

}

void CStaticMovementRecorder::TeleportTo() {

	if (CL_ConnectionState() != CA_ACTIVE)
		return;

	const std::int32_t num_args = cmd_args->argc[cmd_args->nesting];

	if (num_args != 2) {
		return Com_Printf("usage: mr_teleportTo <filename>\n");
	}

	const auto file = *(cmd_args->argv[cmd_args->nesting] + 1);
	const auto item = Instance->LevelPlaybacks.find(file);

	if(item == Instance->LevelPlaybacks.end())
		return Com_Printf(CON_CHANNEL_CONSOLEONLY, "'%s' doesn't exist\n", file);

	*(fvec3*)(ps_loc->origin) = item->second->GetOrigin();
	CL_SetPlayerAngles(CL_GetUserCmd(clients->cmdNumber - 1), cgs->predictedPlayerState.delta_angles, item->second->GetAngles());


}

bool CStaticMovementRecorder::m_bPlaybacksLoaded = false;

void CStaticMovementRecorder::Update()
{
	if (CL_ConnectionState() == CA_ACTIVE && !m_bPlaybacksLoaded) {
		
		CMovementRecorderIO io(*Instance);
		io.RefreshAllLevelPlaybacks();
		m_bPlaybacksLoaded = true;
	}


}
void CStaticMovementRecorder::Clear() {
	Instance->PendingRecording.reset();
};

bool CStaticMovementRecorder::GetActivePlayback() noexcept
{
	return Instance->GetActivePlayback();
}

void CStaticMovementRecorder::Simulation()
{
	auto ps = &cgs->predictedPlayerState;

	if (ps->pm_type != PM_NORMAL || !Instance->PendingRecording)
		return;

	const CPlayback playback(*Instance->PendingRecording,
		{
			.g_speed = CG_GetSpeed(&cgs->predictedPlayerState),
			.jump_slowdownEnable = Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
			.ignorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get()
		});



	const auto origin = Instance->PendingRecording->front().origin;
	const auto angles = Instance->PendingRecording->front().viewangles;

	Instance->Simulation = std::make_unique<CPlaybackSimulation>(playback);
	auto& sim = *Instance->Simulation;



	sim.Simulate(origin, angles);
	//const auto results = sim.GetAnalysis();
	//const auto pm = &results->pm;
	//const auto dist = Instance->PendingRecording->back().origin.dist(pm->ps->origin);

	////(fvec3&)ps_loc->origin = pm->ps->origin;
	////(fvec3&)ps_loc->viewangles = pm->ps->viewangles;

	//Com_Printf("distance: ^1%.6f\n", dist);


}
