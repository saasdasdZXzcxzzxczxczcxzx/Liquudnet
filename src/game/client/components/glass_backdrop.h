/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_GLASS_BACKDROP_H
#define GAME_CLIENT_COMPONENTS_GLASS_BACKDROP_H

#include <game/client/component.h>

/**
 * Kept as an intentionally empty component so the game-client component list
 * and the header/API stay stable.
 *
 * Liquudnet removed the engine-side frosted-glass backdrop: it needed a GPU
 * framebuffer readback roughly once a second plus a CPU box blur, which made
 * real iOS devices overheat within about a minute of gameplay. Native Liquid
 * Glass is now composited directly by UIKit (see ios/ios_main.mm); plain
 * engine panels are drawn by CUIRect without any backdrop sampling.
 */
class CGlassBackdrop : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }

	void OnInit() override {}
	void OnShutdown() override {}
	void OnRender() override {}
};

#endif
