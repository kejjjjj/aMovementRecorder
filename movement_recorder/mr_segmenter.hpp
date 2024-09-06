#pragma once

#include <memory>
#include <vector>

class CPlayback;
class CRecorder;
struct usercmd_s;
struct playerState_s;
struct playback_cmd;

using success_t = bool;

class CPlaybackSegmenter
{
public:
	CPlaybackSegmenter(const CPlayback& source);
	~CPlaybackSegmenter();

	//return false if everything is finished
	success_t Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd);
	void StartSegmenting() noexcept;
	void StopSegmenting() noexcept;
	bool ResultExists() const noexcept;
	std::unique_ptr<CPlayback> GetResult() const;
	CPlayback* GetPlayback();

private:

	std::unique_ptr<CRecorder> m_oRecorder;
	std::unique_ptr<CPlayback> m_pOriginal;

	std::size_t m_iNewSegment = 0u;
	bool m_bExternalPartyWantsToStartSegmenting = false;
};
