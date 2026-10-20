# Changelog

## v5.2.1

### Fixes
- Fixed "Invalid Geode File" (missing `info` in the `issues` block of `mod.json`)
- Repository links now point to the real repository

## v5.2.0

### Fixes
- **Crash when the game lost focus** (Android): removed the `AppDelegate` hook that caused it
- Selecting an option no longer scrolls the settings window back to the top
- Colors are no longer drawn on top of the button text
- The HUD button no longer shows text under its icon
- The "current screen" label no longer resets to *Menu*; it now detects the real screen, including popups and menus from other mods
- Settings window fits the screen and no longer lets touches through to the menu behind it
- Settings are applied and saved instantly; saving can no longer overwrite pending changes
- Drag-to-reposition keeps the selected corner and persists
- CPS counts jumps again, accuracy is noclip aware, deaths are no longer double counted
- Session timer no longer resets when focus is lost
- Mod menu detection is local only (no network requests)
- Names containing `{}` or separators no longer break the label

### New
- Redesigned settings window with icons, panels and info popups
- Real color picker
- Real TPS inside levels
- Mod page now has its own description, changelog and logo

### Removed
- macOS and iOS support (Windows + Android only)

## v5.1.0
- Performance improvements and internal cleanup
