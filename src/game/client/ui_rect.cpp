/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ui_rect.h"

#include <engine/graphics.h>

#include <algorithm>

IGraphics *CUIRect::ms_pGraphics = nullptr;

void CUIRect::HSplitMid(CUIRect *pTop, CUIRect *pBottom, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.h / 2;
	const float HalfSpacing = Spacing / 2;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut - HalfSpacing;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut + HalfSpacing;
		pBottom->w = r.w;
		pBottom->h = Cut - HalfSpacing;
	}
}

void CUIRect::HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + Cut;
		pBottom->w = r.w;
		pBottom->h = r.h - Cut;
	}
}

void CUIRect::HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const
{
	CUIRect r = *this;

	if(pTop)
	{
		pTop->x = r.x;
		pTop->y = r.y;
		pTop->w = r.w;
		pTop->h = r.h - Cut;
	}

	if(pBottom)
	{
		pBottom->x = r.x;
		pBottom->y = r.y + r.h - Cut;
		pBottom->w = r.w;
		pBottom->h = Cut;
	}
}

void CUIRect::VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Spacing) const
{
	CUIRect r = *this;
	const float Cut = r.w / 2;
	const float HalfSpacing = Spacing / 2;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut - HalfSpacing;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut + HalfSpacing;
		pRight->y = r.y;
		pRight->w = Cut - HalfSpacing;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + Cut;
		pRight->y = r.y;
		pRight->w = r.w - Cut;
		pRight->h = r.h;
	}
}

void CUIRect::VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const
{
	CUIRect r = *this;

	if(pLeft)
	{
		pLeft->x = r.x;
		pLeft->y = r.y;
		pLeft->w = r.w - Cut;
		pLeft->h = r.h;
	}

	if(pRight)
	{
		pRight->x = r.x + r.w - Cut;
		pRight->y = r.y;
		pRight->w = Cut;
		pRight->h = r.h;
	}
}

void CUIRect::Margin(vec2 Cut, CUIRect *pOtherRect) const
{
	CUIRect r = *this;

	pOtherRect->x = r.x + Cut.x;
	pOtherRect->y = r.y + Cut.y;
	pOtherRect->w = r.w - 2 * Cut.x;
	pOtherRect->h = r.h - 2 * Cut.y;
}

void CUIRect::Margin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(Cut, Cut), pOtherRect);
}

void CUIRect::VMargin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(Cut, 0.0f), pOtherRect);
}

void CUIRect::HMargin(float Cut, CUIRect *pOtherRect) const
{
	Margin(vec2(0.0f, Cut), pOtherRect);
}

bool CUIRect::Inside(vec2 Point) const
{
	return Point.x >= x && Point.x < x + w && Point.y >= y && Point.y < y + h;
}

// Liquid Glass fill: a flat color panel becomes a subtle vertical glass gradient
// (brighter, slightly desaturated sheen at the top edge; a touch darker at the
// bottom) plus a thin specular highlight line along the top edge. This is purely
// a visual change to the shared fill primitive used by nearly every panel/button
// in the UI; it does not touch game logic, networking, or input handling.
static ColorRGBA GlassTopTint(ColorRGBA Color)
{
	const float Mix = 0.22f;
	ColorRGBA Out(
		Color.r + (1.0f - Color.r) * Mix,
		Color.g + (1.0f - Color.g) * Mix,
		Color.b + (1.0f - Color.b) * Mix,
		std::clamp(Color.a * 1.12f, 0.0f, 1.0f));
	return Out;
}

static ColorRGBA GlassBottomTint(ColorRGBA Color)
{
	const float Mix = 0.10f;
	return ColorRGBA(
		Color.r * (1.0f - Mix),
		Color.g * (1.0f - Mix),
		Color.b * (1.0f - Mix),
		Color.a);
}

void CUIRect::Draw(ColorRGBA Color, int Corners, float Rounding) const
{
	Draw4(GlassTopTint(Color), GlassTopTint(Color), GlassBottomTint(Color), GlassBottomTint(Color), Corners, Rounding);
}

void CUIRect::Draw4(ColorRGBA ColorTopLeft, ColorRGBA ColorTopRight, ColorRGBA ColorBottomLeft, ColorRGBA ColorBottomRight, int Corners, float Rounding) const
{
	ms_pGraphics->DrawRect4(x, y, w, h, ColorTopLeft, ColorTopRight, ColorBottomLeft, ColorBottomRight, Corners, Rounding);

	// Specular highlight: a thin brighter line just inside the top edge, the
	// classic "light catching the rim of the glass" cue. Skipped for rects too
	// thin to show it (separators, scrollbar thumbs, etc.) or fully transparent
	// rects (invisible hitboxes), so it never adds a visible seam where none was
	// intended and never costs a draw call for invisible elements.
	const float MaxSourceAlpha = std::max({ColorTopLeft.a, ColorTopRight.a, ColorBottomLeft.a, ColorBottomRight.a});
	if(MaxSourceAlpha > 0.02f && h >= 6.0f && w >= 6.0f)
	{
		const float Inset = std::min(Rounding * 0.5f + 1.0f, h * 0.25f);
		const IGraphics::CLineItem HighlightLine(x + Inset, y + Inset, x + w - Inset, y + Inset);
		ms_pGraphics->TextureClear();
		ms_pGraphics->LinesBegin();
		ms_pGraphics->SetColor(1.0f, 1.0f, 1.0f, std::min(0.22f, MaxSourceAlpha * 0.3f));
		ms_pGraphics->LinesDraw(&HighlightLine, 1);
		ms_pGraphics->LinesEnd();
	}
}

void CUIRect::DrawOutline(ColorRGBA Color) const
{
	const IGraphics::CLineItem aArray[] = {
		IGraphics::CLineItem(x, y, x + w, y),
		IGraphics::CLineItem(x + w, y, x + w, y + h),
		IGraphics::CLineItem(x + w, y + h, x, y + h),
		IGraphics::CLineItem(x, y + h, x, y)};
	ms_pGraphics->TextureClear();
	ms_pGraphics->LinesBegin();
	ms_pGraphics->SetColor(Color);
	ms_pGraphics->LinesDraw(aArray, std::size(aArray));
	ms_pGraphics->LinesEnd();
}
