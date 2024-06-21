//#include "cg_hooks.hpp"
#include "cg_init.hpp"

#include "cl/cl_move.hpp"
#include "utils/hook.hpp"

#include "r/r_drawactive.hpp"
#include "wnd/wnd.hpp"

#include "cod4x/cod4x.hpp"

#include "bg/bg_pmove.hpp"
#include "cl/cl_main.hpp"

#include "scr/scr_menuresponse.hpp"
#include <shared/sv_shared.hpp>

#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include <thread>


static void CG_CreateHooks();
void CG_CreatePermaHooks()
{
#if(DEBUG_SUPPORT)
	hooktable::initialize();
#else
	hooktable::initialize(CMain::Shared::GetFunctionOrExit("GetHookTable")->As<std::vector<phookbase>*>()->Call());
#endif

	CG_CreateHooks();
}

void CG_CreateHooks()
{
	//std::cout << HOOK_PREFIX(": (Creating hooks)\n");
	hooktable::preserver<void, usercmd_s*>(HOOK_PREFIX("CL_FinishMove"), 0x463A60u, CL_FinishMove);
	hooktable::preserver<void>(HOOK_PREFIX("CG_DrawActive"), COD4X::get() ? COD4X::get() + 0x5464 : 0x42F7F0, CG_DrawActive);
	hooktable::preserver<void, int>(HOOK_PREFIX("CL_Disconnect"), 0x4696B0, CL_Disconnect);
	hooktable::preserver<void>(HOOK_PREFIX("PM_Weapon_FinishWeaponChangeASM"), 0x41A5F7, PM_Weapon_FinishWeaponChangeASM);

	hooktable::preserver<void>(HOOK_PREFIX("Script_ScriptMenuResponse"), 0x54DE59, Script_ScriptMenuResponse);
	hooktable::preserver<void>(HOOK_PREFIX("Script_OpenScriptMenu"), 0x46D4CF, Script_OpenScriptMenu);

	//hooktable::preserver<char, GfxViewParms*>(HOOK_PREFIX("RB_DrawDebug"), 0x658860, RB_DrawDebug);

	//hooktable::overwriter<void>(HOOK_PREFIX("PM_Weapon_FinishWeaponChange"), 0x41817A, PM_Weapon_FinishWeaponChangeASM);

	if(COD4X::get())
		BG_WeaponNames = reinterpret_cast<WeaponDef**>(COD4X::get() + 0x443DDE0);

#if(DEBUG_SUPPORT)
	hooktable::preserver<void>(HOOK_PREFIX("R_RecoverLostDevice"), 0x5F5360, R_RecoverLostDevice);
	hooktable::preserver<void>(HOOK_PREFIX("CL_ShutdownRenderer"), 0x46CA40, CL_ShutdownRenderer);
	hooktable::preserver<LRESULT, HWND, UINT, WPARAM, LPARAM>(HOOK_PREFIX("WndProc"), COD4X::get() ? COD4X::get() + 0x801D6 : 0x57BB20, WndProc);

	using namespace std::chrono_literals;

	while (!dx || !dx->device) {
		std::this_thread::sleep_for(100ms);
	}
	if (dx && dx->device)
		hooktable::preserver<long, IDirect3DDevice9*>(HOOK_PREFIX("R_EndScene"),
			reinterpret_cast<size_t>((*reinterpret_cast<PVOID**>(dx->device))[42]), R_EndScene);

#endif

}
void CG_ReleaseHooks()
{

}