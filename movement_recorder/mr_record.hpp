#pragma once

#include "utils/typedefs.hpp"
#include <vector>

struct playerState_s;
struct usercmd_s;
struct playback_cmd;

//#pragma pack(push, 1)
//struct playback_cmd
//{
//	std::int32_t serverTime = 0;
//	std::int32_t oldTime = 0;
//	std::int32_t buttons = 0;
//	std::int8_t forwardmove = 0;
//	std::int8_t rightmove = 0;
//	std::int8_t weapon = 0;
//	std::int8_t offhand = 0;
//	//std::int32_t FPS = 0;
//	fvec3 deltas;
//	fvec3 viewangles;
//	fvec3 origin;
//	fvec3 velocity;
//	static playback_cmd FromPlayerState(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd);
//};
//#pragma pack(pop)

class CRecorder
{
public:
	CRecorder() = default;
	explicit CRecorder(bool start_from_movement, int start_timer = 0) : m_bStartFromMove(start_from_movement), m_iStartTimer(start_timer){}
	~CRecorder();
	void Record(playerState_s* ps, usercmd_s* cmd, usercmd_s* oldcmd) noexcept;

	inline bool IsWaiting() const noexcept { return m_iStartTimer > 0; }

	std::vector<playback_cmd>&& StopRecording() noexcept;

private:
	std::vector<playback_cmd> data;
	bool m_bStartFromMove = {};

	//how many server frames will it take before the recording starts
	int m_iStartTimer = {};
};