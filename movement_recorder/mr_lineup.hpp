#pragma once

#include "global_macros.hpp"
#include <utils/typedefs.hpp>
#include <vector>

struct usercmd_s;
struct playerState_s;
struct playback_cmd;
class CPlayback;
struct clientInput_t;

class CLineup
{
public:
	NONCOPYABLE(CLineup);

	CLineup(const playerState_s* ps, const fvec3& destination, const fvec3& destination_angles, float lineup_distance);

	enum State : std::int8_t
	{
		gave_up,
		active,
		finished
	};

	bool Update(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd) noexcept;

	constexpr bool Finished() const noexcept { return m_eState == finished; }
	constexpr bool GaveUp() const noexcept { return m_eState == gave_up; }


private:

	void UpdateViewangles(const playerState_s* ps, usercmd_s* cmd);
	void UpdateOrigin(const playerState_s* ps);
	
	void MoveCloser(const playerState_s* ps, usercmd_s* cmd, const usercmd_s* oldcmd);

	void CreatePlayback(const playerState_s* ps, usercmd_s* cmd) const;
	bool CanPathfind(const playerState_s* ps) const noexcept;

	clientInput_t GetNextDirection(const playerState_s* ps, const usercmd_s* cmd, const usercmd_s* oldcmd) const;

	fvec3 m_vecDestination;
	fvec3 m_vecDestinationAngles;
	fvec3 m_vecTargetAngles;
	fvec3 m_vecOldOrigin;
	ivec3 m_ivecTargetCmdAngles;
	
	float m_fYawToTarget = {};
	float m_fLineupAccuracy = {};
	float m_fTotalDistance = {};
	float m_fDistanceMoved = {};

	int m_iCmdButtons = {};

	size_t m_uAccuracyTestFrames = {};

	State m_eState = active;
	State m_eAngleState = active;

};

class CLineupPlayback : public CLineup
{
public:
	NONCOPYABLE(CLineupPlayback);

	CLineupPlayback(const playerState_s* ps, const CPlayback& playback, const fvec3& destination, const fvec3& destination_angles);
	CPlayback& GetPlayback();

private:
	std::unique_ptr<CPlayback> m_pPlayback;
};