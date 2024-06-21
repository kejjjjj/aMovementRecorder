#pragma once


#include <vector>
#include "fs/fs_io.hpp"
#include "global_macros.hpp"

struct playback_cmd;
struct playerState_s;
struct usercmd_s;

template<typename T>
struct vec3;
using fvec3 = vec3<float>;


class CPlayback
{
public:
	CPlayback(std::vector<playback_cmd>&& data, int g_speed = 190, bool jump_slowdownEnable = false);
	explicit CPlayback(const std::vector<playback_cmd>& _data, int g_speed = 190, bool jump_slowdownEnable = false);
	~CPlayback();
	void DoPlayback(usercmd_s* cmd, usercmd_s* oldcmd);
	bool IsPlayback() const noexcept;
	void StopPlayback();

	playback_cmd* GetIterator();
	std::size_t GetIteratorIndex() const;

	bool IsCompatibleWithState(const playerState_s* ps) const noexcept;

	fvec3 GetOrigin() const noexcept;
	fvec3 GetAngles() const noexcept;

	operator std::vector<playback_cmd>();

	friend class CPlaybackIOWriter;
	friend class CPlaybackIOReader;
	friend class CPlaybackGui;

	std::vector<playback_cmd> cmds;

private:
	void TryFixingTime(usercmd_s* cmd, usercmd_s* oldcmd);

	enum slowdown_t : std::int8_t
	{
		disabled,
		enabled,
		both
	};

#pragma pack(push, 1)
	struct Header
	{
		int m_iSpeed = 190;
		slowdown_t m_bJumpSlowdownEnable = disabled;
	}m_objHeader;
#pragma pack(pop)

	std::size_t m_iCmd = 0u;
	//std::span<playback_cmd>::iterator icmd;

	int m_iSpeed = 190;
	bool m_jumpSlowdownEnable = false;

};

class CPlaybackGui
{
	NONCOPYABLE(CPlaybackGui);
public:
	CPlaybackGui(CPlayback& owner, const std::string name);
	~CPlaybackGui();
	void Render();

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
