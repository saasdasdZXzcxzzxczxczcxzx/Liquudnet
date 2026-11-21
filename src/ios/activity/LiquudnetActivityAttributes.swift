import ActivityKit

// Shared between the main app and LiquudnetWidgetExtension (see
// CMakeLists.txt: this file is listed as a source of both targets).
// ActivityKit matches an Activity to the Widget Extension's UI by this
// type's name and Codable shape, not by the two sides sharing a literal
// compiled module - compiling the same source file into both targets is
// the pattern Apple's own docs describe for sharing attributes.
//
// First pass only (see the Liquudnet settings tab and Task #12): a single
// placeholder status string, not yet wired to any real in-game state like
// a race timer. Intentionally minimal so the Widget Extension target and
// its Info.plist/embedding setup can be proven out on their own first.
@available(iOS 16.1, *)
struct LiquudnetActivityAttributes: ActivityAttributes {
    public struct ContentState: Codable, Hashable {
        var status: String
        // PNG bytes of the app icon, captured on the app side by
        // LiquudnetActivityBridge (UIApplication.shared.applicationIconImage)
        // and rendered by the widget as the Dynamic Island badge. Kept as
        // raw Data rather than an asset name so the widget extension does
        // not need its own copy of the app-icon asset catalog.
        var iconPng: Data
    }
}
