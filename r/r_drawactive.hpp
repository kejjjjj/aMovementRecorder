#pragma once

void CG_DrawActive();

struct IDirect3DDevice9;

#if(DEBUG_SUPPORT)

struct _D3DPRESENT_PARAMETERS_;
struct HWND__;
void R_RecoverLostDevice();
void CL_ShutdownRenderer();
int __cdecl R_CreateDeviceInternal(HWND__* hwnd, int BehaviorFlags, _D3DPRESENT_PARAMETERS_* d3dpp);
long __stdcall R_EndScene(IDirect3DDevice9* device);

#else

void R_EndScene(IDirect3DDevice9* device);

#endif
