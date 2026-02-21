# Panel Widget Decoupling Design

**Date**: 2026-02-21
**Status**: Approved
**Goal**: Decouple widgets from HomePanel so any panel can host them. Rename HomeWidget → PanelWidget.

## Problem

The widget system has a clean abstraction layer (HomeWidget base class, registry, factory pattern) but the orchestration is tightly coupled to HomePanel:

1. Factory registration happens in `HomePanel::init_subjects()`, capturing `this`
2. `populate_widgets()` is a HomePanel method
3. `active_widgets_` lifecycle is owned by HomePanel
4. Settings overlay hard-calls `get_global_home_panel().populate_widgets()`
5. NetworkWidget looks up subjects created by HomePanel
6. Gate observers are wired in HomePanel
7. Widget factories depend on HomePanel members (printer_state_, api_, temp_control_panel_)
8. Several widgets `extern` reference `get_global_home_panel()`

## Design

### Approach: PanelWidgetManager Singleton

A new singleton owns registry access, config, shared resources, and widget lifecycle. Panels become thin consumers.

### Rename Map

| Old | New |
|-----|-----|
| `HomeWidget` | `PanelWidget` |
| `HomeWidgetDef` | `PanelWidgetDef` |
| `HomeWidgetEntry` | `PanelWidgetEntry` |
| `HomeWidgetConfig` | `PanelWidgetConfig` |
| `home_widget.h` | `panel_widget.h` |
| `home_widget_registry.h/cpp` | `panel_widget_registry.h/cpp` |
| `home_widget_config.h/cpp` | `panel_widget_config.h/cpp` |
| `home_widget_from_event<T>()` | `panel_widget_from_event<T>()` |
| `src/ui/home_widgets/` | `src/ui/panel_widgets/` |
| XML: `home_widget_*.xml` | `panel_widget_*.xml` |

Individual widget class names (TemperatureWidget, PowerWidget, etc.) stay as-is.

### Widget Self-Registration

Each behavioral widget registers its own factory via static initializer. Dependencies come from singletons or the manager's shared resources — no panel capture needed.

```cpp
// In temperature_widget.cpp
static const bool registered = [] {
    helix::register_widget_factory("temperature", []() {
        return std::make_unique<helix::TemperatureWidget>(
            PrinterState::instance(),
            PanelWidgetManager::instance().shared_resource<TempControlPanel>());
    });
    return true;
}();
```

### PanelWidgetManager

```
PanelWidgetManager::instance()
├── Registry access (delegates to PanelWidgetRegistry)
├── Config: PanelWidgetConfig keyed by panel ID
├── Shared resources (TempControlPanel, etc.)
├── populate_widgets(panel_id, container)
│   → returns vector<unique_ptr<PanelWidget>>
├── setup_gate_observers(panel_id, rebuild_callback)
│   → watches hardware gate subjects, calls back on change
├── notify_config_changed(panel_id)
│   → triggers rebuild callback for that panel
└── init() / deinit() for lifecycle
```

**populate_widgets()**: Extracts today's `HomePanel::populate_widgets()` logic. Reads config for the panel ID, creates XML components, instantiates factories, calls `attach()`. Returns ownership to the caller.

**Shared resources**: Manager holds shared objects (like TempControlPanel) that multiple widgets need. Replaces the pattern of HomePanel passing its members into factories.

**Gate observers**: Manager watches hardware gate subjects per-panel and triggers rebuild callbacks when hardware availability changes.

### Panel Integration

Panels become thin consumers (~5 lines of widget code):

```cpp
// In any panel's setup()
auto& mgr = PanelWidgetManager::instance();
active_widgets_ = mgr.populate_widgets("home", widget_container_);
mgr.setup_gate_observers("home", [this]() {
    active_widgets_ = mgr.populate_widgets("home", widget_container_);
});
```

No factory registration, no gate observer wiring, no XML creation logic in the panel.

### Settings Overlay

Stops calling `get_global_home_panel().populate_widgets()`. Instead:

```cpp
PanelWidgetManager::instance().notify_config_changed("home");
```

Manager holds callbacks per panel ID. Settings overlay doesn't need to know about any specific panel. UX stays the same for now — only shows home widgets. Parameterize with panel ID later when adding widgets to other panels.

### Per-Panel Config

JSON structure:

```json
{
  "panel_widgets": {
    "home": [
      {"id": "power", "enabled": true, "config": {}},
      {"id": "temperature", "enabled": true, "config": {}}
    ]
  }
}
```

Backward compatible: on first load, migrates `"home_widgets"` → `"panel_widgets": {"home": [...]}`.

### Subject Ownership

Widgets own their own subjects. NetworkWidget's `home_network_icon_state` and `network_label` subjects move from HomePanel into NetworkWidget itself, created in `attach()` and cleaned up in `detach()`.

### parent_screen

Not passed through the API. Widgets that need to create overlays call `lv_scr_act()` internally.

## What Doesn't Change

- Pure XML widgets (filament, probe, humidity, etc.) — just renamed files
- The `attach()`/`detach()` lifecycle contract
- Hardware gating logic — same mechanism, lives in manager instead of HomePanel
- Widget XML components — renamed from `home_widget_*.xml` to `panel_widget_*.xml`

## Files Affected

### New Files
- `include/panel_widget_manager.h`
- `src/ui/panel_widget_manager.cpp`

### Renamed Files (move + rename)
- `include/home_widget.h` → `include/panel_widget.h`
- `include/home_widget_registry.h` → `include/panel_widget_registry.h`
- `include/home_widget_config.h` → `include/panel_widget_config.h`
- `src/ui/home_widget_registry.cpp` → `src/ui/panel_widget_registry.cpp`
- `src/ui/home_widget_config.cpp` → `src/ui/panel_widget_config.cpp`
- `src/ui/home_widgets/*.h/cpp` → `src/ui/panel_widgets/*.h/cpp`
- `ui_xml/components/home_widget_*.xml` → `ui_xml/components/panel_widget_*.xml`

### Modified Files
- `src/ui/panels/ui_panel_home.cpp` — remove ~200 lines of widget orchestration, replace with manager calls
- `include/ui_panel_home.h` — remove widget-related members
- `src/ui/overlays/ui_settings_home_widgets.cpp` — call manager instead of HomePanel
- All widget implementations — self-registration, own subjects, use `lv_scr_act()`
- Any file that `#include`s renamed headers
