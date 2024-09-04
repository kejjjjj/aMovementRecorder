#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "global_macros.hpp"

/***********************************************************************
 > the MOVEMENT_RECORDER macro is used because if other debug modules
 > need to use the movement recorder, they only want the most minimal
 > part of it
***********************************************************************/

enum is_segment_t : bool
{
	no_segmenting = false,
	segmenting = true,
};

enum is_lineup_t : bool
{
	no_lineup = false,
	lineup = true,
};

class CPlayback;
class CDebugPlayback;

struct CPlaybackSettings;

#if(MOVEMENT_RECORDER)
class CGuiMovementRecorder;
class CRMovementRecorder;
class CPlaybackSegmenter;
class CLineupPlayback;
class CLineup;
class CRecorder;
class CPlaybackGui;
class CPlaybackSimulation;
#endif

struct playback_cmd;
struct usercmd_s;
struct playerState_s;
struct GfxViewParms;

/***********************************************************************
 > CMovementRecorder is a class that handles both recording, segmenting, and playbacks
 > This class can hold a playback queue which means that it will play all inserted playbacks in order
 > This class expects that the update function is called from CL_FinishMove
***********************************************************************/
class CMovementRecorder
{
	NONCOPYABLE(CMovementRecorder);
	friend class CStaticMovementRecorder;

#if(MOVEMENT_RECORDER)
	friend class CRMovementRecorder;
	friend class CGuiMovementRecorder;
	friend class CMovementRecorderIO;
	friend class CRBMovementRecorder;
#endif

public:
	CMovementRecorder();
	virtual ~CMovementRecorder();
	void Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd);

#if(MOVEMENT_RECORDER)

	// Recording control
	void StartRecording(bool start_from_movement = false, bool includePlayerState = false);
	void StopRecording();
	bool IsRecording() const noexcept;

	// Segmenting control
	void StopSegmenting();
	bool IsSegmenting() const noexcept;

	// Playback control
	void OnPositionLoaded();
	void SelectPlayback();
	void OnDisconnect();

	// Add playback to the queue or segmenter
	void PushPlayback(CPlayback&& playback, is_segment_t segmenting_allowed = no_segmenting, is_lineup_t do_lineup = no_lineup);
	void PushPlayback(const CPlayback& playback, is_segment_t segmenting_allowed = no_segmenting, is_lineup_t do_lineup = no_lineup);
#else
	void PushPlayback(CPlayback&& playback);
	void PushPlayback(const CPlayback& playback);
#endif

	void AddDebugPlayback(const std::vector<playback_cmd>& playback);
	CDebugPlayback* GetDebugPlayback() const noexcept;
	void ClearDebugPlayback();


	CPlayback* GetActivePlayback();
	CPlayback* GetActivePlayback() const;

#if(MOVEMENT_RECORDER)
	const CPlaybackSimulation* GetSimulation() const { return Simulation.get(); }
#endif

protected:

	//Playback
	std::queue<std::unique_ptr<CPlayback>> PlaybackQueue;
	CPlayback* PlaybackActive = {};

	std::unique_ptr<CDebugPlayback> m_pDebugPlayback;

#if(MOVEMENT_RECORDER)
	std::map<std::string, std::unique_ptr<CPlayback>> LevelPlaybacks;

	//Segmenting
	std::unique_ptr<CPlaybackSegmenter> Segmenter;
	
	//Recorder
	std::unique_ptr<std::vector<playback_cmd>> PendingRecording;
	std::vector<playerState_s> PendingRecordingPlayerStates;
	std::unique_ptr<CRecorder> Recorder;
	

	//Lineup
	std::unique_ptr<CLineupPlayback> Lineup;

	//Simulations
	std::unique_ptr<CPlaybackSimulation> Simulation;
#endif

private:
#if(MOVEMENT_RECORDER)
	void UpdateLineup(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd);
#endif

	void UpdatePlaybackQueue(usercmd_s* cmd, const usercmd_s* oldcmd);

};

#if(MOVEMENT_RECORDER)


class CRMovementRecorder
{
	NONCOPYABLE(CRMovementRecorder);

public:

	CRMovementRecorder(CMovementRecorder& recorder)
		: m_oRefMovementRecorder(recorder) {}

	void CG_Render() const;

private:
	void CG_RenderOrigins() const;
	void CG_RenderPrecision() const;
	void CG_RenderStatus() const;

	CMovementRecorder& m_oRefMovementRecorder;

};

class CRBMovementRecorder
{
	NONCOPYABLE(CRBMovementRecorder);

public:
	CRBMovementRecorder(CMovementRecorder& recorder);
	~CRBMovementRecorder();

	void RB_Render(GfxViewParms* viewParms) const;
private:
	void RB_RenderOrigins() const;

	CMovementRecorder& m_oRefMovementRecorder;
};

class CGuiMovementRecorder
{
	friend class CMovementRecorderWindow;
public:

	CGuiMovementRecorder(CMovementRecorder* recorder);
	~CGuiMovementRecorder();

	void RenderLevelRecordings();

private:

	CMovementRecorder* m_oRefMovementRecorder = 0;
	size_t m_uSelectedIndex = {};
	std::shared_ptr<CPlaybackGui> m_pItem;
};

class CMovementRecorderIO
{
	NONCOPYABLE(CMovementRecorderIO);

public:
	CMovementRecorderIO(CMovementRecorder& recorder)
		: m_oRefMovementRecorder(recorder) {}

	[[maybe_unused]] bool SaveToDisk(const std::string& name, const std::vector<playback_cmd>& cmds);
	[[maybe_unused]] bool SavePlayerStatePlaybackToDisk(const std::string& name, const std::vector<playback_cmd>& cmds, const std::vector<playerState_s>& ps);

	//pushes the file to LevelPlaybacks
	[[maybe_unused]] bool LoadFromDisk(const std::string& name);
	[[maybe_unused]] bool LoadPlayerStatePlaybackFromDisk(const std::string& name);

	[[maybe_unused]] bool DeleteFileFromDisk(const std::string& name);


	[[maybe_unused]] bool RefreshAllLevelPlaybacks();

private:
	CMovementRecorder& m_oRefMovementRecorder;

};

#endif

class CStaticMovementRecorder
{
public:
	static std::unique_ptr<CMovementRecorder> Instance;

	//for other modules that need to use the movement recorder
	static void PushPlayback(std::vector<playback_cmd>&& cmds, const CPlaybackSettings& init);
	static void PushPlayback(const std::vector<playback_cmd>& cmds, const CPlaybackSettings& init);
	static bool GetActivePlayback() noexcept;

#if(MOVEMENT_RECORDER)

	//console commands
	static void ToggleRecording();
	static void ToggleRecordingWithPlayerState();

	static void SelectPlayback();
	static void OnDisconnect();
	static void Save();
	static void TeleportTo();
	static void Clear();
	static void Simulation();


	//automatically load all recordings for the level
	static void Update();

private:

	static bool m_bPlaybacksLoaded;

#endif

};
