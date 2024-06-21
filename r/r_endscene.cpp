#include "r/r_drawactive.hpp"
#include "movement_recorder/mr_main.hpp"

#if(DEBUG_SUPPORT)
#include "utils/hook.hpp"
#include "r/gui/r_main_gui.hpp"
#include "cod4x/cod4x.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "main.hpp"
long __stdcall R_EndScene(IDirect3DDevice9* device)
{
	if (COD4X::attempted_screenshot())
		return hooktable::find<long, IDirect3DDevice9*>(HOOK_PREFIX(__func__))->call(device);

	auto MainGui = CStaticMainGui::Owner.get();

	if (MainGui && MainGui->OnFrameBegin()) {
		MainGui->Render();

		MainGui->OnFrameEnd();
	}

	return hooktable::find<long, IDirect3DDevice9*>(HOOK_PREFIX(__func__))->call(device);
}
void R_RecoverLostDevice()
{
	hooktable::find<void>(HOOK_PREFIX(__func__))->call();

	if (ImGui::GetCurrentContext()) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
	}

	return;

}
void CL_ShutdownRenderer()
{

	if (ImGui::GetCurrentContext()) {
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}


	return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
}
int __cdecl R_CreateDeviceInternal(HWND__* hwnd, int BehaviorFlags, _D3DPRESENT_PARAMETERS_* d3dpp)
{
	auto r = hooktable::find<int, HWND__*, int, _D3DPRESENT_PARAMETERS_*>(HOOK_PREFIX(__func__))->call(hwnd, BehaviorFlags, d3dpp);

	if (r >= 0)
		CStaticMainGui::Owner->Init(dx->device, FindWindow(NULL, COD4X::get() ? "Call of Duty 4 X" : "Call of Duty 4"));

	return r;
}
#else
void R_EndScene([[maybe_unused]] IDirect3DDevice9* device)
{
	

}
#endif