# Declarative UI Refactoring

This document tracks the ongoing effort to convert imperative LVGL patterns to declarative XML bindings.

## Completed Work

### Phase 2: State Machine Visibility (Complete)

Converted all calibration wizard panels from imperative `show_state_view()` functions to declarative XML subject bindings.

| Panel | File | States | Pattern |
|-------|------|--------|---------|
| PID Tuning | `calibration_pid_panel.xml` | IDLE(0), CALIBRATING(1), SAVING(2), COMPLETE(3), ERROR(4) | `pid_cal_state` subject |
| Input Shaper | `input_shaper_panel.xml` | IDLE(0), MEASURING(1), RESULTS(2), ERROR(3) | `input_shaper_state` subject |
| Screws Tilt | `screws_tilt_panel.xml` | IDLE(0), PROBING(1), RESULTS(2), LEVELED(3), ERROR(4) | `screws_tilt_state` subject |
| Z-Offset | `calibration_zoffset_panel.xml` | IDLE(0), PROBING(1), ADJUSTING(2), SAVING(3), COMPLETE(4), ERROR(5) | `zoffset_cal_state` subject |

**Pattern established:**

```xml
<!-- XML: Each state view declares its visibility condition -->
<lv_obj name="state_idle">
  <lv_obj-bind_flag_if_not_eq subject="panel_state" flag="hidden" ref_value="0"/>
  <!-- content -->
</lv_obj>
<lv_obj name="state_probing">
  <lv_obj-bind_flag_if_not_eq subject="panel_state" flag="hidden" ref_value="1"/>
  <!-- content -->
</lv_obj>
```

```cpp
// C++: Static subject registered before XML creation
static lv_subject_t s_panel_state;

void Panel::init_subjects() {
    lv_subject_init_int(&s_panel_state, 0);
    lv_xml_register_subject(nullptr, "panel_state", &s_panel_state);
}

// State changes just update the subject - UI updates automatically
void Panel::set_state(State new_state) {
    state_ = new_state;
    lv_subject_set_int(&s_panel_state, static_cast<int>(new_state));
}
```

**Benefits:**
- Eliminated ~24 imperative visibility calls
- Reduced ~120 lines of code
- Removed 6 state view member variables per panel
- XML documents the state machine structure visually
- Impossible to forget to hide a view on state change

---

## Remaining Work

### Remaining Visibility Patterns (~152 calls)

These are **appropriately imperative** and should NOT be converted:

| Pattern | Example | Why Imperative |
|---------|---------|----------------|
| Object pooling | `ui_panel_print_select.cpp` | Performance: recycling views as user scrolls |
| Navigation | `ui_panel_controls.cpp` | Responds to nav stack push/pop events |
| Modal management | Various | Dynamic show/hide based on user actions |
| Conditional UI | Sort direction icons | Complex boolean logic |

### Phase 1: Event Callbacks (~187 remaining `lv_obj_add_event_cb`)

**Partially convertible.** Some panels still use the old pattern:

```cpp
// Old pattern (imperative)
lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, user_data);
```

```xml
<!-- New pattern (declarative) -->
<lv_button>
  <event_cb trigger="clicked" callback="callback_name"/>
</lv_button>
```

**Convertible:** Callbacks that use a global panel accessor (e.g., `get_global_*_panel()`)

**Not easily convertible:** Callbacks that pass context via `user_data` (e.g., Z-Offset's adjustment buttons passing delta values)

**Files with most remaining `lv_obj_add_event_cb` calls:**
- `ui_panel_print_select.cpp` - Uses user_data for file context
- `ui_panel_controls.cpp` - Uses user_data for sub-panel context
- `ui_panel_calibration_zoffset.cpp` - Uses user_data for delta values
- `ui_keyboard.cpp` - Complex keyboard handling

### Phase 3: Label Text Bindings (~136 `lv_label_set_text` calls)

**Partially convertible.** Some labels update from a single data source:

```cpp
// Old pattern
lv_label_set_text(label, "Z: 0.123");
```

```xml
<!-- New pattern -->
<lv_label bind_text="z_position_subject"/>
```

**Convertible:** Labels bound to a single subject (temperatures, positions, counts)

**Not easily convertible:**
- Labels with complex formatting (`snprintf` with multiple values)
- Labels that combine static and dynamic text
- Labels updated from multiple data sources

**Candidates for conversion:**
- Temperature displays (already using subjects in some panels)
- Position displays
- Status text that maps directly to state

---

## Guidelines for Future Conversions

### When to Convert to Declarative

1. **State machines** - Discrete states with exclusive visibility (IDLE/ACTIVE/ERROR)
2. **Simple data bindings** - One subject → one label with no formatting
3. **Boolean visibility** - Show/hide based on single boolean condition

### When to Keep Imperative

1. **Object pooling** - Dynamic recycling for performance
2. **Complex formatting** - `snprintf` with multiple values
3. **User data context** - Callbacks needing object-specific data
4. **Navigation state** - Responds to nav stack changes
5. **Conditional logic** - Multiple conditions determining visibility

### Conversion Checklist

- [ ] Add static `lv_subject_t` for the binding
- [ ] Register subject in `init_subjects()` BEFORE XML creation
- [ ] Add XML binding element as first child of target widget
- [ ] Remove imperative code (member variables, update functions)
- [ ] Test all states/values work correctly
- [ ] Commit with clear message describing the pattern change
