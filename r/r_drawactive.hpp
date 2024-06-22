#pragma once

void CG_DrawActive();

struct IDirect3DDevice9;

#if(DEBUG_SUPPORT)
long __stdcall R_EndScene(IDirect3DDevice9* device);
#else

void R_EndScene(IDirect3DDevice9* device);

#endif
