# HelixScreen Documentation

Welcome to the HelixScreen documentation. Choose your path:

---

## For Users

**Installing and using HelixScreen with pre-built packages.**

| Document | Description |
|----------|-------------|
| [**Installation Guide**](user/INSTALL.md) | Get HelixScreen running on your display |
| [**User Guide**](user/USER_GUIDE.md) | Learn how to use the interface |
| [**Configuration**](user/CONFIGURATION.md) | All settings explained |
| [**Troubleshooting**](user/TROUBLESHOOTING.md) | Solutions to common problems |
| [**FAQ**](user/FAQ.md) | Quick answers to common questions |

---

## For Developers

**Building from source and contributing to HelixScreen.**

| Document | Description |
|----------|-------------|
| [**Development Guide**](DEVELOPMENT.md) | Build system and daily workflow |
| [**Architecture**](ARCHITECTURE.md) | System design and patterns |
| [**Contributing**](CONTRIBUTING.md) | Code standards and PR process |
| [**Build System**](BUILD_SYSTEM.md) | Makefile, cross-compilation, patches |
| [**Testing**](TESTING.md) | Test infrastructure and Catch2 usage |

---

## Technical Reference

**API documentation and implementation details.**

| Document | Description |
|----------|-------------|
| [**LVGL 9 XML Guide**](LVGL9_XML_GUIDE.md) | Complete XML syntax reference (92K) |
| [**Quick Reference**](QUICK_REFERENCE.md) | Common patterns and code snippets |
| [**Moonraker Architecture**](MOONRAKER_ARCHITECTURE.md) | Moonraker integration details |
| [**Logging Guidelines**](LOGGING.md) | Log levels and message format |
| [**Copyright Headers**](COPYRIGHT_HEADERS.md) | SPDX license requirements |

---

## Planning & Status

**Project roadmap and feature tracking.**

| Document | Description |
|----------|-------------|
| [**Roadmap**](ROADMAP.md) | Feature timeline and milestones |
| [**AMS Implementation Plan**](AMS_IMPLEMENTATION_PLAN.md) | Multi-material support spec |

---

## Audits & Reports

**Security reviews and quality assessments.**

| Document | Description |
|----------|-------------|
| [**Security Review**](audits/MOONRAKER_SECURITY_REVIEW.md) | Moonraker security assessment |
| [**Memory Analysis**](audits/MEMORY_ANALYSIS.md) | Memory profiling and optimization |
| [**Test Coverage**](audits/TEST_COVERAGE_REPORT.md) | Test coverage report |

---

## Documentation Map

```
docs/
├── README.md                 # This file - documentation index
│
├── user/                     # END-USER DOCUMENTATION
│   ├── README.md             # User docs index
│   ├── INSTALL.md            # Installation guide
│   ├── USER_GUIDE.md         # How to use HelixScreen
│   ├── CONFIGURATION.md      # Settings reference
│   ├── TROUBLESHOOTING.md    # Common problems
│   └── FAQ.md                # Frequently asked questions
│
├── DEVELOPMENT.md            # Developer setup and workflow
├── ARCHITECTURE.md           # System design
├── CONTRIBUTING.md           # Code standards
├── BUILD_SYSTEM.md           # Build internals
├── TESTING.md                # Test infrastructure
│
├── LVGL9_XML_GUIDE.md        # XML reference
├── QUICK_REFERENCE.md        # Code patterns
├── MOONRAKER_ARCHITECTURE.md # Moonraker integration
├── LOGGING.md                # Logging standards
│
├── ROADMAP.md                # Feature timeline
│
├── audits/                   # Security & quality
├── archive/                  # Historical docs
└── images/                   # Screenshots
```

---

## Quick Start

**I want to...**

| Goal | Start Here |
|------|------------|
| Install HelixScreen | [Installation Guide](user/INSTALL.md) |
| Use HelixScreen | [User Guide](user/USER_GUIDE.md) |
| Build from source | [Development Guide](DEVELOPMENT.md) |
| Contribute code | [Contributing Guide](CONTRIBUTING.md) |
| Create XML layouts | [LVGL 9 XML Guide](LVGL9_XML_GUIDE.md) |
| Understand the architecture | [Architecture Guide](ARCHITECTURE.md) |
| Cross-compile for Pi | [Build System - Cross-Compilation](BUILD_SYSTEM.md#cross-compilation-embedded-targets) |

---

*Back to [Project README](../README.md)*
