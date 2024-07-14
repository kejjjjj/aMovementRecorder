#pragma once
#include "cg/cg_local.hpp"
#include "utils/typedefs.hpp"
#include <vector>
#include <mutex>

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

	//this doesn't belong here but this is not supposed to be clean!
	void RenderPath() const;
	
private:
	mutable std::mutex SimulationMutex;
	CPlaybackSimulationAnalysis analysis{};

	//might point to trash so beware of object lifetime!
	const CPlayback& m_oRefPlayback;
};


