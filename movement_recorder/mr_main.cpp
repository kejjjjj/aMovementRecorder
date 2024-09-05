#include "mr_main.hpp"
#include "mr_playback.hpp"

#if(MOVEMENT_RECORDER)
#include "mr_lineup.hpp"
#include "mr_record.hpp"
#include "mr_segmenter.hpp"
#include "mr_tests.hpp"
#include "mr_playbackeditor.hpp"
#endif

#include "cg/cg_angles.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "cl/cl_move.hpp"
#include "com/com_channel.hpp"
#include "dvar/dvar.hpp"
#include "net/nvar_table.hpp"
#include "cg/cg_client.hpp"
#include "fs/fs_globals.hpp"
#include "bg/bg_pmove_simulation.hpp"
#include "cl/cl_utils.hpp"
#include "utils/functions.hpp"

#include <Windows.h>


std::unique_ptr<CMovementRecorder> CStaticMovementRecorder::Instance = std::make_unique<CMovementRecorder>();

CMovementRecorder::CMovementRecorder()
{

}

CMovementRecorder::~CMovementRecorder() = default;
void CMovementRecorder::Update([[maybe_unused]]const playerState_s* ps, usercmd_s* cmd, const  usercmd_s* oldcmd)
{

#if(MOVEMENT_RECORDER)

	if (TimedPlayback) {

		if (!TimedPlaybackTiming)
			CreateTimedPlayback();

		if (level->time < TimedPlaybackTiming) {
			return;
		}

		TimedPlayback = nullptr;
		TimedPlaybackTiming = 0;
	}

	UpdateLineup(ps, cmd, oldcmd);
	UpdatePlaybackQueue(cmd, oldcmd);

	//Moved these below so that the recorder&segmenter gets the newest cmds
	if (Recorder) {
		Recorder->Record(ps, cmd, oldcmd);
	}

	if (Segmenter && !Segmenter->Update(ps, cmd, oldcmd)) {
		//this means that the playback ended without any user inputs
		Segmenter.reset();

	}
#else
	UpdatePlaybackQueue(cmd, oldcmd);
#endif

}
void CMovementRecorder::UpdatePlaybackQueue(usercmd_s* cmd, const usercmd_s* oldcmd)
{
	if (PlaybackQueue.empty())
		return;

	const auto ps = &cgs->predictedPlayerState;

	//set a new active
	if (!PlaybackActive) {
		PlaybackActive = PlaybackQueue.front().get();

	}

	if (!PlaybackActive)
		return;

	if (ps->pm_type != PM_NORMAL || WASD_PRESSED() && !PlaybackActive->bIgnoreWASD())
		PlaybackActive->StopPlayback();

	PlaybackActive->DoPlayback(cmd, oldcmd);

	if (!PlaybackActive->IsPlayback()) {
		PlaybackActive = {};
		PlaybackQueue.pop();
		return;
	}



}
#if(MOVEMENT_RECORDER)
void CMovementRecorder::StartRecording(bool start_from_movement, bool includePlayerState) {
	PendingRecording.reset();

	//wait 300 server frames before starting the recoding so that SetOrigin() has enough time to set the position 
	if(!includePlayerState)
		Recorder = std::make_unique<CRecorder>(start_from_movement, 300);
	else
		Recorder = std::make_unique<CPlayerStateRecorder>(start_from_movement, 300);

}
void CMovementRecorder::StopRecording() {
	if (!Recorder)
		return;

	PendingRecording = std::make_unique<std::vector<playback_cmd>>(Recorder->StopRecording());

	//kind of a silly hardcoded thing, but I doubt the recorder will extend functionality after this :clueless:
	if (Recorder->AmIDerived()) {
		const auto r = dynamic_cast<CPlayerStateRecorder*>(Recorder.get());
		PendingRecordingPlayerStates = r->playerState;
	}
	Recorder.reset();

}
bool CMovementRecorder::IsRecording() const noexcept { return Recorder != nullptr; }
void CMovementRecorder::StopSegmenting() {
	if (!Segmenter)
		return;

	if (Segmenter->ResultExists()) {

		if (auto result = Segmenter->GetResult()) {
			PendingRecording = std::make_unique<std::vector<playback_cmd>>(result->cmds);
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
		const bool wasDerived = Recorder->AmIDerived();
		StopRecording();
		StartRecording(true, wasDerived);
	}

	//loading position resets the segmenter
	if (Segmenter) {
		Segmenter.reset();
	}

}

void CMovementRecorder::SelectPlayback()
{
	const is_segment_t segmenting_allowed = static_cast<is_segment_t>(NVar_FindMalleableVar<bool>("Segmenting")->Get());

	if (InEditor()) {
		TimedPlayback = Editor->GetCurrentState();

		if (CG_IsOnGround(TimedPlayback)) {
			TimedPlayback = 0;
			Com_Printf("^1you can only continue from a point where the player is in the air\n");
		}

		return;
	}

	if (PendingRecording) {
		PushPlayback(CPlayback(
			std::vector<playback_cmd>(*PendingRecording),
			{
				.m_iGSpeed = CG_GetSpeed(&cgs->predictedPlayerState),
				.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
				.m_bIgnorePitch = 	NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
				.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),

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
		(*closest)->IgnoreWeapon(NVar_FindMalleableVar<bool>("Ignore Weapon")->Get());

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
	Editor.reset();

	while (PlaybackQueue.size())
		PlaybackQueue.pop();


}

void CMovementRecorder::PushPlayback(CPlayback&& playback, is_segment_t segmenting_allowed, is_lineup_t do_lineup) 
{
	if (playback.cmds.empty())
		return;

	if (do_lineup == lineup) {
		Lineup = std::make_unique<CLineupPlayback>(&cgs->predictedPlayerState, playback, playback.GetOrigin(), playback.GetAngles());
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
		Lineup = std::make_unique<CLineupPlayback>(&cgs->predictedPlayerState, playback, playback.GetOrigin(), playback.GetAngles());
		return;
	}

	if (segmenting_allowed == segmenting) {
		Segmenter = std::make_unique<CPlaybackSegmenter>(playback);
		return;
	}

	PlaybackQueue.emplace(std::make_unique<CPlayback>(playback));
}
void CMovementRecorder::UpdateLineup(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd)
{
	if (!Lineup)
		return;

	if (!Lineup->Update(ps, cmd, oldcmd)) {
		const is_segment_t segmenting_allowed = static_cast<is_segment_t>(NVar_FindMalleableVar<bool>("Segmenting")->Get());

		if (Lineup->Finished()) {
			PushPlayback(Lineup->GetPlayback(), segmenting_allowed, no_lineup);
		}

		Lineup.reset();
		return;
	}



}

void CMovementRecorder::CreateTimedPlayback()
{
	std::vector<playback_cmd> copy;
	auto& cmds = Editor->m_oRefPlayback.cmds;

	copy.resize(cmds.size() - Editor->GetCurrentCmdOffset());

	std::copy(cmds.begin() + Editor->GetCurrentCmdOffset(), cmds.end(), copy.begin());

	PlaybackQueue.emplace(
		std::make_unique<CPlayback>(copy,
			CPlaybackSettings{
				.m_iGSpeed = CG_GetSpeed(&cgs->predictedPlayerState),
				.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
				.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
				.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),
			}, false));


	const auto oldTime = ps_loc->commandTime;
	auto oldSprint = ps_loc->sprintState;
	const auto oldJumpTime = ps_loc->jumpTime;

	oldSprint.sprintButtonUpRequired = 0;
	oldSprint.lastSprintStart = 0;
	oldSprint.lastSprintEnd = 0;

	*ps_loc = *TimedPlayback;
	ps_loc->commandTime = oldTime;
	ps_loc->sprintState = oldSprint;
	ps_loc->jumpTime = oldJumpTime;

	const auto& f = PlaybackQueue.front()->cmds.front();

	level->time = level->previousTime + (f.serverTime - f.oldTime);
	TimedPlaybackTiming = level->time;


}
void CStaticMovementRecorder::SelectPlayback()
{
	if (CL_ConnectionState() != CA_ACTIVE)
		return;

	if (Instance->GetActivePlayback()) {
		Instance->PlaybackActive = {};

		if (Instance->Segmenter)
			Instance->Segmenter.reset();

		if(!Instance->PlaybackQueue.empty())
			Instance->PlaybackQueue.pop();
		return;
	}

	return Instance->SelectPlayback();
}
void CStaticMovementRecorder::ToggleRecording()
{
	if (CL_ConnectionState() != CA_ACTIVE || cgs->predictedPlayerState.pm_type != PM_NORMAL)
		return;

	if (Instance->InEditor())
		return Com_Printf("not allowed when editing a playback\n");

	if (Instance->GetActivePlayback()) {
		if (Instance->Segmenter)
			return Instance->Segmenter->StartSegmenting();

		return;
	}

	if (Instance->IsRecording())
		return Instance->StopRecording();

	if (Instance->IsSegmenting())
		return Instance->StopSegmenting();


	Instance->StartRecording();
}
void CStaticMovementRecorder::ToggleRecordingWithPlayerState()
{
	if (CL_ConnectionState() != CA_ACTIVE || cgs->predictedPlayerState.pm_type != PM_NORMAL)
		return;

	if (!Dvar_FindMalleableVar("sv_running")->current.enabled)
		return Com_Printf("^1this command is only available when sv_running is 1");

	if (Instance->InEditor())
		return Com_Printf("not allowed when editing a playback\n");


	if (Instance->GetActivePlayback()) {
		//no such thing as segmenting here LOL
		Com_Printf("^1Segmenting playerstate recordings is not supported!\n");
		return;
	}

	if (Instance->IsRecording())
		return Instance->StopRecording();

	if (Instance->IsSegmenting())
		return Instance->StopSegmenting();


	Instance->StartRecording(false, true);
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

	
	if(!Instance->PendingRecordingPlayerStates.empty())
		if (io.SavePlayerStatePlaybackToDisk(filename, *Instance->PendingRecording, Instance->PendingRecordingPlayerStates))
			Instance->PendingRecording.reset();

	else if (io.SaveToDisk(filename, *Instance->PendingRecording))
		Instance->PendingRecording.reset();

}

void CStaticMovementRecorder::TeleportTo() {

	if (CL_ConnectionState() != CA_ACTIVE)
		return;

	const std::int32_t num_args = cmd_args->argc[cmd_args->nesting];

	if (num_args != 2) {
		return Com_Printf("usage: mr_teleportTo <filename>\n");
	}

	if(Instance->InEditor())
		return Com_Printf("not allowed when editing a playback\n");


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


void CStaticMovementRecorder::Simulation()
{
	auto ps = &cgs->predictedPlayerState;

	if (ps->pm_type != PM_NORMAL || !Instance->PendingRecording)
		return;

	const CPlayback playback(*Instance->PendingRecording,
		{
			.m_iGSpeed = CG_GetSpeed(&cgs->predictedPlayerState),
			.m_eJumpSlowdownEnable = (slowdown_t)Dvar_FindMalleableVar("jump_slowdownEnable")->current.enabled,
			.m_bIgnorePitch = NVar_FindMalleableVar<bool>("Ignore Pitch")->Get(),
			.m_bIgnoreWeapon = NVar_FindMalleableVar<bool>("Ignore Weapon")->Get(),

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
void CStaticMovementRecorder::AdvanceEditor()
{
	if (CL_ConnectionState() != CA_ACTIVE)
		return;


	const auto numArgs = cmd_args->argc[cmd_args->nesting];

	if (numArgs != 2)
		return Com_Printf("usage: mr_advance <number of frames>");

	if(!IsInteger(cmd_args->argv[cmd_args->nesting][1]))
		return Com_Printf("^1argument must be integral");

	if(!Dvar_FindMalleableVar("sv_running")->current.enabled)
		return Com_Printf("^1this command is only available when sv_running is 1");

	if(!Instance->InEditor())
		return Com_Printf("^1this command is only available when editing a playback");

	const auto frameCount = std::stoi(cmd_args->argv[cmd_args->nesting][1]);

	Instance->Editor->Advance(frameCount);



}
#else
void CMovementRecorder::PushPlayback(CPlayback&& playback)
{
	if (playback.cmds.empty())
		return;

	PlaybackQueue.emplace(std::make_unique<CPlayback>(std::forward<CPlayback&&>(playback)));
}
void CMovementRecorder::PushPlayback(const CPlayback& playback)
{
	if (playback.cmds.empty())
		return;
	
	PlaybackQueue.emplace(std::make_unique<CPlayback>(playback));
}
#endif


void CMovementRecorder::AddDebugPlayback(const std::vector<playback_cmd>& playback)
{
	m_pDebugPlayback = std::make_unique<CDebugPlayback>(playback);
}
CDebugPlayback* CMovementRecorder::GetDebugPlayback() const noexcept
{
	return m_pDebugPlayback ? m_pDebugPlayback.get() : nullptr;
}
void CMovementRecorder::ClearDebugPlayback()
{
	m_pDebugPlayback = 0;
}
bool CMovementRecorder::InEditor() const noexcept
{
	return Editor != nullptr;
}
void CMovementRecorder::CreateEditor(const CPlayerStatePlayback& playback)
{
	DeleteEditor();
	Editor = std::make_unique<CPlaybackEditor>(playback);
}
void CMovementRecorder::DeleteEditor()
{
	Editor.reset();
	Editor = 0;
}
CPlayback* CMovementRecorder::GetActivePlayback() {

#if(MOVEMENT_RECORDER)
	if (Segmenter && !Segmenter->ResultExists())
		return Segmenter->GetPlayback();


	if (Lineup)
		return &Lineup->GetPlayback();
#endif

	return PlaybackActive;

}
CPlayback* CMovementRecorder::GetActivePlayback() const {
#if(MOVEMENT_RECORDER)

	if (Segmenter)
		return Segmenter->GetPlayback();

	if (Lineup)
		return &Lineup->GetPlayback();
#endif
	return PlaybackActive;

}

void CStaticMovementRecorder::PushPlayback(std::vector<playback_cmd>&& cmds, const CPlaybackSettings& init) {
	if(Instance)
		Instance->PushPlayback(CPlayback(std::move(cmds), init));
}
void CStaticMovementRecorder::PushPlayback(const std::vector<playback_cmd>& cmds, const CPlaybackSettings& init) {
	if (Instance)
		Instance->PushPlayback(CPlayback(cmds, init));
}
bool CStaticMovementRecorder::GetActivePlayback() noexcept
{
	return Instance->GetActivePlayback();
}