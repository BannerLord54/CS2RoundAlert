# CS2RoundAlert

[中文说明](README.zh-CN.md)

A small Windows tray app for Counter-Strike 2.

When a new round starts and you are tabbed out, CS2RoundAlert plays a sound so you know to switch back.

## Download

Download `CS2RoundAlert.exe` here:

<https://github.com/BannerLord54/CS2RoundAlert/releases/latest>

Download the `.exe` file, not `Source code`.

## How to use

1. Run `CS2RoundAlert.exe`.
2. If Windows shows a warning, choose `More info` -> `Run anyway`.
3. Start or restart CS2.
4. Keep CS2RoundAlert running in the system tray.

That is all.

When the next round begins:

- if CS2 is already focused, nothing happens
- if you are in another window, a sound plays

## First run

On first run, the app installs a CS2 Game State Integration config file automatically.

If it cannot find CS2, it will ask you to choose the CS2 config folder:

```text
Counter-Strike Global Offensive\game\csgo\cfg
```

After this, restart CS2 once.

## Tray menu

Right-click the tray icon:

- `Enable alerts`: turn alerts on or off
- `Open settings folder`: open app settings
- `Open GitHub repo`: open this page
- `Language`: choose Auto, English, or Chinese
- `Quit`: exit the app

Default language is Auto.

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

1. Make sure CS2RoundAlert is still running in the tray.
2. Right-click the tray icon and make sure alerts are enabled.
3. Restart CS2.

### Windows warning

The app is not code-signed, so Windows SmartScreen may show a warning. The source code and build workflow are public in this repository.

## Settings

Settings are stored here:

```text
%AppData%\CS2RoundAlert\settings.json
```

Most users do not need to edit this file.

## For developers

Build with .NET 8:

```powershell
dotnet publish src/CS2RoundAlert/CS2RoundAlert.csproj -c Release -r win-x64 --self-contained -p:PublishSingleFile=true -o publish
```

## License

MIT
