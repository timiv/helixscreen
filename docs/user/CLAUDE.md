# docs/user/CLAUDE.md — User Documentation

These docs are **end-user facing**. They must be written for people who are NOT developers — clear language, no implementation details, no source code references.

## Style Rules

- Write for someone who just bought a Raspberry Pi and a touchscreen
- Use step-by-step instructions with exact commands
- Screenshots are better than descriptions
- Never reference source files, class names, or internal architecture
- Config examples should be copy-pasteable
- When mentioning settings, show the exact path in the UI (e.g., "Settings > Advanced > Beta Features")
- Test all instructions on a clean install before publishing

## User Docs Index

| Doc | Contents |
|-----|----------|
| `INSTALL.md` | Installation guide for all platforms |
| `USER_GUIDE.md` | Complete usage guide, all features |
| `CONFIGURATION.md` | All settings explained with examples |
| `TROUBLESHOOTING.md` | Common problems and solutions |
| `FAQ.md` | Quick answers to common questions |
| `UPGRADING.md` | Version upgrade instructions |
| `PLUGIN_DEVELOPMENT.md` | Plugin creation guide (power users) |
| `TESTING_INSTALLATION.md` | Post-install verification steps |
| `TELEMETRY.md` | What telemetry collects, privacy controls, opt-in/out |
| `PRIVACY_POLICY.md` | Privacy policy for telemetry data |

## When Updating User Docs

- New features: Add to `USER_GUIDE.md` under the appropriate section
- New settings: Add to `CONFIGURATION.md`
- New install methods/platforms: Add to `INSTALL.md`
- Known issues: Add to `TROUBLESHOOTING.md`
- After every release: Review all user docs for accuracy
