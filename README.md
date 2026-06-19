# 四六级单词训练与 C++ 桌宠伴学系统

这是一个整合版 C++ 课程大作业项目。网页背单词系统是主体入口，Windows C++ 桌宠作为网页的本地伴学模块，由网页自动唤起并通过本地接口联动。

在线演示网页：

https://tyh-ty.github.io/word-trainer-web/

> 在线网页可以作为主体界面展示。C++ 桌宠、Windows TTS、本地 HTTP 服务需要在 Windows 本机注册协议并编译运行。

## 目录结构

```text
.
├─ web/              # 四六级单词训练网页
├─ desktop-pet/      # C++17 Windows 桌宠源码
├─ open_web.bat      # 本地一键打开网页
├─ register_desktop_pet_protocol.bat # 注册网页唤起桌宠协议
├─ run_desktop_pet.bat # 本地一键构建/启动 C++ 桌宠
└─ .github/          # GitHub Pages 自动部署
```

## 项目关系

本项目以网页为主界面：用户打开 `web/index.html` 或 GitHub Pages 后，网页会先检查本机 C++ 桌宠服务是否在线；如果没有在线，网页会尝试通过 `wordtrainerpet://start` 唤起桌宠。桌宠启动后提供 Windows TTS 朗读、桌面气泡、复习提醒和本地 HTTP 接口，网页负责学习流程和数据展示。

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

第一次在本机使用“网页启动桌宠”前，先注册一次 Windows 协议：

```powershell
.\register_desktop_pet_protocol.bat
```

打开网页：

```powershell
.\open_web.bat
```

这个脚本会先注册网页唤起桌宠协议，再打开网页。网页打开后会自动尝试启动并连接 C++ 桌宠；如果浏览器拦截了自动唤起，可以点击页面里的“启动桌宠”按钮。

或直接打开：

```powershell
start .\web\index.html
```

默认演示账号：

```text
账号：student
密码：123456
```

如果只想单独调试 C++ 桌宠，也可以直接构建并启动：

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

1. 先运行 `register_desktop_pet_protocol.bat`，说明网页可以唤起本机 C++ 模块。
2. 打开 `web/index.html` 或在线网页，展示网页作为主界面。
3. 网页自动启动 C++ 桌宠后，展示桌宠悬浮窗口。
4. 在网页里点击“朗读”，展示网页调用 C++ 本地服务并用 Windows TTS 读单词。
5. 答题或查词，展示 C++ 桌宠气泡和动作反馈。
6. 说明 C++ 服务端如何通过本地 HTTP 接口与网页协作。
