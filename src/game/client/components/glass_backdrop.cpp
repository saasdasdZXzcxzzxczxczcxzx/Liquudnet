/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "glass_backdrop.h"

#include <base/mem.h>
#include <base/time.h>

#include <engine/engine.h>
#include <engine/kernel.h>

#include <game/client/ui.h>
#include <game/client/ui_rect.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
	// Small, fixed destination resolution for the blurred backdrop. The blur
	// itself (an O(Width*Height) box filter run a few times) is the expensive
	// part, so downsampling to this size first - rather than blurring the full
	// capture - is what keeps this affordable on a phone every ~1s.
	constexpr int MAX_BACKDROP_WIDTH = 480;
	constexpr int MIN_BACKDROP_HEIGHT = 32;
	constexpr int BLUR_PASSES = 3;

	void BoxBlurInPlace(uint8_t *pData, int Width, int Height)
	{
		// Separable 3x3 box blur, horizontal pass then vertical, repeated
		// BLUR_PASSES times to approximate a wider, softer falloff cheaply.
		// Not a real gaussian, just enough to kill the "flat gradient" look.
		std::vector<uint8_t> vScratch(static_cast<size_t>(Width) * Height * 4);
		for(int Pass = 0; Pass < BLUR_PASSES; Pass++)
		{
			for(int y = 0; y < Height; y++)
			{
				for(int x = 0; x < Width; x++)
				{
					int Sum[4] = {0, 0, 0, 0};
					for(int dx = -1; dx <= 1; dx++)
					{
						const int SampleX = std::clamp(x + dx, 0, Width - 1);
						const uint8_t *pPixel = &pData[(static_cast<size_t>(y) * Width + SampleX) * 4];
						for(int c = 0; c < 4; c++)
							Sum[c] += pPixel[c];
					}
					uint8_t *pOut = &vScratch[(static_cast<size_t>(y) * Width + x) * 4];
					for(int c = 0; c < 4; c++)
						pOut[c] = static_cast<uint8_t>(Sum[c] / 3);
				}
			}
			mem_copy(pData, vScratch.data(), vScratch.size());

			for(int y = 0; y < Height; y++)
			{
				for(int x = 0; x < Width; x++)
				{
					int Sum[4] = {0, 0, 0, 0};
					for(int dy = -1; dy <= 1; dy++)
					{
						const int SampleY = std::clamp(y + dy, 0, Height - 1);
						const uint8_t *pPixel = &pData[(static_cast<size_t>(SampleY) * Width + x) * 4];
						for(int c = 0; c < 4; c++)
							Sum[c] += pPixel[c];
					}
					uint8_t *pOut = &vScratch[(static_cast<size_t>(y) * Width + x) * 4];
					for(int c = 0; c < 4; c++)
						pOut[c] = static_cast<uint8_t>(Sum[c] / 3);
				}
			}
			mem_copy(pData, vScratch.data(), vScratch.size());
		}
	}
}

CGlassBackdrop::CBlurJob::CBlurJob(CImageInfo &&SourceImage) :
	m_SourceImage(std::move(SourceImage))
{
	Abortable(true);
}

CGlassBackdrop::CBlurJob::~CBlurJob()
{
	m_SourceImage.Free();
	m_ResultImage.Free();
}

void CGlassBackdrop::CBlurJob::Run()
{
	if(m_SourceImage.m_Format != CImageInfo::FORMAT_RGBA || m_SourceImage.m_Width == 0 || m_SourceImage.m_Height == 0)
		return;

	const int SrcWidth = static_cast<int>(m_SourceImage.m_Width);
	const int SrcHeight = static_cast<int>(m_SourceImage.m_Height);
	const int DstWidth = std::min(MAX_BACKDROP_WIDTH, SrcWidth);
	const int DstHeight = std::max(MIN_BACKDROP_HEIGHT, DstWidth * SrcHeight / SrcWidth);

	m_ResultImage.m_Width = DstWidth;
	m_ResultImage.m_Height = DstHeight;
	m_ResultImage.m_Format = CImageInfo::FORMAT_RGBA;
	m_ResultImage.Allocate();

	// Downsample via point sampling (nearest neighbour); the box blur
	// applied right after smooths over the aliasing this introduces, so a
	// proper area-average downsample isn't worth the extra cost here.
	for(int dy = 0; dy < DstHeight; dy++)
	{
		const int SrcY = std::min(SrcHeight - 1, dy * SrcHeight / DstHeight);
		for(int dx = 0; dx < DstWidth; dx++)
		{
			const int SrcX = std::min(SrcWidth - 1, dx * SrcWidth / DstWidth);
			const uint8_t *pSrc = &m_SourceImage.m_pData[(static_cast<size_t>(SrcY) * SrcWidth + SrcX) * 4];
			uint8_t *pDst = &m_ResultImage.m_pData[(static_cast<size_t>(dy) * DstWidth + dx) * 4];
			mem_copy(pDst, pSrc, 4);
		}
	}

	BoxBlurInPlace(m_ResultImage.m_pData, DstWidth, DstHeight);
}

