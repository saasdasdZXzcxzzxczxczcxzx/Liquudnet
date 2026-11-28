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

	// Native UIKit Liquid Glass button. DoButton_Menu routes this through
	// the shared per-frame native-glass layer on iOS (see menus.cpp); the
	// engine's old frosted-glass backdrop was removed because it overheated
	// iOS devices.
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Button, &MainView);
	Button.VSplitLeft(280.0f, &Button, nullptr);
	static CButtonContainer s_GlassButtonId;
	DoButton_Menu(&s_GlassButtonId, "Glass Button", 0, &Button);

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

	// Raw ActivityKit diagnostics (see IosGetLiveActivityDiagnostics): this
	// is the actual reason a Live Activity doesn't start even when Live
	// Activities are allowed in iOS Settings (e.g. "lastError=..."), shown
	// directly here because there's usually no Mac/Console.app around to
	// read the engine log on a real device.
	MainView.HSplitTop(8.0f, nullptr, &MainView);
	MainView.HSplitTop(60.0f, &Label, &MainView);
	char aDiagnostics[256];
	if(Graphics()->GetLiveActivityDiagnostics(aDiagnostics, sizeof(aDiagnostics)))
	{
		Ui()->DoLabel(&Label, aDiagnostics, 10.0f, TEXTALIGN_ML, {.m_MaxWidth = Label.w});
	}
#else
	DoButton_Menu(&s_LiveActivityToggleId, Localize("Dynamic Island (iOS only)"), 0, &Button);
#endif

	// --- Game view aspect ratio -------------------------------------------
	MainView.HSplitTop(20.0f, nullptr, &MainView);
	MainView.HSplitTop(40.0f, &Button, &MainView);
	Button.VSplitLeft(280.0f, &Button, nullptr);
	static const char *const s_apAspects[] = {"Aspect ratio: native", "Aspect ratio: 4:3", "Aspect ratio: 16:9", "Aspect ratio: 16:10", "Aspect ratio: 5:4", "Aspect ratio: 1:1"};
	static CButtonContainer s_AspectButtonId;
	const int Aspect = (g_Config.m_ClLiquudnetAspect < 0 || g_Config.m_ClLiquudnetAspect > 5) ? 0 : g_Config.m_ClLiquudnetAspect;
	if(DoButton_Menu(&s_AspectButtonId, s_apAspects[Aspect], 0, &Button))
	{
		g_Config.m_ClLiquudnetAspect = (Aspect + 1) % 6;
		Graphics()->RefreshViewport();
	}
}
