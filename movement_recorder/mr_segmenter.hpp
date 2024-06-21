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
	success_t Update(usercmd_s* cmd, usercmd_s* oldcmd, playerState_s* ps);
	bool ResultExists() const noexcept;
	std::optional<CPlayback> GetResult() const;
	CPlayback* GetPlayback();

private:

	std::unique_ptr<CPlaybackSegmenterImpl> m_oImpl;
};
