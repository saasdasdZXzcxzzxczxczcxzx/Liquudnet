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
// Every method below guards its real work behind `if #available(iOS
// 16.2, *)` and no-ops below that, so this file compiles and the engine
// can call it unconditionally on any supported iOS version.
@objc(LiquudnetActivityBridge)
public class LiquudnetActivityBridge: NSObject {
    // Type-erased so the stored property itself doesn't need an
    // `@available` annotation.
    private static var activity: Any?

    // Last reason ActivityKit refused to start/update an activity. Exposed
    // to the engine so it ends up in the ordinary DDNet log - previously
    // every error here was swallowed, which made "Activity не стартует"
    // impossible to diagnose from a device.
    private static var lastError: String = "none"

    private static func log(_ message: String) {
        // NSLog goes to the iOS unified log (visible in Console.app and in
        // crash/sysdiagnose reports) even though the engine's own stdout is
        // not captured for App Store builds.
        NSLog("[LiquudnetActivity] %@", message)
    }

    // ActivityKit and UIKit's application-icon APIs must run on the main
    // thread. The engine calls this bridge from its render thread, so hop
    // over synchronously (unless we're already on main) and keep the
    // start/update/end calls ordered with the engine's log diagnostics.
    private static func onMain<T>(_ work: () -> T) -> T {
        if Thread.isMainThread {
            return work()
        }
        return DispatchQueue.main.sync(execute: work)
    }

    // If the process was killed while a Live Activity was still on screen,
    // the system keeps that activity alive but our static `activity` is nil
    // after relaunch. Adopt it so subsequent status updates go to the
    // existing island instead of trying to create a second one.
    @available(iOS 16.2, *)
    private static func adoptExistingIfNeeded() {
        guard activity == nil else {
            return
        }
        if let existing = Activity<LiquudnetActivityAttributes>.activities.first {
            activity = existing
            log("adopted existing activity, id=\(existing.id)")
        }
    }

    /// Full, human-readable snapshot of ActivityKit state. Called from the
    /// engine right after a start attempt and once during startup, so the
    /// reason a Dynamic Island didn't appear is visible in the game log.
    @objc public static var diagnostics: String {
        onMain {
            var parts: [String] = []
            let osVersion = ProcessInfo.processInfo.operatingSystemVersion
            parts.append("iOS \(osVersion.majorVersion).\(osVersion.minorVersion).\(osVersion.patchVersion)")
            if #available(iOS 16.2, *) {
                adoptExistingIfNeeded()
                parts.append("available=yes")
                parts.append("activitiesEnabled=\(ActivityAuthorizationInfo().areActivitiesEnabled)")
                parts.append("storedActivity=\(activity == nil ? "nil" : "set")")
                parts.append("runningActivities=\(Activity<LiquudnetActivityAttributes>.activities.count)")
                parts.append("lastError=\(lastError)")
            } else {
                parts.append("available=no (needs iOS 16.2+)")
                parts.append("lastError=\(lastError)")
            }
            let pluginPath = Bundle.main.bundleURL
                .appendingPathComponent("PlugIns")
                .appendingPathComponent("LiquudnetWidgetExtension.appex")
            parts.append("widgetEmbedded=\(FileManager.default.fileExists(atPath: pluginPath.path))")
            let result = parts.joined(separator: ", ")
            log(result)
            return result
        }
    }

    /// Starts the Live Activity if one isn't already running. No-op if one
    /// is already active or on iOS < 16.2. Every refusal is logged (see
    /// `diagnostics`).
    @objc public static func start(withStatus status: String) {
        onMain {
            log("start requested, status=\(status), mainThread=\(Thread.isMainThread)")
            guard #available(iOS 16.2, *) else {
                lastError = "iOS 16.2+ required"
                log("start skipped: \(lastError)")
                return
            }
            adoptExistingIfNeeded()

            let state = LiquudnetActivityAttributes.ContentState(
                status: status,
                iconPng: Self.appIconPngData())
            let content = ActivityContent(state: state, staleDate: nil, relevanceScore: 0.0)

            if activity != nil {
                // An activity from an earlier process is already on screen:
                // refresh it instead of silently doing nothing.
                lastError = "none"
                log("start: adopting/resuming already running activity")
                Task {
                    for current in Activity<LiquudnetActivityAttributes>.activities {
                        await current.update(content)
                    }
                }
                return
            }

            // Do NOT gate on ActivityAuthorizationInfo().areActivitiesEnabled.
            // It can report false in some foreground states even when
            // Activity.request would succeed; let ActivityKit itself decide
            // and log the real error if the user disabled Live Activities.
            let authorization = ActivityAuthorizationInfo()
            log("activitiesEnabled=\(authorization.areActivitiesEnabled), attempting request anyway")
            do {
                let started = try Activity<LiquudnetActivityAttributes>.request(
                    attributes: LiquudnetActivityAttributes(),
                    content: content,
                    pushType: nil)
                activity = started
                lastError = "none"
                log("start OK, id=\(started.id)")
            } catch {
                lastError = "Activity.request failed: \(error.localizedDescription) (domain=\((error as NSError).domain), code=\((error as NSError).code))"
                activity = nil
                log("start failed: \(lastError)")
            }
        }
    }

    /// Updates the running Live Activity's status text. No-op if none is
    /// running.
    @objc public static func update(withStatus status: String) {
        onMain {
            guard #available(iOS 16.2, *) else {
                return
            }
            adoptExistingIfNeeded()
            guard let typedActivity = activity as? Activity<LiquudnetActivityAttributes> else {
                log("update skipped: no stored activity, starting one")
                start(withStatus: status)
                return
            }
            let state = LiquudnetActivityAttributes.ContentState(
                status: status,
                iconPng: Self.appIconPngData())
            let content = ActivityContent(state: state, staleDate: nil, relevanceScore: 0.0)
            Task {
                await typedActivity.update(content)
                log("update OK")
            }
        }
    }

    /// Ends the running Live Activity immediately. Also ends activities of
    /// this type left over from a previous process (e.g. one that was killed
    /// instead of quitting cleanly).
    @objc public static func end() {
        onMain {
            guard #available(iOS 16.2, *) else {
                return
            }
            activity = nil
            Task {
                let all = Activity<LiquudnetActivityAttributes>.activities
                log("end requested, found \(all.count) running activities")
                for current in all {
                    await current.end(nil, dismissalPolicy: .immediate)
                }
            }
        }
    }

    private static func appIconPngData() -> Data {
        guard let url = Bundle.main.url(forResource: "DDNet_256x256x32", withExtension: "png"),
              let data = try? Data(contentsOf: url),
              let image = UIImage(data: data) else
        {
            log("app icon PNG resource missing from app bundle")
            return Data()
        }
        // The bundled 256x256 icon is ~12 KB. ActivityKit content states are
        // meant to be tiny, and an oversized state is a plausible reason for
        // Activity.request to be rejected outright. The island renders this
        // at a few dozen points, so ship a small 48x48 thumbnail instead.
        let size = CGSize(width: 48, height: 48)
        let renderer = UIGraphicsImageRenderer(size: size)
        let thumbnail = renderer.image { _ in
            image.draw(in: CGRect(origin: .zero, size: size))
        }
        return thumbnail.pngData() ?? Data()
    }

    @objc public static var isRunning: Bool {
        onMain {
            guard #available(iOS 16.2, *) else {
                return false
            }
            adoptExistingIfNeeded()
            return activity is Activity<LiquudnetActivityAttributes>
        }
    }
}
