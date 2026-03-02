# v1.1.1
- Updated Geode target version to v5.3.0.
- Removed CustomJump's use of the shared keybind-setting event pipeline.
- Switched CustomJump input handling to raw Geode keyboard and mouse input events only.
- Fixed CustomJump interfering with other mods' keybinds while a custom jump input is held.

# v1.1.0
- Fixed remaining custom input registration gaps in editor playtest.
- Improved editor playtest state tracking using playtest start/stop lifecycle hooks.
- Improved pause-state filtering during editor playtest input handling.

# v1.0.9
- Updated Geode target version to v5.0.1.
- Fixed intermittent custom input registration issues during gameplay.
- Fixed custom input not registering in editor playtest.
- Improved key/mouse press-release handling to prevent missed input transitions.

# v1.0.8
- Updated for Geode v5.0.0 stable.
- Migrated metadata from beta.4 target to stable loader target.
- Removed beta-only migration labels from source comments.
- Revalidated custom jump input flow for gameplay and editor playtest on GD 2.2081.

# v1.0.7
- Updated for Geode v5.0.0-beta.4.
- Updated metadata for beta.4 compatibility (`requires-patching`, full platform targets).
- Revalidated keybind event handling for beta.4 event recursion/race-condition fixes.
- Revalidated input stability in gameplay and editor playtest for both Player 1 and Player 2.

# v1.0.6
- Updated for Geode v5.0.0-beta.3.
- Migrated to the beta.3 keybind category system (`category` + keybind `priority` metadata).
- Implemented explicit keybind listener priority handling and optional stop-propagation behavior.
- Improved input handling compatibility using keybind event timestamps and modifier-aware keybind matching.
- Added custom jump support for `LevelEditorLayer` playtest sessions.
- Added playtest-only context gating so editor tools and menus are not affected.
- Kept default jump controls additive while custom keybinds are active.

# v1.0.5
- Added a separate custom jump keybind for Player 2 (`Jump Key (Player 2)`).
- Added Player 2 custom jump input processing during active gameplay.
- Kept default player inputs untouched while adding custom key support for both players.
- Added `Enable Player 2 Custom Key` toggle setting.
- Added runtime gating for Player 2 custom jump input based on the toggle.
- Kept backward-compatible behavior by defaulting Player 2 custom input to enabled when the toggle setting is missing.

# v1.0.4
- Fixed custom jump hold behavior after death and respawn.
- Improved jump key state handling so held inputs re-register correctly in gameplay.
- Kept default jump keys additive while custom keybind remains active.
- Cleaned CI logs by suppressing third-party `fmt` deprecated locale warnings on Clang builds.

# v1.0.0
- Initial release of CustomJump.
- Added `Jump Key` keybind setting for custom jump input.
- Implemented gameplay-only input handling for active `PlayLayer` sessions.
- Kept vanilla jump keys active while adding custom-key support.
- Added Geode `5.0.0-beta.2` and Geometry Dash `2.2081` compatibility metadata.
