# CS2RoundAlert

[English README](README.md)

一个给 Counter-Strike 2 用的 Windows 小工具。

你切到别的软件时，新回合开始会播放提示音。

它不控制游戏，只接收 CS2 官方 GSI 数据。

## 下载

下载这里的 `CS2RoundAlert.exe`：

<https://github.com/BannerLord54/CS2RoundAlert/releases/latest>

下载 `.exe`，不要下载 `Source code`。

## 使用方法

1. 运行 `CS2RoundAlert.exe`。
2. 如果 Windows 提示风险，点 `更多信息` -> `仍要运行`。
3. 程序会自动查找 CS2，并安装 GSI 配置。
4. 点窗口里的 `测试提示音`，确认声音正常。
5. 可选：勾选 `回合结束时也提醒`。
6. 启动或重启 CS2。
7. 让 CS2RoundAlert 保持运行。

新回合开始时：

- 如果 CS2 正在前台，不会响。
- 如果你切到了别的窗口，会响。

如果开启 `回合结束时也提醒`，回合结束进入 `over` 阶段时也会响。这个提醒在 CS2 前台时也会响。

## 没有声音怎么办

按顺序检查：

1. 点 `测试提示音`。如果这个都没声音，检查 Windows 音量和系统声音。
2. 确认窗口里 `开启提示音` 已勾选。
3. 安装 GSI 配置后，重启一次 CS2。
4. 测试新回合提醒时要切出 CS2。
5. 看窗口里的 GSI 状态。如果一直显示没收到 CS2 数据，说明 CS2 没读到 GSI 配置。

## 托盘菜单

右键托盘图标：

- `Enable alerts`：开启或关闭提示音
- `Also alert when a round ends`：回合结束也提醒
- `Test sound`：测试提示音
- `Open settings folder`：打开设置文件夹
- `Open GitHub repo`：打开项目页面
- `Choose CS2 cfg folder`：手动选择 CS2 配置文件夹
- `Language`：切换语言
- `Quit`：退出程序

关闭窗口只是隐藏到托盘。要退出请点 `Quit`。

## 对 CS2 安全吗

安全设计如下：

- 不读取游戏内存
- 不注入代码
- 不模拟鼠标键盘
- 不自动操作游戏
- 只接收 Valve 官方 GSI JSON

Valve GSI 文档：

<https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration>

不确定第三方平台是否永远放行。完美平台、5E 等平台如果弹出拦截或警告，请停止使用。

## Windows 风险提示

这个 exe 没有代码签名，所以 SmartScreen 或杀毒软件可能提示风险。

为了方便检查：

- 源码公开
- exe 由 GitHub Actions 构建
- Release 附带 SHA256 校验
- 没有加壳、混淆、压缩壳

## 设置位置

```text
%AppData%\CS2RoundAlert\settings.json
```

日志位置：

```text
%AppData%\CS2RoundAlert\CS2RoundAlert.log
```

## 许可证

MIT