bool CGlassBackdrop::ShouldCapture() const
{
	// Captures run both in menus and in-game: in-game touch-control buttons
	// (movement, jump, hook, ...) are translucent too and are meant to show
	// a live blurred view of whatever is behind them (the map, chat, other
	// HUD elements), not just a flat tint. See OnRender() for why that means
	// the backdrop is always kept live rather than frozen/cleared outside
	// menus.
	if(m_pPendingJob)
		return false; // previous capture is still being downsampled/blurred
	if(m_pEngineGraphics == nullptr)
		return false;
	const int64_t Now = time_get();
	// ~1 capture/sec. Started at 4/sec, but each capture forces a GPU
	// pipeline stall (see IEngineGraphics::ReadFramebuffer / WaitForIdle),
	// which turned out to be a real, noticeable battery/thermal cost on a
	// real device when menus were left open for a while - a frosted-glass
	// background doesn't need to track the live scene that closely, so this
	// trades a bit of backdrop freshness for being cheap enough to leave
	// running continuously, including during actual gameplay.
	const int64_t Interval = time_freq();
	return m_LastCaptureTime < 0 || Now - m_LastCaptureTime >= Interval;
}

void CGlassBackdrop::OnInit()
{
	m_pEngineGraphics = Kernel()->RequestInterface<IEngineGraphics>();
}

void CGlassBackdrop::OnShutdown()
{
	m_pPendingJob = nullptr;
	if(m_BackdropTexture.IsValid())
		Graphics()->UnloadTexture(&m_BackdropTexture);
	CUIRect::SetGlassBackdrop(IGraphics::CTextureHandle(), 0.0f, 0.0f);
}

void CGlassBackdrop::OnRender()
{
	// Pick up a finished blur job and upload it as the new backdrop texture.
	// This becomes visible starting with the UI drawn next frame; this
	// frame's panels (already past this point in the render order) keep
	// using whatever texture was already set.
	if(m_pPendingJob && m_pPendingJob->Done())
	{
		if(m_pPendingJob->State() == IJob::STATE_DONE && m_pPendingJob->ResultImage().m_pData != nullptr)
		{
			IGraphics::CTextureHandle OldTexture = m_BackdropTexture;
			m_BackdropTexture = Graphics()->LoadTextureRawMove(m_pPendingJob->ResultImage(), 0, "glass-backdrop");
			if(OldTexture.IsValid())
				Graphics()->UnloadTexture(&OldTexture);
		}
		m_pPendingJob = nullptr;
	}

	// Apply the latest known backdrop everywhere, menu or in-game: captures
	// keep refreshing continuously (see ShouldCapture()), so there's no
	// "stale freeze-frame" problem here as long as captures are actually
	// running - touch-control buttons end up showing a live, roughly
	// 1-second-old blurred view of the map/chat/HUD behind them, same as
	// menu panels. An invalid handle (before the first capture completes)
	// correctly falls back to the plain gradient look in CUIRect::Draw4().
	const CUIRect *pScreen = Ui()->Screen();
	CUIRect::SetGlassBackdrop(m_BackdropTexture, pScreen->w, pScreen->h);

	// Kick off a new capture+blur, throttled, so the result is ready to use
	// a frame or two from now.
	if(ShouldCapture())
	{
		m_LastCaptureTime = time_get();
		CImageInfo Capture;
		m_pEngineGraphics->ReadFramebuffer(Capture);
		if(Capture.m_pData != nullptr)
		{
			m_pPendingJob = std::make_shared<CBlurJob>(std::move(Capture));
			Engine()->AddJob(m_pPendingJob);
		}
	}
}
