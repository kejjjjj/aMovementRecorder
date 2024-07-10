#include "r_drawactive.hpp"
#include "utils/hook.hpp"
#include "movement_recorder/mr_main.hpp"
#include <r/gui/r_main_gui.hpp>
#include <r/r_utils.hpp>
#include <r/r_drawtools.hpp>
#include "cg/cg_local.hpp"
#include <cg/cg_offsets.hpp>
#include <cl/cl_utils.hpp>

void CG_DrawActive()
{
	if (R_NoRender())
#if(DEBUG_SUPPORT)
		return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#else
		return;
#endif

	CStaticMovementRecorder::Update();

	auto renderer = CRMovementRecorder(*CStaticMovementRecorder::Instance);
	renderer.CG_Render();


#if(DEBUG_SUPPORT)
	return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#endif

}