#pragma once

#include "utils/typedefs.hpp"
#include <vector>

struct playerState_s;
struct usercmd_s;
struct playback_cmd;

class CRecorder
{
public:
	CRecorder() = default;
	explicit CRecorder(bool start_from_movement, int start_timer = 0) : m_bStartFromMove(start_from_movement), m_iStartTimer(start_timer){}
	virtual ~CRecorder();
	virtual void Record(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd) noexcept;

	[[nodiscard]] inline bool IsWaiting() const noexcept { return m_iStartTimer > 0; }

	[[nodiscard]] std::vector<playback_cmd>&& StopRecording() noexcept;
	[[nodiscard]] playback_cmd* GetCurrentCmd();
	
	[[nodiscard]] virtual constexpr bool AmIDerived() const noexcept { return false; }

protected:
	std::vector<playback_cmd> data;
	[[maybe_unused]] bool m_bStartFromMove = {};

	//how many server frames will it take before the recording starts
	int m_iStartTimer = {};
};

class CPlayerStateRecorder : public CRecorder
{
public:
	CPlayerStateRecorder() : CRecorder() {};
	explicit CPlayerStateRecorder(bool start_from_movement, int start_timer = 0) : CRecorder(start_from_movement, start_timer) {}
	~CPlayerStateRecorder();

	void Record(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd) noexcept override;
	[[nodiscard]] constexpr bool AmIDerived() const noexcept override { return true; }

	std::vector<playerState_s> playerState;

protected:
};
