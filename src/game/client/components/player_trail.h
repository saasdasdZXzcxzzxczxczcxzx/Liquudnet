/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_PLAYER_TRAIL_H
#define GAME_CLIENT_COMPONENTS_PLAYER_TRAIL_H

#include <base/vmath.h>

#include <generated/protocol.h>

#include <game/client/component.h>

#include <vector>

// Liquudnet addition, not part of upstream DDNet: draws a short fading
// trail behind each player's tee as it moves, in a client-configurable
// color (see menus_settings_liquudnet.cpp and ClPlayerTrail* in
// config_variables.h). Purely cosmetic and purely client-side: it only
// ever reads the already-computed render position each player's tee is
// drawn at (see CPlayers::RenderPlayer()), so it doesn't touch prediction,
// networking or any other game state.
class CPlayerTrail : public CComponent
{
	struct STrailPoint
	{
		vec2 m_Pos;
	};

	std::vector<STrailPoint> m_aTrail[MAX_CLIENTS];

public:
	int Sizeof() const override { return sizeof(*this); }

	// Drop all recorded trail points, e.g. on map change/reconnect, so old
	// positions from a previous map or round never draw a stray line.
	void OnReset() override;

	// Nothing to do here: trails are updated and drawn from
	// CPlayers::RenderPlayer() instead (see UpdateAndRender()), since that
	// is where each player's already-interpolated/predicted render
	// position for the current frame is available, and because the trail
	// needs to be drawn before the tee itself so the tee sits on top of it.
	void OnRender() override {}

	/**
	 * Records `Pos` as the player's latest position and draws the fading
	 * trail of their previously recorded positions. Call this once per
	 * visible player per frame, right before rendering their tee.
	 *
	 * @param ClientId The player's client ID. Call sites that render things
	 * other than real players (ghosts, shadows, ...) should not call this.
	 * @param Pos The position the tee is about to be drawn at this frame.
	 * @param Alpha Overall opacity to render the trail with, matching the
	 * tee's own alpha (e.g. for other-team transparency).
	 */
	void UpdateAndRender(int ClientId, vec2 Pos, float Alpha);
};

#endif
