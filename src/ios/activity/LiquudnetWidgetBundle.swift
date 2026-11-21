import SwiftUI
import WidgetKit

// Entry point for the LiquudnetWidgetExtension app extension target (see
// CMakeLists.txt). A WidgetBundle can list more than one Widget; for now
// the Dynamic Island / Lock Screen Live Activity is the only one.
@main
struct LiquudnetWidgetBundle: WidgetBundle {
    var body: some Widget {
        LiquudnetLiveActivityWidget()
    }
}
