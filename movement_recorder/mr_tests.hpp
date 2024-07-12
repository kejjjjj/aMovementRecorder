#pragma once
#include "cg/cg_local.hpp"
#include "utils/typedefs.hpp"

class CPlayback;

struct CPlaybackSimulationAnalysis
{
	pmove_t pm{};
	playerState_s ps{}; //pm points to this

	struct FrameData
	{
		fvec3 origin;
		fvec3 angles;
	};

	std::vector<FrameData> frames;
};

//tests if a playback can be performed 1:1 compared to the original recording
//if not, then it attempts to fix it
class CPlaybackSimulation
{
public:
	CPlaybackSimulation() = delete;
	explicit CPlaybackSimulation(const CPlayback& pb);
	~CPlaybackSimulation();

	bool Simulate(const fvec3& origin, const fvec3& angles);
	const auto* GetAnalysis() const noexcept { return &analysis; }
private:
	CPlaybackSimulationAnalysis analysis{};
	const CPlayback& m_oRefPlayback;
};


