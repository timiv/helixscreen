# Copyright (c) 2025 Preston Brown <pbrown@brown-house.net>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixScreen UI Prototype - Tools Module
# Standalone diagnostic and utility programs

# ==============================================================================
# Moonraker Inspector Tool
# ==============================================================================
# Standalone diagnostic tool for querying Moonraker printer metadata
# Usage: moonraker-inspector <ip_address> [port]

# Tool sources
TOOLS_DIR := tools
INSPECTOR_SRC := $(TOOLS_DIR)/moonraker_inspector.cpp
INSPECTOR_INTERACTIVE_SRC := $(TOOLS_DIR)/moonraker_inspector_interactive.cpp
INSPECTOR_OBJ := $(OBJ_DIR)/tools/moonraker_inspector.o
INSPECTOR_INTERACTIVE_OBJ := $(OBJ_DIR)/tools/moonraker_inspector_interactive.o
INSPECTOR_STUB_OBJ := $(OBJ_DIR)/tools/ui_notification_stub.o

# Inspector needs moonraker_client and its dependencies, but NOT UI code
# Extract just the Moonraker-related objects (no LVGL, no UI, no SDL2)
INSPECTOR_DEPS := \
	$(OBJ_DIR)/moonraker_client.o \
	$(INSPECTOR_INTERACTIVE_OBJ) \
	$(INSPECTOR_STUB_OBJ)

# Simplified linker flags (no SDL2, no UI frameworks)
INSPECTOR_LDFLAGS := $(LIBHV_LIBS) -lm -lpthread

ifeq ($(UNAME_S),Darwin)
	INSPECTOR_LDFLAGS += -framework Foundation -framework CoreFoundation -framework Security
else
	INSPECTOR_LDFLAGS += -lssl -lcrypto -ldl
endif

# Build rule for inspector
$(MOONRAKER_INSPECTOR): $(LIBHV_LIB) $(INSPECTOR_OBJ) $(INSPECTOR_DEPS)
	$(Q)mkdir -p $(BIN_DIR)
	$(ECHO) "$(MAGENTA)$(BOLD)[LD]$(RESET) $@"
	$(Q)$(CXX) $(CXXFLAGS) $(INSPECTOR_OBJ) $(INSPECTOR_DEPS) -o $@ $(INSPECTOR_LDFLAGS) || { \
		echo "$(RED)$(BOLD)✗ Linking failed!$(RESET)"; \
		exit 1; \
	}
	$(ECHO) "$(GREEN)✓ Moonraker Inspector built: $@$(RESET)"

# Compile inspector source
$(INSPECTOR_OBJ): $(INSPECTOR_SRC) $(HEADERS) $(LIBHV_LIB)
	$(Q)mkdir -p $(dir $@)
	$(ECHO) "$(BLUE)[CXX]$(RESET) $<"
ifeq ($(V),1)
	$(Q)echo "$(YELLOW)Command:$(RESET) $(CXX) $(CXXFLAGS) $(INCLUDES) -I$(TOOLS_DIR) -c $< -o $@"
endif
	$(Q)$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(TOOLS_DIR) -c $< -o $@ || { \
		echo "$(RED)$(BOLD)✗ Compilation failed:$(RESET) $<"; \
		exit 1; \
	}

# Compile interactive module
$(INSPECTOR_INTERACTIVE_OBJ): $(INSPECTOR_INTERACTIVE_SRC) $(HEADERS) $(LIBHV_LIB)
	$(Q)mkdir -p $(dir $@)
	$(ECHO) "$(BLUE)[CXX]$(RESET) $<"
ifeq ($(V),1)
	$(Q)echo "$(YELLOW)Command:$(RESET) $(CXX) $(CXXFLAGS) $(INCLUDES) -I$(TOOLS_DIR) -c $< -o $@"
endif
	$(Q)$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(TOOLS_DIR) -c $< -o $@ || { \
		echo "$(RED)$(BOLD)✗ Compilation failed:$(RESET) $<"; \
		exit 1; \
	}

# Compile UI notification stubs (needed for moonraker_client linkage)
$(INSPECTOR_STUB_OBJ): $(TOOLS_DIR)/ui_notification_stub.cpp
	$(Q)mkdir -p $(dir $@)
	$(ECHO) "$(BLUE)[CXX]$(RESET) $<"
ifeq ($(V),1)
	$(Q)echo "$(YELLOW)Command:$(RESET) $(CXX) $(CXXFLAGS) -c $< -o $@"
endif
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@ || { \
		echo "$(RED)$(BOLD)✗ Compilation failed:$(RESET) $<"; \
		exit 1; \
	}

# Phony targets
.PHONY: tools moonraker-inspector

# Build all tools
tools: moonraker-inspector

# Individual tool targets
moonraker-inspector: $(MOONRAKER_INSPECTOR)
	$(ECHO) "$(CYAN)Usage: $(YELLOW)./$(MOONRAKER_INSPECTOR) <ip_address> [port] [options]$(RESET)"
	$(ECHO) "$(CYAN)Example: $(YELLOW)./$(MOONRAKER_INSPECTOR) 192.168.1.100 7125 -i$(RESET)"
	$(ECHO) "$(CYAN)Options: $(YELLOW)-i/--interactive$(RESET) for TUI mode with collapsible sections"
