#pragma once

enum connstate_t : int;
struct usercmd_s;

connstate_t CL_ConnectionState() noexcept;
usercmd_s* CL_GetUserCmd(const int cmdNumber);

void CL_FixedTime(usercmd_s* cmd, usercmd_s* oldcmd);
void CL_FinishMove(usercmd_s*);





