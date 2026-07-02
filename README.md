# CS2RoundAlert

CS2RoundAlert is a small Windows tray utility for Counter-Strike 2.

It listens for CS2 Game State Integration (GSI) events on `127.0.0.1`, watches `round.phase`, and plays an alert sound when a round enters `freezetime`. If CS2 is already the foreground window, the alert is skipped by default.

## Why this is designed to be safe

CS2RoundAlert uses Valve's official Game State Integration mechanism:

<https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration>

The tool:

- does not read CS2 memory
- does not inject code
- does not hook the game process
- does not simulate mouse or keyboard input
- only receives JSON that CS2 sends to a local HTTP endpoint

This avoids the behavior normally associated with cheats or automation tools. No third-party tool can guarantee future anti-cheat policy decisions, so review the source and use it at your own risk.

## Features

- Windows system tray app, no console window
- Local GSI HTTP listener using `System.Net.HttpListener`
- Default endpoint: `http://127.0.0.1:3000`
- Alerts on transition into `round.phase = freezetime`
- Skips alerts when `cs2.exe` is the foreground window
- Automatically writes the CS2 GSI config file
- Falls back to a folder picker if CS2 is not auto-detected
- JSON settings in `%AppData%\CS2RoundAlert\settings.json`
- Built with .NET 8 and WinForms

## Install

1. Download `CS2RoundAlert.exe` from the latest GitHub Release.
2. Run it.
3. The app appears in the Windows system tray.
4. On first run, it installs:

```text
gamestate_integration_cs2roundalert.cfg
```

into:

```text
Counter-Strike Global Offensive\game\csgo\cfg
```

If the app cannot find CS2 automatically, select the CS2 `cfg` folder when prompted.

Restart CS2 after the config file is installed.

## Usage

Keep CS2RoundAlert running in the tray.

When CS2 sends a GSI payload where `round.phase` enters `freezetime`, the app checks the foreground window:

- if CS2 is focused, no sound plays
- if another app is focused, the alert sound plays

Tray menu:

- `Enable alerts`: toggle alerts
- `Open settings folder`: open `%AppData%\CS2RoundAlert`
- `Open GitHub repo`: open this repository
- `Quit`: exit the app

## Settings

Settings are stored at:

```text
%AppData%\CS2RoundAlert\settings.json
```

Default settings:

```json
{
  "port": 3000,
  "enabled": true,
  "alertOnlyWhenNotFocused": true,
  "useSystemSound": true,
  "customWavPath": null,
  "cs2CfgFolderPath": null,
  "firstRunNotificationShown": false
}
```

To use a custom `.wav` file:

```json
{
  "useSystemSound": false,
  "customWavPath": "C:\\Path\\To\\alert.wav"
}
```

Restart CS2RoundAlert after editing settings. If you change the port, restart CS2 after the app rewrites the GSI config.

## Build from source

Requirements:

- Windows
- .NET 8 SDK

Run:

```powershell
dotnet publish src/CS2RoundAlert/CS2RoundAlert.csproj `
  -c Release `
  -r win-x64 `
  --self-contained `
  -p:PublishSingleFile=true `
  -p:EnableCompressionInSingleFile=true `
  -o publish
```

The output is:

```text
publish\CS2RoundAlert.exe
```

## Release workflow

GitHub Actions builds a self-contained single-file Windows executable on tag push.

Example:

```powershell
git tag v0.1.0
git push origin v0.1.0
```

The workflow creates a GitHub Release and attaches `CS2RoundAlert.exe`.

## Screenshots

Screenshots will be added after the first packaged release.

- Tray menu
- First-run notification
- Settings folder

## License

MIT
