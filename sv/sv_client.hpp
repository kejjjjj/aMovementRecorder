#pragma once

#include "cg/cg_local.hpp"

void SV_SendMessageToClientASM();
void SV_SendMessageToClient(msg_t* msg, client_t* client);

void ClientThink_realASM();
void ClientThink_real(usercmd_s* cmd, gentity_s* gentity);

void Pmove(pmove_t* pm);

void SetClientViewAngleASM();
void SetClientViewAngle(float* angles, gentity_s* client);

class CStaticServerToClient
{
public:
	struct Data
	{
		usercmd_s cmd{};
		clientSnapshot_t* snap{};
		playerState_s* ps{};
	};

	static Data whatWasSent;
	static Data whatWasUsed;
	static Data whatWasReceived;
	static float delta_angles;
};
