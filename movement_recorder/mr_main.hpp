#pragma once

#include "global_macros.hpp"

#include <queue>
#include <com/com_channel.hpp>
#include <map>
#include <memory>

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

class CGuiMovementRecorder;
class CRMovementRecorder;
class CPlaybackSegmenter;
class CLineupPlayback;
class CLineup;
class CRecorder;
class CPlayback;
class CPlaybackGui;
struct PlaybackInitializer;

struct playback_cmd;
struct usercmd_s;
struct playerState_s;


/***********************************************************************
 > CMovementRecorder is a class that handles both recording, segmenting, and playbacks
 > This class can hold a playback queue which means that it will play all inserted playbacks in order
 > This class expects that the update function is called from CL_FinishMove
***********************************************************************/
class CMovementRecorder
{
	NONCOPYABLE(CMovementRecorder);
	friend class CStaticMovementRecorder;
	friend class CRMovementRecorder;
	friend class CGuiMovementRecorder;
	friend class CMovementRecorderIO;

public:
	CMovementRecorder();
	virtual ~CMovementRecorder();
	void Update(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd);

	// Recording control
	void StartRecording(bool start_from_movement = false);
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

	CPlayback* GetActivePlayback();
	CPlayback* GetActivePlayback() const;

protected:

	//Segmenting
	std::unique_ptr<CPlaybackSegmenter> Segmenter;
	
	//Recorder
	std::unique_ptr<std::vector<playback_cmd>> PendingRecording;
	std::unique_ptr<CRecorder> Recorder;
	
	//Playback
	std::queue<std::unique_ptr<CPlayback>> PlaybackQueue;
	std::map<std::string, std::unique_ptr<CPlayback>> LevelPlaybacks;
	CPlayback* PlaybackActive = {};

	//Lineup
	std::unique_ptr<CLineupPlayback> Lineup;

private:
	void UpdateLineup(usercmd_s* cmd, usercmd_s* oldcmd);
	void UpdatePlaybackQueue(usercmd_s* cmd, usercmd_s* oldcmd);

};

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


class CGuiMovementRecorder
{
	NONCOPYABLE(CGuiMovementRecorder);

public:

	CGuiMovementRecorder(CMovementRecorder& recorder);
	~CGuiMovementRecorder();

	void RenderLevelRecordings();

private:

	CMovementRecorder& m_oRefMovementRecorder;
	size_t m_uSelectedIndex = {};
	std::unique_ptr<CPlaybackGui> m_pItem;
};

class CMovementRecorderIO
{
	NONCOPYABLE(CMovementRecorderIO);

public:
	CMovementRecorderIO(CMovementRecorder& recorder)
		: m_oRefMovementRecorder(recorder) {}

	[[maybe_unused]] bool SaveToDisk(const std::string& name, const std::vector<playback_cmd>& cmds);
	
	//pushes the file to LevelPlaybacks
	[[maybe_unused]] bool LoadFromDisk(const std::string& name);


	[[maybe_unused]] bool RefreshAllLevelPlaybacks();

private:
	CMovementRecorder& m_oRefMovementRecorder;

};

class CStaticMovementRecorder
{
public:
	static std::unique_ptr<CMovementRecorder> Instance;

	//for other modules that need to use the movement recorder
	static void PushPlayback(std::vector<playback_cmd>&& cmds, const PlaybackInitializer& init);
	static void PushPlayback(const std::vector<playback_cmd>& cmds, const PlaybackInitializer& init);
	static bool GetActivePlayback() noexcept;

	//console commands
	static void ToggleRecording();
	static void SelectPlayback();
	static void OnDisconnect();
	static void Save();
	static void TeleportTo();
	static void Clear();

	//automatically load all recordings for the level
	static void Update();


private:

	static bool m_bPlaybacksLoaded;

};
