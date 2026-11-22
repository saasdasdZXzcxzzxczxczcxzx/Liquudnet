/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus.h"

#include <base/color.h>
#include <base/str.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
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

	// --- Player trails ------------------------------------------------------
	// Purely cosmetic fading trail behind player tees (see
	// game/client/components/player_trail.h). Enable checkbox is folded into
	// the color-picker line itself, same pattern as e.g. the "Friend message"
	// color option in menus_settings_appearance.cpp.
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(24.0f, &Label, &MainView);
	Ui()->DoLabel(&Label, Localize("Player trails"), 16.0f, TEXTALIGN_ML);

	static CButtonContainer s_TrailColorReset;
	MainView.HSplitTop(6.0f, nullptr, &MainView);
	DoLine_ColorPicker(&s_TrailColorReset, 20.0f, 13.0f, 5.0f, &MainView, Localize("Show player trails"), &g_Config.m_ClPlayerTrailColor,
		color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::ClPlayerTrailColor, true)), true, &g_Config.m_ClPlayerTrail, true);

	if(g_Config.m_ClPlayerTrail)
	{
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_ClPlayerTrailLength, &g_Config.m_ClPlayerTrailLength, &Button, Localize("Trail length"), 2, 60);

		MainView.HSplitTop(20.0f, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_ClPlayerTrailWidth, &g_Config.m_ClPlayerTrailWidth, &Button, Localize("Trail width"), 2, 64);
	}

	// Native UIKit Liquid Glass button (see ios/ios_main.mm,
	// IGraphics::SetNativeGlassButton). The engine's old frosted-glass
	// backdrop was removed - it overheated iOS devices.
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Button, &MainView);
	Button.VSplitLeft(280.0f, &Button, nullptr);

	static int s_GlassButtonTapCount = 0;

#if defined(CONF_PLATFORM_IOS)
	// Convert the reserved CUIRect, in UI units (0..Ui()->Screen()->w/h),
	// into the pixel coordinates the native overlay needs. On iOS Liquudnet
	// renders edge-to-edge (IosDisplayCutoutInsets returns zero), so no extra
	// cutout offset has to be applied here.
	const CUIRect *pUiScreen = Ui()->Screen();
	const float ScaleX = Graphics()->ScreenWidth() / pUiScreen->w;
	const float ScaleY = Graphics()->ScreenHeight() / pUiScreen->h;

	char aLabel[64];
	str_format(aLabel, sizeof(aLabel), "Glass Button (%d)", s_GlassButtonTapCount);
	Graphics()->SetNativeGlassButton(0, true, Button.x * ScaleX, Button.y * ScaleY, Button.w * ScaleX, Button.h * ScaleY, aLabel);
	if(Graphics()->ConsumeNativeGlassButtonTap(0))
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

	// --- Dynamic Island / Live Activity -----------------------------------
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Button, &MainView);
	Button.VSplitLeft(280.0f, &Button, nullptr);

	static CButtonContainer s_LiveActivityToggleId;
#if defined(CONF_PLATFORM_IOS)
	const bool LiveActivityRunning = Graphics()->IsLiveActivityRunning();
	if(DoButton_Menu(&s_LiveActivityToggleId, LiveActivityRunning ? Localize("Stop Dynamic Island") : Localize("Start Dynamic Island"), 0, &Button))
	{
		Graphics()->SetLiveActivityActive(!LiveActivityRunning, "Playing Liquudnet");
	}
#else
	DoButton_Menu(&s_LiveActivityToggleId, Localize("Dynamic Island (iOS only)"), 0, &Button);
#endif
}
