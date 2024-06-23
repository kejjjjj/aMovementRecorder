#include "r_drawactive.hpp"
#include "utils/hook.hpp"
#include "movement_recorder/mr_main.hpp"
#include <r/gui/r_main_gui.hpp>
#include <r/r_utils.hpp>
#include <r/r_drawtools.hpp>

void CG_DrawActive()
{
	if (R_NoRender())
#if(DEBUG_SUPPORT)
		return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#else
		return;
#endif
	//constexpr float col[4] = { 1,1,1,1 };
	//constexpr float glowCol[4] = { 1,0,0,1 };

	//char buff[64];
	//sprintf_s(buff, "this should not be visible (MR)");

	//float x = R_GetTextCentered(buff, "fonts/normalfont", 320.f, 0.5f);
	//R_AddCmdDrawTextWithEffects(buff, "fonts/normalfont", x, 260.f, 0.5f, 0.5f, 0.f, col, 3, glowCol, nullptr, nullptr, 0, 0, 0, 0);

	CStaticMovementRecorder::Instance->CG_Render();

#if(DEBUG_SUPPORT)
	return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#endif

}