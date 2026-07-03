# CS2RoundAlert

[中文说明](README.zh-CN.md)

A small Windows tray app for Counter-Strike 2. It reminds you when a new round starts after you tab out.

It does not control the game. It only receives official CS2 GSI data on your own computer.

Portable app: no installation is required. Report issues or suggestions on GitHub.

## Download

Download `CS2RoundAlert.exe` here:

<https://github.com/BannerLord54/CS2RoundAlert/releases/latest>

Download the `.exe` file, not `Source code`.

## How to use

1. Run `CS2RoundAlert.exe`.
2. If Windows shows a warning, choose `More info` -> `Run anyway`.
3. The app finds CS2 and installs the GSI config automatically.
4. Use the small window to test the sound and turn alerts on or off.
5. Optional: enable `Also alert when a round ends`.
6. Start or restart CS2.
7. Keep CS2RoundAlert running in the system tray.

Only one instance runs at a time. Opening the exe again brings the existing window forward.

That is all.

When the next round begins:

- if CS2 is already focused, nothing happens
- if you are in another window, a sound plays

If `Also alert when a round ends` is enabled, the app also plays a sound when `round.phase` becomes `over`. This alert can play while CS2 is focused.

## First run

On first run, the app tries to find CS2 from Steam and installs a CS2 Game State Integration config file automatically.

If auto-detection fails, right-click the tray icon and choose `Choose CS2 cfg folder`:

```text
Counter-Strike Global Offensive\game\csgo\cfg
```

After this, restart CS2 once.

## Tray menu

Right-click the tray icon:

- `Enable alerts`: turn alerts on or off
- `Also alert when a round ends`: optional round-end sound
- `Test sound`: play the alert sound once
- `Open settings folder`: open app settings
- `Open GitHub repo`: open this page
- `Choose CS2 cfg folder`: manually select CS2 if auto-detect fails
- `Language`: choose Auto, English, or Chinese
- `Quit`: exit the app

Default language is Auto.

Closing the window hides it to the tray. Use `Quit` to exit.

## Is this safe for CS2?

CS2RoundAlert only uses Valve's official Game State Integration feature.

It does not:

- read game memory
- inject code
- simulate mouse or keyboard input
- automate gameplay

It only receives round-state JSON from CS2 on your own computer.

Valve GSI documentation:

<https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration>

## Common problems

### No sound

Try this:

1. Click `Test sound` in the app. If that is silent, check Windows sound settings.
2. Make sure CS2RoundAlert is still running in the tray.
3. Make sure alerts are enabled.
4. Restart CS2 after the app installs the GSI config.
5. Tab out of CS2. The app skips the sound while CS2 is the focused window.

The app window shows whether it has received GSI data from CS2. If it keeps saying `no CS2 data received yet`, CS2 has not loaded the GSI config.

### Windows warning

The app is not code-signed, so Windows SmartScreen or antivirus software may show a warning.

To make the release easier to check:

- the source code is public
- the exe is built by GitHub Actions
- the release includes a SHA256 checksum
- the app is not packed, obfuscated, or compressed

If your antivirus deletes it, do not disable your antivirus blindly. Check the source, the release workflow, and the checksum first.

## Settings

Settings are stored here:

```text
%AppData%\CS2RoundAlert\settings.json
```

Most users do not need to edit this file.

## For developers

Build with MSYS2 MinGW64:

```powershell
C:\msys64\usr\bin\bash.exe -lc 'bash /d/Path/To/CS2RoundAlert/scripts/build-native-msys2.sh'
```

## License

MIT
