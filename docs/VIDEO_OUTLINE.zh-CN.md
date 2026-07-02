# CS2RoundAlert 视频发布提纲

## 标题备选

- CS2 切出去也能听到新回合提醒
- 我做了一个 CS2 回合开始提醒工具
- 用官方 GSI 做一个无注入的 CS2 提醒小工具

## 60 秒脚本

1. 痛点：CS2 死亡后切出去看网页，下一回合开始容易错过。
2. 工具：CS2RoundAlert，后台托盘运行。
3. 原理：CS2 官方 GSI 会把回合状态 POST 到本地地址。
4. 安全边界：不读内存、不注入、不模拟输入，只接收 JSON。
5. 演示：切到别的窗口，回合进入 freezetime，播放提示音。
6. 收尾：GitHub 开源，Release 下载单文件 exe。

## 视频简介

CS2RoundAlert 是一个 Windows 托盘小工具。它使用 Valve 官方 Game State Integration 接收 CS2 回合状态，在新回合进入 freezetime 时播放提示音。项目不读游戏内存、不注入、不模拟输入。

GitHub: https://github.com/BannerLord54/CS2RoundAlert

## 标签

CS2, Counter-Strike 2, GSI, GitHub, Windows, CSharp, DotNet
