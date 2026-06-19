# 四六级单词训练与 C++ 桌宠伴学系统

这是一个整合版 C++ 课程大作业项目，包含网页背单词系统和 Windows C++ 桌面宠物程序。

在线演示网页：

https://tyh-ty.github.io/word-trainer-web/

> 在线网页只能演示前端功能。C++ 桌宠、Windows TTS、本地 HTTP 服务需要在 Windows 本机编译运行。

## 目录结构

```text
.
├─ web/              # 四六级单词训练网页
├─ desktop-pet/      # C++17 Windows 桌宠源码
├─ open_web.bat      # 本地一键打开网页
├─ run_desktop_pet.bat # 本地一键构建/启动 C++ 桌宠
└─ .github/          # GitHub Pages 自动部署
```

## C++ 技术点

`desktop-pet/` 是本项目的 C++ 核心，包含：

- Win32 透明悬浮窗口、右键菜单、热键和消息循环
- Direct2D / WIC 绘制桌宠图片、阴影和气泡
- Windows SAPI TTS 朗读英文单词
- WinSock 本地 HTTP 服务，默认监听 `http://127.0.0.1:18080`
- WinHTTP 调用聊天 API 和天气接口
- Boost.JSON 解析配置、接口请求和学习记录
- 多线程处理 HTTP 服务、复习提醒、API 请求和朗读
- 文件读写保存复习记录、窗口位置和日志
- 与网页通过 `/status`、`/speak`、`/record`、`/study-state`、`/bubble`、`/due` 等接口联动

## 本地运行

打开网页：

```powershell
.\open_web.bat
```

或直接打开：

```powershell
start .\web\index.html
```

默认演示账号：

```text
账号：student
密码：123456
```

构建并启动 C++ 桌宠：

```powershell
.\run_desktop_pet.bat
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

## API Key 配置

仓库里的 `desktop-pet/config.json` 不包含真实 API key。运行 C++ 桌宠时建议设置环境变量：

```powershell
$env:DESKTOPPET_API_KEY="你的 API key"
.\desktop-pet\build\DesktopPet.exe
```

网页宠的独立 AI 闲聊在网页内点击“AI设置”填写 endpoint、model 和 API key，配置只保存在当前浏览器的 `localStorage`。

## 大作业演示建议

1. 打开 `web/index.html` 或在线网页，展示登录、背词、查词、复习计划和错题统计。
2. 启动 `desktop-pet/build/DesktopPet.exe`，展示 C++ 桌宠悬浮窗口。
3. 在网页里点击“朗读”，展示网页调用 C++ 本地服务并用 Windows TTS 读单词。
4. 答题或查词，展示 C++ 桌宠气泡和动作反馈。
5. 说明 C++ 服务端如何通过本地 HTTP 接口与网页协作。
