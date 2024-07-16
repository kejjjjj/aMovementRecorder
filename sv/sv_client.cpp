#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"

#include "sv_client.hpp"
#include "utils/engine.hpp"
#include "utils/typedefs.hpp"

#include <iostream>
#include <cg/cg_angles.hpp>

CStaticServerToClient::Data CStaticServerToClient::whatWasSent;
CStaticServerToClient::Data CStaticServerToClient::whatWasUsed;
CStaticServerToClient::Data CStaticServerToClient::whatWasReceived;
float CStaticServerToClient::delta_angles;

__declspec(naked) void SV_SendMessageToClientASM()
{
	static constexpr auto a = 0x535BB5;
	__asm
	{
		push ebx;
		push ebp;
		push esi;
		mov esi, eax;

		push edi;
		push eax;
		call SV_SendMessageToClient;
		add esp, 8;

		jmp a;
	}
}
void SV_SendMessageToClient([[maybe_unused]]msg_t* msg, client_t* client)
{
	auto& Instance = CStaticServerToClient::whatWasReceived;

	Instance.cmd = client->lastUsercmd;
	Instance.snap = &client->frames[client->header.netchan.outgoingSequence & 0x1F];
	Instance.ps = &Instance.snap->ps;

}
__declspec(naked) void ClientThink_realASM()
{
	static constexpr auto u = 0x04A850E;
	__asm
	{
		push ebp;
		mov ebp, esp;
		and esp, 0FFFFFFF8h;
		sub esp, 204h;
		push ebx;
		push esi;

		mov esi, [ebp + 8h];
		push esi;
		push eax;
		call ClientThink_real;
		add esp, 8;
		jmp u;
	}
}
void ClientThink_real([[maybe_unused]]usercmd_s* cmd, [[maybe_unused]] gentity_s* gentity)
{

	auto& Instance = CStaticServerToClient::whatWasUsed;

	Instance.cmd = *cmd;
	Instance.snap = nullptr;
	Instance.ps = &gentity->client->ps;

}

void Pmove(pmove_t* pm)
{
	int msec;
	int finalTime;
	playerState_s* ps; 
	ps = pm->ps;
	finalTime = pm->cmd.serverTime;
	if (finalTime >= ps->commandTime)
	{
		if (finalTime > ps->commandTime + 1000)
			ps->commandTime = finalTime - 1000;
		pm->numtouch = 0;
		while (ps->commandTime != finalTime)
		{
			msec = finalTime - ps->commandTime;
			if (msec > 66)
				msec = 66;
			pm->cmd.serverTime = msec + ps->commandTime;

			//a stupid temp fix for broken playbacks
			//if (AngleNormalize180(ps->delta_angles[YAW]) < 0) {

			//	ps->delta_angles[YAW] = -AngleNormalize180(ps->delta_angles[YAW]);
			//	pm->cmd.angles[YAW] = ANGLE2SHORT(AngleDelta(ps->viewangles[YAW], ps->delta_angles[YAW]));

			//}
			Engine::call<void>(0x04143A0, pm);
			memcpy(&pm->oldcmd, &pm->cmd, sizeof(pm->oldcmd));

		}
	}
}

__declspec(naked) void SetClientViewAngleASM()
{
	__asm
	{
		sub esp, 18h;
		push ebx;
		
		push ebp;
		mov ebp, [esp + 20h+4];
		push ebp;
		push eax;
		call SetClientViewAngle;
		add esp, 8;
		pop ebp;
		pop ebx;
		add esp, 18h;
		retn;
	}
}
void SetClientViewAngle(float* angles, gentity_s* client)
{
	(fvec3&)client->r.currentAngles = fvec3(angles);
	(fvec3&)client->client->ps.viewangles = fvec3(angles);

}
