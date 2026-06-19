# 四六级单词训练与 C++ 桌宠伴学系统

这是一个 C++ 课程大作业项目。项目主体是 Windows C++ 桌面程序 `DesktopPet.exe`：它启动桌面桌宠，同时用 WebView2 内嵌背单词网页界面。网页只负责交互展示，桌宠显示、TTS 朗读、复习提醒、本地记录、AI 调用和桥接接口由 C++ 完成。

在线网页演示：

https://tyh-ty.github.io/word-trainer-web/

> 在线网页只能展示前端界面。完整大作业演示请在 Windows 本机运行 C++ 程序。

## 目录结构

```text
.
├─ desktop-pet/       # C++17 Windows 桌宠主程序
├─ web/               # 被 C++ WebView2 内嵌的背单词界面
├─ open_web.bat       # 一键构建/启动 C++ 主程序
├─ run_desktop_pet.bat
├─ docs/              # 演讲稿、PPT 等答辩材料
└─ .github/           # GitHub Pages 前端演示部署
```

## 项目架构

```text
DesktopPet.exe
├─ Win32 / Direct2D 桌面桌宠窗口
├─ WebView2 内嵌网页学习界面
├─ Windows SAPI TTS 英文朗读
├─ WinSock 本地服务和复习提醒线程
├─ Boost.JSON 配置、记录和消息解析
└─ WebView2 bridge：网页按钮直接调用同进程 C++ 能力
```

网页中不再绘制第二个 HTML 桌宠，只保留“C++ 桌宠状态”面板。真正的桌宠是 C++ 透明悬浮窗口，避免前端桌宠和 C++ 桌宠重复。

## C++ 技术点

- Win32 透明悬浮窗口、右键菜单、热键和消息循环
- Direct2D / WIC 绘制桌宠图片、阴影和气泡
- WebView2 在 C++ 程序中内嵌 HTML/CSS/JS 学习界面
- WebView2 `postMessage` 桥接网页操作和 C++ 能力
- Windows SAPI TTS 朗读英文单词
- WinSock 本地 HTTP 服务，兼容外部浏览器调试
- WinHTTP 调用聊天 API 和天气接口
- Boost.JSON 解析配置、接口请求和学习记录
- 多线程处理桌宠、HTTP 服务、复习提醒、API 请求和朗读
- 文件读写保存复习记录、窗口位置和日志

## 本地运行

推荐直接运行：

```powershell
.\open_web.bat
```

这个脚本会构建并启动 C++ 主程序。程序启动后会出现两个窗口：

- C++ 桌宠透明悬浮窗口
- C++ 内嵌 WebView2 背单词学习窗口

默认演示账号：

```text
账号：student
密码：123456
```

也可以手动构建：

```powershell
cd desktop-pet
cmake -S . -B build
cmake --build build
.\build\DesktopPet.exe
```

如果 CMake 找不到 Boost，可以先设置：

```powershell
$env:BOOST_ROOT="C:\path\to\boost"
```

WebView2 SDK 会在 CMake 配置阶段自动下载到 `desktop-pet/third_party/`，该目录不提交到仓库。

## API Key 配置

仓库里的 `desktop-pet/config.json` 不包含真实 API key。运行 C++ 桌宠时建议设置环境变量：

```powershell
$env:DESKTOPPET_API_KEY="你的 API key"
.\desktop-pet\build\DesktopPet.exe
```

网页里的“AI设置”只影响学习界面的远程 AI 提示，配置保存在当前浏览器/WebView 的 `localStorage`。

## 大作业演示建议

1. 运行 `open_web.bat`，说明项目入口是 C++ 程序。
2. 展示 C++ 桌宠悬浮窗口和内嵌 WebView2 学习窗口。
3. 在内嵌网页点击“朗读”，展示网页通过 bridge 调用 C++ SAPI TTS。
4. 答题或查词，展示学习记录同步和 C++ 桌宠气泡反馈。
5. 右键桌宠切换心情/外观，展示 Win32 菜单和 Direct2D 绘制。
6. 说明复习计划、错题、查词计划如何由网页交互与 C++ 本地记录协作完成。
