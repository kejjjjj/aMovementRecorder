#pragma once


#include <vector>
#include "fs/fs_io.hpp"
#include "global_macros.hpp"

struct playback_cmd;
struct playerState_s;
struct usercmd_s;
struct GfxViewParms;

template<typename T>
struct vec3;
using fvec3 = vec3<float>;

enum slowdown_t : std::int8_t
{
	disabled,
	enabled,
	both
};

struct CPlaybackSettings
{
	int m_iGSpeed = 190;
	slowdown_t m_eJumpSlowdownEnable = disabled;
	bool m_bIgnorePitch = false;
	bool m_bIgnoreWeapon = false;
	bool m_bIgnoreWASD = false;
	bool m_bSetComMaxfps = true;
	bool m_bNoLag = false; 
	bool m_bRenderExpectationVsReality = false;
};

class CPlayback
{
	friend class CDebugPlayback;
public:
	CPlayback(std::vector<playback_cmd>&& data, const CPlaybackSettings& init);
	explicit CPlayback(const std::vector<playback_cmd>& _data, const CPlaybackSettings& init);
	~CPlayback();

	void DoPlayback(usercmd_s* cmd, const usercmd_s* oldcmd);
	bool IsPlayback() const noexcept;
	void StopPlayback();

	playback_cmd* GetIterator();
	std::size_t GetIteratorIndex() const;

	bool IsCompatibleWithState(const playerState_s* ps) const noexcept;

	fvec3 GetOrigin() const noexcept;
	fvec3 GetAngles() const noexcept;

	__forceinline constexpr void IgnorePitch(bool ignore = true) const noexcept { m_oSettings.m_bIgnorePitch = ignore; }
	__forceinline constexpr void IgnoreWeapon(bool ignore = true) const noexcept { m_oSettings.m_bIgnoreWeapon = ignore; }
	__forceinline constexpr void IgnoreWASD(bool ignore = true) const noexcept { m_oSettings.m_bIgnoreWASD = ignore; }
	__forceinline constexpr bool bIgnoreWASD() const noexcept { return m_oSettings.m_bIgnoreWASD; }

	operator std::vector<playback_cmd>();

	friend class CPlaybackIOWriter;
	friend class CPlaybackIOReader;
	friend class CPlaybackGui;

	friend void UpdatePlaybackQueue(usercmd_s* cmd, usercmd_s* oldcmd);

	std::vector<playback_cmd> cmds;

private:
	std::int32_t GetCurrentTimeFromIndex(const std::int32_t cmdIndex) const;
	void SyncClientFPS(const std::int32_t index) const noexcept;
	void EditUserCmd(usercmd_s* cmd, const std::int32_t index) const;

	void EraseDeadFrames();
	void TryFixingTime(usercmd_s* cmd, const usercmd_s* oldcmd);

	std::size_t m_iCmd = 0u;
	std::int32_t m_iFirstServerTime = 0;

	mutable CPlaybackSettings m_oSettings;

	#pragma pack(push, 1)
		struct Header
		{
			int m_iVersion = 1;
			int m_iSpeed = 190;
			slowdown_t m_bJumpSlowdownEnable = disabled;
		}m_objHeader;
	#pragma pack(pop)

};

class CDebugPlayback
{
public:

	CDebugPlayback(const std::vector<playback_cmd>& cmds);
	void RB_Render(GfxViewParms* viewParms) const;
	void CG_Render() const;


	std::vector<playback_cmd> m_oExpectedPlayback;
	std::vector<playback_cmd> m_oRealityPlayback;

private:
};

class CPlaybackGui
{
	NONCOPYABLE(CPlaybackGui);
public:
	CPlaybackGui(CPlayback& owner, const std::string name);
	~CPlaybackGui();
	bool Render();

private:
	size_t m_uNumChanges = {};
	CPlayback& m_refOwner;
	std::string m_sName;
	int m_iCurrentSlowdown = {};
	size_t m_uChanges = {};
};


class CPlaybackIOWriter : public AgentIOWriter
{
public:
	CPlaybackIOWriter(const CPlayback* target, const std::string& name) 
		: AgentIOWriter("Playbacks\\" + name, true), m_pTarget(target), m_sName(name) {}

	bool Write() const;

protected:
	const CPlayback* m_pTarget = 0;
	std::string m_sName;
};

class CPlaybackIOReader : public AgentIOReader
{
public:
	CPlaybackIOReader(const std::string& name)
		: AgentIOReader("Playbacks\\" + name, true), m_sName(name) {}

	bool Read();
	std::unique_ptr<CPlayback> m_objResult;

protected:
	std::string m_sName;
};
