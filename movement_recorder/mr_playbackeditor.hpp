#include <cstdint>
#include <vector>

class CPlayerStatePlayback;
struct GfxViewParms;
struct GfxPointVertex;

class CPlaybackEditor
{
	friend class CPlaybackEditorRenderer;

public:
	CPlaybackEditor(const CPlayerStatePlayback& pb);
	~CPlaybackEditor();

	void Advance(std::int32_t amount);

	[[nodiscard]] const playerState_s* GetCurrentState() const;
	[[nodiscard]] size_t GetCurrentCmdOffset() const noexcept;
private:

	std::uint32_t m_iCurrentPlayerStateOffsetFromBeginning = 0u;
	const std::uint32_t m_uPlayerStateToCmdRatio = 0u;
public:
	const CPlayerStatePlayback& m_oRefPlayback;

};

class CPlaybackEditorRenderer
{
public:
	CPlaybackEditorRenderer(CPlaybackEditor& editor);
	~CPlaybackEditorRenderer();

	void RB_Render(GfxViewParms* vParms) const;

private:

	void UpdateVertices();

	//std::vector<GfxPointVertex> m_oPathVertices;
	//std::int32_t m_iVertCount;
	const CPlaybackEditor& m_oRefPlaybackEditor;
};

