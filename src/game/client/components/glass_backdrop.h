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
 * a few times a second, grabs a copy of the just-rendered frame, downsamples
 * and blurs it on a worker thread, and uploads the result as a texture that
 * CUIRect::Draw()/Draw4() paint underneath translucent menu panels so they
 * read as real frosted glass instead of a flat tinted gradient.
 *
 * Deliberately simple: a single CPU box blur done at a small fixed
 * resolution, refreshed a few times a second rather than every frame, to
 * keep the capture call (which stalls the GPU pipeline briefly - see
 * IEngineGraphics::ReadFramebuffer) cheap enough not to threaten 60fps.
 * Known, accepted tradeoffs (see glass_backdrop.cpp and ui_rect.cpp):
 *  - The backdrop texture used while drawing a frame's panels is the frame
 *    captured at the END of the *previous* frame, which already includes
 *    that previous frame's own glass panels ("blur of blur"). Cheap
 *    backdrop-blur techniques commonly accept this; it is not corrected.
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
