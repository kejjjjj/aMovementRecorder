#include "r_drawactive.hpp"
#include "utils/hook.hpp"
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>
#include "r/r_drawtools.hpp"

#include "movement_recorder/mr_main.hpp"
#include <cg/cg_client.hpp>
#include <main.hpp>
#include <r/gui/r_main_gui.hpp>

#include "cod4x/cod4x.hpp"

void CG_DrawActive()
{
	constexpr float col[4] = { 1,1,1,1 };
	constexpr float glowCol[4] = { 1,0,0,1 };
	CStaticMovementRecorder::Instance->CG_Render();

	char buff[64];
	sprintf_s(buff, "%i\n", CG_GetSpeed(&cgs->predictedPlayerState));

	float x = R_GetTextCentered(buff, "fonts/normalfont", 320.f, 0.5f);
	R_AddCmdDrawTextWithEffects(buff, "fonts/normalfont", x, 260.f, 0.5f, 0.5f, 0.f, col, 3, glowCol, nullptr, nullptr, 0, 0, 0, 0);

	return hooktable::find<void>(HOOK_PREFIX(__func__))->call();

}
#if(DEBUG_SUPPORT)

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

#endif