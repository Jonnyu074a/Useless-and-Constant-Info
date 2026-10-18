# Useless And Constant info

A fully customizable, real-time **HUD overlay** for Geometry Dash.

Open the settings with the **stats icon** next to the Daily Chest on the main menu.

## What it can show

- **FPS / TPS** (real physics ticks inside levels), with optional target refresh rate
- **CPS** (clicks per second) and **accuracy** (noclip aware)
- **Level info**: name, author, ID, attempts, best %, current %, practice mode
- **Deaths** (whole session or per level)
- **Time, date, session timer and total play time**
- **GD / Geode version, mod count, OS, username**
- The **mod menu** you have installed (MegaHack, OpenHack...)
- The **screen you are on**, including other mods' menus

## Features

- Settings window with info popups on every option
- **Drag to reposition** the overlay anywhere (position is saved)
- **Label Builder**: pick tags, separator and presets with a live preview
- Fonts, scale, colors, translucent background, horizontal or vertical layout
- FPS color coding, hide on enter / leave level

## Label tags

`{fps}` `{tps}` `{cps}` `{gd}` `{geode}` `{mods}` `{os}` `{user}` `{mh}` `{date}` `{time}`
`{session}` `{session_total}` `{lvl}` `{lvlid}` `{att}` `{best}` `{progress}` `{practice}`
`{accuracy}` `{deaths}`

The separator splits the format into parts; a part whose tags are all empty is hidden automatically.

*Supported platforms: Windows and Android.*
