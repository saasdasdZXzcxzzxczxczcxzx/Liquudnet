#ifndef IOS_IOS_MAIN_H
#define IOS_IOS_MAIN_H

#include <base/detect.h>
#if !defined(CONF_PLATFORM_IOS)
#error "This header should only be included when compiling for iOS"
#endif

/**
 * @defgroup iOS iOS
 *
 * iOS-specific functions.
 */

struct SDL_Window;

/**
 * Determines the insets of the drawable area of a window which are covered by
 * the cutout of the display, in pixels.
 *
 * @ingroup iOS
 *
 * Only the cutout hides content. The home indicator is drawn on top of the
 * content instead, so the entire height of the display stays usable.
 *
 * @param pWindow The window to determine the insets of.
 * @param pLeft Pointer to variable that will receive the left inset.
 * @param pRight Pointer to variable that will receive the right inset.
 */
void IosDisplayCutoutInsets(SDL_Window *pWindow, int *pLeft, int *pRight);

/**
 * Initializes iOS specific runtime settings.
 *
 * @ingroup iOS
 *
 * This changes the current working directory to the app bundle so the data
 * files can be read directly from the packaged app.
 *
 * Failure must be handled by exiting the app.
 *
 * @return `nullptr` on success, error message on failure.
 */
const char *InitIos();

/** Maximum number of simultaneous native Liquid Glass overlay buttons. */
// Enough slots for every DoButton_Menu / DoButton_MenuTab drawn on a single
// menu frame (settings pages are the heaviest). The ids are reused per frame.
// In-game touch controls deliberately use the engine-rendered buttons only:
// UIKit overlays there blocked touches and added per-frame UI work.
#define IOS_GLASS_MENU_BUTTONS 32
#define IOS_MAX_GLASS_BUTTONS IOS_GLASS_MENU_BUTTONS

/**
 * Shows, hides or repositions a native UIKit "Liquid Glass" button
 * overlaid on top of the game's drawable area.
 *
 * @ingroup iOS
 *
 * Uses the real system Liquid Glass material (`UIGlassEffect` on iOS 26+,
 * falling back to a regular `UIBlurEffect` on older versions) instead of the
 * engine's own hand-rolled approximation (see CUIRect::SetGlassBackdrop /
 * CGlassBackdrop). The engine's own UI is drawn entirely into one OpenGL
 * surface, so native buttons are real UIKit views layered on top of it.
 * Each button is identified by @p Id so several can be used at once (e.g.
 * the main menu action row). Safe to call every frame: buttons are lazily
 * created on first use and only their frame/label/visibility is updated.
 *
 * @param pWindow The window to attach the button to.
 * @param Id Index in [0, IOS_MAX_GLASS_BUTTONS) identifying the button.
 * @param Visible Whether the button should currently be shown.
 * @param X Left edge of the button, in pixels, relative to the window's drawable area.
 * @param Y Top edge of the button, in pixels, relative to the window's drawable area.
 * @param Width Width of the button, in pixels.
 * @param Height Height of the button, in pixels.
 * @param pLabel UTF-8 label text to show on the button.
 */
void IosSetGlassButtonFrame(SDL_Window *pWindow, int Id, bool Visible, float X, float Y, float Width, float Height, const char *pLabel);

/**
 * Reports and clears whether the native Liquid Glass button with the given id
 * (see IosSetGlassButtonFrame) was tapped since the last call.
 *
 * @ingroup iOS
 *
 * @param Id Index in [0, IOS_MAX_GLASS_BUTTONS) identifying the button.
 * @return `true` if that button was tapped, `false` otherwise.
 */
bool IosConsumeGlassButtonTap(int Id);

/**
 * Starts or updates the Dynamic Island / Lock Screen Live Activity (see
 * src/ios/activity/), or ends it.
 *
 * @ingroup iOS
 *
 * Like the Live Activity itself, this is a proof of concept: it drives a
 * single activity with one plain status string, via the Swift
 * `LiquudnetActivityBridge` class (ActivityKit has no Objective-C or C++
 * surface at all, so a Swift bridge is unavoidable).
 *
 * Safe to call every frame, similar to IosSetGlassButtonFrame: passing
 * Active=true starts the activity on first call and just updates its
 * status text on later calls, and Active=false ends it (a no-op if it
 * isn't running). Also safe to call on iOS versions below 16.2 or when
 * Live Activities are disabled system-wide: the bridge silently no-ops.
 *
 * @param Active Whether the Live Activity should currently be running.
 * @param pStatusText UTF-8 status text to show on the Live Activity.
 */
void IosSetLiveActivityActive(bool Active, const char *pStatusText);

/**
 * Reports whether the Dynamic Island / Lock Screen Live Activity (see
 * IosSetLiveActivityActive) is currently running.
 *
 * @ingroup iOS
 *
 * @return `true` if the Live Activity is currently running, `false` otherwise.
 */
bool IosIsLiveActivityRunning();

/**
 * Finds the on-disk path of the system UI font (San Francisco), so the
 * engine's own FreeType-based text renderer can load it like any other
 * bundled font file.
 *
 * @ingroup iOS
 *
 * There is no supported way to bundle San Francisco itself (its license
 * doesn't allow shipping the font file in a non-Apple app), and its
 * internal FreeType-visible family name (".SFUI...", "SFUI-Text", ...) is
 * a private implementation detail Apple explicitly says not to hardcode,
 * since it has changed between iOS versions. Instead this asks CoreText
 * for the system font's own file URL via the public
 * `CTFontCreateUIFontForLanguage`/`kCTFontURLAttribute` APIs, so the
 * actual family name can be read back from the font file itself (with
 * FreeType, after loading it) instead of being guessed.
 *
 * @param pBuf Buffer that receives the UTF-8 path on success.
 * @param BufSize Size of `pBuf`, in bytes.
 *
 * @return `true` on success, `false` if the system font's file couldn't be
 * located (in which case `pBuf` is left untouched and the engine should
 * fall back to its own bundled fonts, as it does on every other platform).
 */
bool IosGetSystemFontPath(char *pBuf, size_t BufSize);

#endif // IOS_IOS_MAIN_H
