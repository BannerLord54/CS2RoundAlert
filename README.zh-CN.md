# CS2RoundAlert

[English README](README.md)

一个给 Counter-Strike 2 用的 Windows 托盘小工具。

当新回合开始，而你切到了其他窗口时，它会播放提示音，提醒你切回游戏。

## 下载

在这里下载 `CS2RoundAlert.exe`：

<https://github.com/BannerLord54/CS2RoundAlert/releases/latest>

下载 `.exe` 文件，不要下载 `Source code`。

## 怎么使用

1. 运行 `CS2RoundAlert.exe`。
2. 如果 Windows 弹出警告，点 `更多信息` -> `仍要运行`。
3. 启动或重启 CS2。
4. 让 CS2RoundAlert 保持在系统托盘里运行。

就这样。

当下一回合开始时：

- 如果你已经在 CS2 里，不会响。
- 如果你切到了别的窗口，会播放提示音。

## 第一次运行

第一次运行时，程序会自动安装 CS2 的 GSI 配置文件。

如果程序找不到 CS2，它会让你手动选择 CS2 配置文件夹：

```text
Counter-Strike Global Offensive\game\csgo\cfg
```

选好后，重启一次 CS2。

## 托盘菜单

右键托盘图标：

- `Enable alerts`：开启或关闭提示音。
- `Open settings folder`：打开设置文件夹。
- `Open GitHub repo`：打开项目页面。
- `Language`：选择自动、英文或中文。
- `Quit`：退出程序。

默认是自动跟随系统语言。

## 对 CS2 安全吗？

CS2RoundAlert 只使用 Valve 官方的 Game State Integration 功能。

它不会：

- 读取游戏内存
- 注入代码
- 模拟鼠标或键盘输入
- 自动操作游戏

它只是在你自己的电脑上接收 CS2 发来的回合状态 JSON。

Valve GSI 文档：

<https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration>

## 常见问题

### 没有声音

按顺序检查：

1. 确认 CS2RoundAlert 还在托盘里运行。
2. 右键托盘图标，确认 alerts 已开启。
3. 重启 CS2。

### Windows 提示风险

因为这个 exe 没有代码签名，Windows SmartScreen 可能会弹警告。项目源码和构建流程都在这个仓库里公开。

## 设置

设置文件在这里：

```text
%AppData%\CS2RoundAlert\settings.json
```

大多数用户不需要改这个文件。

## 开发者

使用 .NET 8 构建：

```powershell
dotnet publish src/CS2RoundAlert/CS2RoundAlert.csproj -c Release -r win-x64 --self-contained -p:PublishSingleFile=true -o publish
```

## 许可证

MIT
