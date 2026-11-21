import ActivityKit
import Foundation
import UIKit

// Objective-C++ entry point for starting/updating/ending the Liquudnet
// Dynamic Island / Lock Screen Live Activity from the game engine (see
// ios/ios_main.mm, which imports this file's Xcode-generated
// "LiquudnetActivity-Swift.h" header). ActivityKit itself has no
// Objective-C surface at all (Activity<T> is a Swift-only generic type),
// so this class exists purely to give the rest of the app - which is
// C++/Objective-C++ - a plain Objective-C-compatible surface to call.
//
// This is deliberately kept at the floor the main app itself targets
// (see IOS_DEPLOYMENT_TARGET), unlike LiquudnetWidgetExtension, which can
// require iOS 16.1+ on its own since it's a separate optional extension.
// Every method below guards its real work behind `if #available(iOS
// 16.2, *)` and no-ops below that, so this file compiles and the engine
// can call it unconditionally on any supported iOS version - the
// `@objc`-exposed methods themselves carry no `@available` annotation
// (Objective-C has no concept of it), only their *implementations* branch
// on iOS version at runtime. 16.2, not 16.1: the ActivityContent-based
// request/update/end APIs used below were introduced in iOS 16.2 (16.1
// only has the older contentState-only overloads, which this code does
// not use).
@objc(LiquudnetActivityBridge)
public class LiquudnetActivityBridge: NSObject {
    // Type-erased so the stored property itself doesn't need an
    // `@available` annotation (which would force every access, even
    // from within an `if #available` block, to jump through extra hoops).
    private static var activity: Any?

    /// Starts the Live Activity if one isn't already running. No-op if one
    /// is already active (call `update` instead), if Live Activities are
    /// disabled system-wide, or on iOS < 16.2.
    @objc public static func start(withStatus status: String) {
        if #available(iOS 16.2, *) {
            guard activity == nil else { return }
            guard ActivityAuthorizationInfo().areActivitiesEnabled else { return }
            let state = LiquudnetActivityAttributes.ContentState(
                status: status,
                iconPng: Self.appIconPngData())
            let content = ActivityContent(state: state, staleDate: nil, relevanceScore: 0.0)
            do {
                activity = try Activity<LiquudnetActivityAttributes>.request(
                    attributes: LiquudnetActivityAttributes(),
                    content: content,
                    pushType: nil)
            } catch {
                // Deliberately swallowed: this is a cosmetic, optional
                // feature, and ActivityKit can refuse to start an activity
                // for reasons outside this app's control (Live Activities
                // turned off by the person in Settings, a simulator
                // limitation, etc.) - that should never be treated as a
                // hard error by the engine.
                activity = nil
            }
        }
    }

    /// Updates the running Live Activity's status text. No-op if none is
    /// running.
    @objc public static func update(withStatus status: String) {
        if #available(iOS 16.2, *) {
            guard let typedActivity = activity as? Activity<LiquudnetActivityAttributes> else { return }
            let state = LiquudnetActivityAttributes.ContentState(
                status: status,
                iconPng: Self.appIconPngData())
            let content = ActivityContent(state: state, staleDate: nil, relevanceScore: 0.0)
            Task {
                await typedActivity.update(content)
            }
        }
    }

    /// Ends the running Live Activity immediately. No-op if none is
    /// running. Also ends activities of this type that were started by a
    /// previous process (e.g. one that was killed instead of quitting
    /// cleanly), so a quit always clears the Dynamic Island.
    @objc public static func end() {
        if #available(iOS 16.2, *) {
            activity = nil
            Task {
                for Current in Activity<LiquudnetActivityAttributes>.activities {
                    await Current.end(nil, dismissalPolicy: .immediate)
                }
            }
        }
    }

    private static func appIconPngData() -> Data {
        // UIKit is available here (main app process); the widget extension
        // only receives the resulting bytes in ContentState.
        UIApplication.shared.applicationIconImage?.pngData() ?? Data()
    }

    @objc public static var isRunning: Bool {
        if #available(iOS 16.2, *) {
            return activity is Activity<LiquudnetActivityAttributes>
        }
        return false
    }
}
