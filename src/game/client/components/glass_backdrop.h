/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_GLASS_BACKDROP_H
#define GAME_CLIENT_COMPONENTS_GLASS_BACKDROP_H

#include <engine/graphics.h>
#include <engine/shared/jobs.h>

#include <game/client/component.h>

#include <memory>

/**
 * Drives the real Liquid Glass backdrop blur (see CUIRect::SetGlassBackdrop):
 * roughly once a second, grabs a copy of the just-rendered frame, downsamples
 * and blurs it on a worker thread, and uploads the result as a texture that
 * CUIRect::Draw()/Draw4() paint underneath every translucent panel - menu
 * panels and in-game touch-control buttons alike - so they read as real
 * frosted glass instead of a flat tinted gradient. Captures run continuously,
 * in menus and in-game, so a touch-control button genuinely shows a live
 * (about 1-second-old) blurred view of whatever is behind it - the map,
 * chat, other HUD elements - not just a static tint.
 *
 * Deliberately simple: a single CPU box blur done at a small fixed
 * resolution, refreshed about once a second rather than every frame, to keep
 * the capture call (which stalls the GPU pipeline - see
 * IEngineGraphics::ReadFramebuffer) cheap enough not to threaten 60fps or
 * cause thermal/battery issues; an earlier, much more frequent (~4/sec),
 * menu-only version was confirmed to run a real device hot and was dialed
 * back, then widened to also run in-game at the user's request.
 * Known, accepted tradeoffs (see glass_backdrop.cpp and ui_rect.cpp):
 *  - The backdrop texture used while drawing a frame's panels is the frame
 *    captured roughly a second earlier, so fast on-screen motion (e.g. chat
 *    scrolling past, the player moving) will visibly lag behind what's
 *    actually there right now. Raising the capture rate would reduce this
 *    at a battery/thermal cost - see the interval comment in
 *    ShouldCapture().
 *  - Rounded panel corners can show a few pixels of the (rectangular)
 *    backdrop texture peeking past the rounded tint silhouette.
 */
class CGlassBackdrop : public CComponent
{
	class CBlurJob : public IJob
	{
		CImageInfo m_SourceImage; // moved in from ReadFramebuffer, owned by this job, freed on destruction
		CImageInfo m_ResultImage; // produced by Run(), moved out by the component once Done()

	protected:
		void Run() override;

	public:
		explicit CBlurJob(CImageInfo &&SourceImage);
		~CBlurJob() override;

		CImageInfo &ResultImage() { return m_ResultImage; }
	};

	class IEngineGraphics *m_pEngineGraphics = nullptr;
	IGraphics::CTextureHandle m_BackdropTexture;
	std::shared_ptr<CBlurJob> m_pPendingJob;
	int64_t m_LastCaptureTime = -1;

	bool ShouldCapture() const;

public:
	int Sizeof() const override { return sizeof(*this); }

	void OnInit() override;
	void OnShutdown() override;
	void OnRender() override;
};

#endif
