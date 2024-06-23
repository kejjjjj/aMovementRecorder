#pragma once

#include "global_macros.hpp"

#include <queue>
#include <com/com_channel.hpp>
#include <unordered_map>
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

class CPlaybackSegmenter;
class CLineupPlayback;
class CLineup;
class CRecorder;
class CPlayback;
class CPlaybackGui;
struct playback_cmd;

struct usercmd_s;
struct playerState_s;

/***********************************************************************
 > CMovementRecorder is a class that handles both recording and playbacks
 > This class can hold a playback queue which means that it will play all inserted playbacks in order
 > This class expects that the update function is called from CL_FinishMove
***********************************************************************/
class CMovementRecorder
{
	NONCOPYABLE(CMovementRecorder);

public:
	CMovementRecorder() = default;
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

	CPlayback* GetActive();
	CPlayback* GetActive() const;

	bool DoingPlayback() const noexcept;

protected:

	//Segmenting
	std::unique_ptr<CPlaybackSegmenter> Segmenter;
	
	//Recorder
	std::unique_ptr<std::vector<playback_cmd>> PendingRecording;
	std::unique_ptr<CRecorder> Recorder;
	
	//Playback
	std::queue<std::unique_ptr<CPlayback>> PlaybackQueue;
	std::unordered_map<std::string, std::unique_ptr<CPlayback>> LevelPlaybacks;
	CPlayback* PlaybackActive = {};

	//Lineup
	std::unique_ptr<CLineupPlayback> Lineup;

private:
	void UpdateLineup(usercmd_s* cmd, usercmd_s* oldcmd);
	void UpdatePlaybackQueue(usercmd_s* cmd, usercmd_s* oldcmd);
};

/***********************************************************************
 > CRMovementRecorder extends CMovementRecorder by adding rendering methods
***********************************************************************/
class CRMovementRecorder : public CMovementRecorder
{
	NONCOPYABLE(CRMovementRecorder);

public:

	CRMovementRecorder() = default;
	~CRMovementRecorder();

	void CG_Render();

private:
	void CG_RenderOrigins() const;
	void CG_RenderPrecision() const;
	void CG_RenderStatus() const;
};
/***********************************************************************
 > CGuiMovementRecorder extends CRMovementRecorder by adding gui rendering methods
***********************************************************************/
class CGuiMovementRecorder : public CRMovementRecorder
{
	NONCOPYABLE(CGuiMovementRecorder);

public:
	friend class CStaticMovementRecorder;

	CGuiMovementRecorder() = default;
	~CGuiMovementRecorder();

	void Gui_RenderLocals();

private:
	size_t m_uSelectedIndex = {};
	std::unique_ptr<CPlaybackGui> m_pItem;
};

class CStaticMovementRecorder
{
public:
	static std::unique_ptr<CGuiMovementRecorder> Instance;

	static void ToggleRecording();
	static void SetPlayback();

	static void PushPlayback(std::vector<playback_cmd>&& cmds, int g_speed = 190, bool slowdownenable = false);
	static void PushPlaybackCopy(const std::vector<playback_cmd>& cmds, int g_speed = 190, bool slowdownenable = false);

	static void OnDisconnect();
	static void Save();
	static void TeleportTo();
	static void Update();
	static void Clear();

	static bool DoingPlayback();

	static CGuiMovementRecorder* Get();

private:
	static void Load(const std::string& name);

	static bool m_bPlaybacksLoaded;

};
