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
