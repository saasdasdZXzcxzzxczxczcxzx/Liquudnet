import ActivityKit
import SwiftUI
import UIKit
import WidgetKit

// The Dynamic Island / Lock Screen presentation for
// LiquudnetActivityAttributes (see LiquudnetActivityAttributes.swift,
// shared with the main app). This is the real, system-rendered UI: the
// engine itself can never draw into the Dynamic Island or Lock Screen, the
// same reason the native Liquid Glass button needs a real UIKit view (see
// IGraphics::SetNativeGlassButton) rather than anything the engine draws.
//
// The leading/minimal badge is the app icon: the bridge captures its PNG
// bytes on the app side into ContentState.iconPng, so the widget extension
// does not need its own copy of the app-icon asset catalog.
struct LiquudnetLiveActivityWidget: Widget {
    var body: some WidgetConfiguration {
        ActivityConfiguration(for: LiquudnetActivityAttributes.self) { context in
            HStack {
                LiquudnetIcon(data: context.state.iconPng)
                Text("Liquudnet")
                    .font(.headline)
                Spacer()
                Text(context.state.status)
            }
            .padding()
        } dynamicIsland: { context in
            DynamicIsland {
                DynamicIslandExpandedRegion(.leading) {
                    LiquudnetIcon(data: context.state.iconPng)
                }
                DynamicIslandExpandedRegion(.center) {
                    Text(context.state.status)
                }
            } compactLeading: {
                LiquudnetIcon(data: context.state.iconPng)
            } compactTrailing: {
                Text(context.state.status.prefix(3))
            } minimal: {
                LiquudnetIcon(data: context.state.iconPng)
            }
        }
    }
}

private struct LiquudnetIcon: View {
    let data: Data

    var body: some View {
        Group {
            if !data.isEmpty, let uiImage = UIImage(data: data) {
                Image(uiImage: uiImage)
                    .resizable()
                    .scaledToFit()
            } else {
                // Fallback if the bridge couldn't capture the icon.
                Image(systemName: "gamecontroller.fill")
                    .resizable()
                    .scaledToFit()
                    .foregroundStyle(.white)
            }
        }
        .frame(width: 18, height: 18)
        .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
    }
}
