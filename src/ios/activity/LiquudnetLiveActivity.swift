import ActivityKit
import SwiftUI
import WidgetKit

// The Dynamic Island / Lock Screen presentation for
// LiquudnetActivityAttributes (see LiquudnetActivityAttributes.swift,
// shared with the main app). This is the real, system-rendered UI: the
// engine itself can never draw into the Dynamic Island or Lock Screen, the
// same reason the native Liquid Glass button needs a real UIKit view (see
// IGraphics::SetNativeGlassButton) rather than anything the engine draws.
//
// First pass only: a plain status line in every presentation, not yet a
// real race-timer/HUD-style layout. The point of this pass is to prove the
// Widget Extension target itself - building, embedding into the app
// bundle, Info.plist wiring - works before investing in real layout work
// or wiring it up to actual game state from the C++ engine (next step,
// not done yet).
struct LiquudnetLiveActivityWidget: Widget {
    var body: some WidgetConfiguration {
        ActivityConfiguration(for: LiquudnetActivityAttributes.self) { context in
            HStack {
                Text("Liquudnet")
                    .font(.headline)
                Spacer()
                Text(context.state.status)
            }
            .padding()
        } dynamicIsland: { context in
            DynamicIsland {
                DynamicIslandExpandedRegion(.center) {
                    Text(context.state.status)
                }
            } compactLeading: {
                Text("L")
            } compactTrailing: {
                Text(context.state.status.prefix(3))
            } minimal: {
                Text("L")
            }
        }
    }
}
