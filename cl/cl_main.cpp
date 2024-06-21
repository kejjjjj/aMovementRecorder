#include "cl_main.hpp"
#include "utils/hook.hpp"
#include "movement_recorder/mr_main.hpp"
#include <cg/cg_local.hpp>
#include <cg/cg_offsets.hpp>


void CL_Disconnect(int clientNum)
{
	if (clientUI->connectionState != CA_DISCONNECTED) { //gets called in the loading screen in 1.7
		CStaticMovementRecorder::OnDisconnect();
	}

	hooktable::find<void, int>(HOOK_PREFIX(__func__))->call(clientNum);

}
