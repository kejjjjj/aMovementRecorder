#include "mr_playback.hpp"

#include "bg/bg_pmove_simulation.hpp"

#include "cg/cg_local.hpp"

#include "r/backend/rb_endscene.hpp"
#include "r/r_drawtools.hpp"

#include "utils/typedefs.hpp"

#include <ranges>

GfxPointVertex verts[2075];
void CDebugPlayback::RB_Render([[maybe_unused]] GfxViewParms* viewParms) const
{

	//expectation
	const auto numOriginalPoints = std::min(300u, m_oExpectedPlayback.size());
	auto vert_count = 0;

	for (const auto i : std::views::iota(0u, numOriginalPoints -1u)) {
		vert_count = RB_AddDebugLine(verts, true, m_oExpectedPlayback[i].origin.As<vec_t*>(), m_oExpectedPlayback[i+1].origin.As<vec_t*>(),
			vec4_t{ 0,1,0,1 }, vert_count);

	}

	const auto numRealityPoints = std::min(300u, m_oRealityPlayback.size());

	if (numRealityPoints > 0) {
		for (const auto i : std::views::iota(0u, numRealityPoints - 1u)) {
			vert_count = RB_AddDebugLine(verts, true, m_oRealityPlayback[i].origin.As<vec_t*>(), m_oRealityPlayback[i + 1].origin.As<vec_t*>(),
				vec4_t{ 1,0,0,1 }, vert_count);

		}
	}

	RB_DrawLines3D(vert_count / 2, 3, verts, true);

}
void CDebugPlayback::CG_Render() const
{
	for (auto i = 0u; const auto & p : m_oExpectedPlayback) {
		if (const auto pos = WorldToScreen(p.origin)) {
			R_AddCmdDrawTextWithEffects(std::to_string(p.velocity.xy().mag()), "fonts/normalFont", *pos, { 0.5f, 0.6f }, 0.f, 3, vec4_t{ 1,1,1,1 }, nullptr);
		}
		i++;
	}

	for (auto i = 0u; const auto& p : m_oRealityPlayback) {
		if (const auto pos = WorldToScreen(p.origin)) {
			R_AddCmdDrawTextWithEffects(std::to_string(p.velocity.xy().mag()), "fonts/normalFont", *pos, { 0.5f, 0.6f }, 0.f, 3, vec4_t{ 1,1,1,1 }, nullptr);
		}
		i++;
	}

}
