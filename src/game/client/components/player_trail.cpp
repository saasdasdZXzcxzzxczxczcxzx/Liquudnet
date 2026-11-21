/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "player_trail.h"

#include <base/color.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <algorithm>

void CPlayerTrail::OnReset()
{
	for(std::vector<STrailPoint> &vTrail : m_aTrail)
		vTrail.clear();
}

void CPlayerTrail::UpdateAndRender(int ClientId, vec2 Pos, float Alpha)
{
	if(!g_Config.m_ClPlayerTrail || !in_range(ClientId, 0, MAX_CLIENTS - 1))
		return;

	std::vector<STrailPoint> &vTrail = m_aTrail[ClientId];

	// If the tee jumped a long distance since the last recorded point
	// (teleporter, respawn, a disconnected client's slot being reused by
	// someone else, ...) the trail would otherwise draw one long straight
	// line across the map - start a fresh trail instead.
	constexpr float MaxStepDistance = 400.0f;
	if(!vTrail.empty() && distance(vTrail.back().m_Pos, Pos) > MaxStepDistance)
		vTrail.clear();

	// Only record a new point once the tee has actually moved a bit, so a
	// standing-still player doesn't accumulate a pile of identical points
	// (which would also break the age-based fade below, since it fades by
	// point index, not by distance or time).
	constexpr float MinPointSpacing = 2.0f;
	if(vTrail.empty() || distance(vTrail.back().m_Pos, Pos) > MinPointSpacing)
	{
		vTrail.push_back({Pos});

		const size_t MaxLength = (size_t)std::max(2, g_Config.m_ClPlayerTrailLength);
		while(vTrail.size() > MaxLength)
			vTrail.erase(vTrail.begin());
	}

	if(vTrail.size() < 2)
		return;

	const ColorRGBA BaseColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClPlayerTrailColor, true));

	// Drawn as a ribbon of quads rather than GL_LINES: thin 1px lines have no
	// portable way to be made thicker (glLineWidth > 1 isn't reliable across
	// the GL/GL ES backends this engine targets), so each segment is instead
	// a small quad, offset perpendicular to the segment direction by half
	// the configured width on each side.
	const float HalfWidth = g_Config.m_ClPlayerTrailWidth / 2.0f;

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	const size_t NumSegments = vTrail.size() - 1;
	for(size_t i = 0; i < NumSegments; i++)
	{
		// Oldest segment is fully faded out, newest is at the configured
		// alpha, matching the tee's own alpha (e.g. other-team transparency).
		const float Age = (float)(i + 1) / (float)NumSegments;
		ColorRGBA SegmentColor = BaseColor;
		SegmentColor.a *= Age * Alpha;

		const vec2 Start = vTrail[i].m_Pos;
		const vec2 End = vTrail[i + 1].m_Pos;
		vec2 Dir = End - Start;
		const float Len = length(Dir);
		if(Len < 0.0001f)
			continue;
		Dir = Dir / Len;
		const vec2 Normal = vec2(-Dir.y, Dir.x) * HalfWidth;

		const IGraphics::CFreeformItem Freeform(
			Start.x - Normal.x, Start.y - Normal.y,
			Start.x + Normal.x, Start.y + Normal.y,
			End.x - Normal.x, End.y - Normal.y,
			End.x + Normal.x, End.y + Normal.y);
		Graphics()->SetColor(SegmentColor);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}
	Graphics()->QuadsEnd();
}
