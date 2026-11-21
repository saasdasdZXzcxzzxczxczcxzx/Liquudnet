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

/**
 * Shows, hides or repositions a single native UIKit "Liquid Glass" button
 * overlaid on top of the game's drawable area.
 *
 * @ingroup iOS
 *
 * This is a proof of concept for using the real system Liquid Glass
 * material (`UIGlassEffect` on iOS 26+, falling back to a regular
 * `UIBlurEffect` on older versions) instead of the engine's own hand-rolled
 * approximation (see CUIRect::SetGlassBackdrop / CGlassBackdrop): since the
 * game's own UI is drawn entirely by the engine into one OpenGL surface
 * rather than as real UIKit views, this only works for UI elements that are
 * willing to be actual native views layered on top of that surface - which
 * this function provides exactly one of. It is not a general mechanism for
 * arbitrary engine UI.
 *
 * Safe to call every frame: it lazily creates the button on first use and
 * just updates its frame/label/visibility afterwards, so the engine can
 * simply stop calling it with Visible=false (or stop calling it at all)
 * once the relevant menu page is no longer shown.
 *
 * @param pWindow The window to attach the button to.
 * @param Visible Whether the button should currently be shown.
 * @param X Left edge of the button, in pixels, relative to the window's drawable area.
 * @param Y Top edge of the button, in pixels, relative to the window's drawable area.
 * @param Width Width of the button, in pixels.
 * @param Height Height of the button, in pixels.
 * @param pLabel UTF-8 label text to show on the button.
 */
void IosSetGlassButtonFrame(SDL_Window *pWindow, bool Visible, float X, float Y, float Width, float Height, const char *pLabel);

/**
 * Reports and clears whether the native Liquid Glass button (see
 * IosSetGlassButtonFrame) has been tapped since the last call.
 *
 * @ingroup iOS
 *
 * @return `true` if the button was tapped since the last call to this function, `false` otherwise.
 */
bool IosConsumeGlassButtonTap();

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
 * isn't running). Also safe to call on iOS versions below 16.1 or when
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

#endif // IOS_IOS_MAIN_H
