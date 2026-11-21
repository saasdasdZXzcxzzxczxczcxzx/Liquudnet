/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus.h"

#include <base/str.h>

#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>

// This tab holds Liquudnet's own additions, kept separate from the
// "DDNet" tab (menus_settings_ddnet.cpp), which stays as close to upstream
// as possible. Reuses the one spare settings-page camera slot upstream left
// for exactly this (CMenuBackground::POS_SETTINGS_RESERVED0, wired up in
// RenderSettings()).
void CMenus::RenderSettingsLiquudnet(CUIRect MainView)
{
	CUIRect Header, Label, Button;

	MainView.HSplitTop(30.0f, &Header, &MainView);
	Ui()->DoLabel(&Header, Localize("Liquudnet"), 20.0f, TEXTALIGN_ML);

	MainView.HSplitTop(10.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Settings specific to this client. The original client's own settings stay on the \"DDNet\" tab."), 14.0f, TEXTALIGN_TL);

	// --- Real iOS Liquid Glass experiment ---------------------------------
	// A single native UIKit button (see ios/ios_main.mm,
	// IGraphics::SetNativeGlassButton), using the real system Liquid Glass
	// material on iOS 26+ instead of the engine's own hand-rolled backdrop
	// blur (CUIRect::SetGlassBackdrop). Proof of concept only: the engine's
	// own UI is drawn into one OpenGL surface, not as real platform views,
	// so this approach only works per-element for things willing to be an
	// actual native view - it is not a path to giving the whole menu system
	// real system glass.
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(24.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Real iOS Liquid Glass (experimental)"), 16.0f, TEXTALIGN_ML);

	MainView.HSplitTop(6.0f, nullptr, &MainView);
	MainView.HSplitTop(34.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("iOS only. Renders as a real system-glass button on top of the game, not an engine-drawn one. Tap it to check the round trip back into the game works."), 12.0f, TEXTALIGN_TL);

	MainView.HSplitTop(10.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Button, &MainView);
	Button.VSplitLeft(280.0f, &Button, nullptr);

	static int s_GlassButtonTapCount = 0;

#if defined(CONF_PLATFORM_IOS)
	// Convert the reserved CUIRect, in UI units (0..Ui()->Screen()->w/h),
	// into the pixel coordinates the native overlay needs. Known rough edge:
	// this does not yet account for the display-cutout viewport offset (see
	// CGraphics_Threaded::AdjustViewport), so in landscape orientations with
	// a cutout on one side, the native button can sit slightly off from
	// where an engine-drawn button at the same CUIRect would land.
	const CUIRect *pUiScreen = Ui()->Screen();
	const float ScaleX = Graphics()->ScreenWidth() / pUiScreen->w;
	const float ScaleY = Graphics()->ScreenHeight() / pUiScreen->h;

	char aLabel[64];
	str_format(aLabel, sizeof(aLabel), "Glass Button (%d)", s_GlassButtonTapCount);
	Graphics()->SetNativeGlassButton(true, Button.x * ScaleX, Button.y * ScaleY, Button.w * ScaleX, Button.h * ScaleY, aLabel);
	if(Graphics()->ConsumeNativeGlassButtonTap())
	{
		s_GlassButtonTapCount++;
	}
	// Nothing drawn here by the engine on purpose: the real button is the
	// native view rendered on top, not anything in this CUIRect's footprint.
#else
	static CButtonContainer s_GlassButtonFallbackId;
	if(DoButton_Menu(&s_GlassButtonFallbackId, Localize("Glass Button (iOS only)"), 0, &Button))
	{
		s_GlassButtonTapCount++;
	}
#endif

	MainView.HSplitTop(12.0f, nullptr, &MainView);
	MainView.HSplitTop(24.0f, &Label, &MainView);
	char aTapLabel[32];
	str_format(aTapLabel, sizeof(aTapLabel), Localize("Tapped: %d"), s_GlassButtonTapCount);
	Ui()->DoLabel(&Label, aTapLabel, 14.0f, TEXTALIGN_ML);
}
