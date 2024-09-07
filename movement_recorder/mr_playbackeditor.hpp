#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

class CPlayerStatePlayback;
class CPlayback;
struct GfxViewParms;
struct GfxPointVertex;

class CPlaybackEditor
{
	friend class CPlaybackEditorRenderer;

public:
	CPlaybackEditor(CPlayerStatePlayback& pb);
	~CPlaybackEditor();

	void Advance(std::int32_t amount);

	[[nodiscard]] const playerState_s* GetCurrentState() const;
	[[nodiscard]] size_t GetCurrentCmdOffset() const noexcept;
	[[nodiscard]] size_t GetCurrentPlayerStateOffset() const noexcept;

	void Merge(const CPlayerStatePlayback& playback);

private:

	std::atomic<std::uint32_t> m_iCurrentPlayerStateOffsetFromBeginning = 0u;
	const std::uint32_t m_uPlayerStateToCmdRatio = 0u;

protected:
	mutable std::mutex mtx;

public:
	CPlayerStatePlayback& m_oRefPlayback;

	//only contains the cmds from where the playback actually starts from
	std::unique_ptr<CPlayerStatePlayback> m_oSegmentedPlayback;
};

class CPlaybackEditorRenderer
{
public:
	CPlaybackEditorRenderer(CPlaybackEditor& editor);
	~CPlaybackEditorRenderer();

	void RB_Render(GfxViewParms* vParms) const;
	void CG_Render() const;
private:

	void UpdateVertices();
	std::vector<playerState_s> GetPlayerStates() const noexcept;
	//std::vector<GfxPointVertex> m_oPathVertices;
	//std::int32_t m_iVertCount;
	const CPlaybackEditor& m_oRefPlaybackEditor;
};

