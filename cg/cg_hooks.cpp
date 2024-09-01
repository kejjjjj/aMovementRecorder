//#include "cg_hooks.hpp"
#include "cg_init.hpp"

#include "cl/cl_move.hpp"
#include "utils/hook.hpp"

#include "bg/bg_pmove.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "cl/cl_main.hpp"
#include "cl/cl_utils.hpp"
#include "cod4x/cod4x.hpp"
#include "r/backend/rb_endscene.hpp"
#include "r/r_drawactive.hpp"
#include "scr/scr_menuresponse.hpp"
#include "shared/sv_shared.hpp"
#include "sv/sv_client.hpp"
#include "wnd/wnd.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

#if(DEBUG_SUPPORT)
#include "r/gui/r_gui.hpp"
#include "cg/cg_memory.hpp"
#endif

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
	hooktable::preserver<void, int>(HOOK_PREFIX("CL_Disconnect"), 0x4696B0, CL_Disconnect);

	hooktable::preserver<void>(HOOK_PREFIX("Script_ScriptMenuResponse"), 0x54DE59, Script_ScriptMenuResponse);
	hooktable::preserver<void>(HOOK_PREFIX("Script_OpenScriptMenu"), 0x46D4CF, Script_OpenScriptMenu);

	hooktable::preserver<void>(HOOK_PREFIX("SV_SendMessageToClientASM"), 0x535BB0, SV_SendMessageToClientASM);


#if(DEBUG_SUPPORT)
	CG_CreateEssentialHooks();

	hooktable::preserver<void, int>(HOOK_PREFIX("CL_CreateNewCommandsASM"), 0x463E00, CL_CreateNewCommandsASM);

	//hooktable::preserver<void, usercmd_s*>(HOOK_PREFIX("CL_FinishMove"), 0x463A60u, CL_FinishMove);
	hooktable::preserver<void>(HOOK_PREFIX("CG_DrawActive"), COD4X::get() ? COD4X::get() + 0x5464 : 0x42F7F0, CG_DrawActive);

	hooktable::preserver<void, GfxViewParms*>(HOOK_PREFIX("RB_DrawDebug"), 0x658860, RB_DrawDebug);

	//hooktable::preserver<void>(HOOK_PREFIX("ClientThink_realASM"), 0x4A8500, ClientThink_realASM);
	//hooktable::preserver<void>(HOOK_PREFIX("Pmove"), 0x414D10, Pmove);
	//hooktable::preserver<void>(HOOK_PREFIX("SetClientViewAngleASM"), 0x4AA550, SetClientViewAngleASM);

#endif

}