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
