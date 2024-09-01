#pragma once

#include <memory>
#include <vector>
#include <optional>
class CPlayback;
class CRecorder;
struct usercmd_s;
struct playerState_s;
struct playback_cmd;

using success_t = bool;

class CPlaybackSegmenterImpl;
class CPlaybackSegmenter
{
public:
	CPlaybackSegmenter(const CPlayback& source);
	~CPlaybackSegmenter();

	//\return false if everything is finished
	success_t Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd);
	void StartSegmenting() noexcept;
	bool ResultExists() const noexcept;
	std::optional<CPlayback> GetResult() const;
	CPlayback* GetPlayback();

private:

	std::unique_ptr<CPlaybackSegmenterImpl> m_oImpl;
};
