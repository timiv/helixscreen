# Temperature Graph Widget Usage

## Overview

The `ui_temp_graph` widget provides a dynamic, multi-series line chart for real-time temperature monitoring with gradient-filled areas under curves and target temperature indicators.

## Key Features

- **Dynamic series management**: Add/remove series at runtime (up to 8 concurrent)
- **Gradient fills**: Vertical gradients under curves (darker at bottom, lighter at top)
- **Target temperature lines**: Horizontal cursors showing target temps
- **Multiple update modes**: Push single values, set array data, or poll
- **Configurable**: Adjustable point count, temperature range, gradients

## Basic Usage

```cpp
#include "ui_temp_graph.h"

// 1. Create the graph widget
ui_temp_graph_t* graph = ui_temp_graph_create(parent_container);

// 2. Add temperature series
int nozzle_id = ui_temp_graph_add_series(graph, "Nozzle", lv_color_hex(0xFF4444)); // Red
int bed_id = ui_temp_graph_add_series(graph, "Bed", lv_color_hex(0x44FF44));    // Green

// 3. Set target temperatures (optional)
ui_temp_graph_set_series_target(graph, nozzle_id, 210.0f, true); // Show 210°C target
ui_temp_graph_set_series_target(graph, bed_id, 60.0f, true);     // Show 60°C target

// 4. Update temperatures (push mode - one value at a time)
ui_temp_graph_update_series(graph, nozzle_id, 195.5f);
ui_temp_graph_update_series(graph, bed_id, 58.2f);

// 5. Cleanup when done
ui_temp_graph_destroy(graph);
```

## Advanced Usage

### Array Mode (Set All Points)

```cpp
// Replace entire series with historical data
float nozzle_temps[300];
// ... fill array with temperature history ...
ui_temp_graph_set_series_data(graph, nozzle_id, nozzle_temps, 300);
```

### Configuration

```cpp
// Set temperature range (Y-axis)
ui_temp_graph_set_temp_range(graph, 0.0f, 300.0f);

// Set point capacity (e.g., 600 points = 10 min @ 1s)
ui_temp_graph_set_point_count(graph, 600);

// Customize gradient opacity
ui_temp_graph_set_series_gradient(graph, nozzle_id,
                                   LV_OPA_80,  // Bottom: 80% opacity
                                   LV_OPA_20); // Top: 20% opacity
```

### Show/Hide Series

```cpp
// Temporarily hide a series (keeps data)
ui_temp_graph_show_series(graph, bed_id, false);

// Show it again
ui_temp_graph_show_series(graph, bed_id, true);
```

### Remove Series

```cpp
// Permanently remove a series
ui_temp_graph_remove_series(graph, bed_id);
```

### Clear Data

```cpp
// Clear all series data
ui_temp_graph_clear(graph);

// Or clear just one series
ui_temp_graph_clear_series(graph, nozzle_id);
```

## Example: Complete Temperature Monitor

```cpp
// Create graph with custom styling
ui_temp_graph_t* graph = ui_temp_graph_create(panel);
lv_obj_t* chart = ui_temp_graph_get_chart(graph);
lv_obj_set_size(chart, lv_pct(100), 300);

// Configure temperature range
ui_temp_graph_set_temp_range(graph, 0.0f, 350.0f);

// Add all heaters
int nozzle = ui_temp_graph_add_series(graph, "Nozzle", lv_color_hex(0xFF4444));
int bed = ui_temp_graph_add_series(graph, "Bed", lv_color_hex(0x44FF44));
int chamber = ui_temp_graph_add_series(graph, "Chamber", lv_color_hex(0x4444FF));

// Set targets
ui_temp_graph_set_series_target(graph, nozzle, 210.0f, true);
ui_temp_graph_set_series_target(graph, bed, 60.0f, true);
ui_temp_graph_set_series_target(graph, chamber, 40.0f, true);

// Simulate temperature updates (in your update loop)
void on_temperature_update(float nozzle_temp, float bed_temp, float chamber_temp) {
    ui_temp_graph_update_series(graph, nozzle, nozzle_temp);
    ui_temp_graph_update_series(graph, bed, bed_temp);
    ui_temp_graph_update_series(graph, chamber, chamber_temp);
}
```

## Integration with Moonraker

```cpp
// In your Moonraker temperature update handler:
void handle_temperature_update(const json& data) {
    // Extract temperatures from Moonraker response
    float extruder_temp = data["extruder"]["temperature"];
    float heater_bed_temp = data["heater_bed"]["temperature"];

    // Update graph
    ui_temp_graph_update_series(temp_graph, extruder_series_id, extruder_temp);
    ui_temp_graph_update_series(temp_graph, bed_series_id, heater_bed_temp);
}

// When target changes:
void handle_target_temp_change(int heater_id, float new_target) {
    ui_temp_graph_set_series_target(temp_graph, heater_id, new_target, true);
}
```

## Color Suggestions

```cpp
// Recommended colors for common heaters (based on Creality style)
#define TEMP_COLOR_NOZZLE    lv_color_hex(0xFF4444)  // Red
#define TEMP_COLOR_BED       lv_color_hex(0x44FF44)  // Green
#define TEMP_COLOR_CHAMBER   lv_color_hex(0x4444FF)  // Blue
#define TEMP_COLOR_AMBIENT   lv_color_hex(0xFFAA44)  // Orange

// Teal/cyan gradient (like Creality screenshots)
lv_color_hex(0x00CED1)  // Dark turquoise
lv_color_hex(0x20B2AA)  // Light sea green
```

## Performance Notes

- **Default capacity**: 300 points = 5 minutes @ 1 second updates
- **Memory**: ~2.4KB per series (300 points × 4 bytes × 2 arrays)
- **Update cost**: O(1) for `update_series()` (uses circular buffer)
- **Gradient rendering**: Simplified rectangle fill (minimal overhead)

## API Reference

See `/Users/pbrown/code/guppyscreen/prototype-ui9/include/ui_temp_graph.h` for complete API documentation.

## Files

- **Header**: `include/ui_temp_graph.h`
- **Implementation**: `src/ui_temp_graph.cpp`
- **This Guide**: `TEMP_GRAPH_USAGE.md`
