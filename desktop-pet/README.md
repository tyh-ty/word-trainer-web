# DesktopPet C++ 主程序

`desktop-pet/` 是整合项目的 C++ 核心。它不是单独的小工具，而是整个大作业的主程序：启动透明桌宠窗口，并用 WebView2 内嵌 `../web/` 背单词界面。

## 功能

- Win32 透明悬浮桌宠窗口
- Direct2D / WIC 图片绘制
- 点击、拖拽、右键菜单和热键
- WebView2 内嵌网页学习界面
- WebView2 bridge 接收网页的朗读、记录、提醒请求
- Windows SAPI TTS 英文朗读
- WinSock 本地 HTTP 服务，方便外部浏览器调试
- 多线程复习提醒和后台朗读
- WinHTTP 调用聊天 API 和天气接口
- Boost.JSON 配置和学习记录读写

## 构建

```powershell
cmake -S . -B build
cmake --build build
.\build\DesktopPet.exe
```

构建时会复制：

- `res/` 到 exe 目录
- `../web/` 到 exe 目录
- `WebView2Loader.dll` 到 exe 目录
- `config.json` 到 exe 目录

## 配置

`config.json` 里的 `word_trainer.web_path` 默认是：

```json
"web_path": "web\\index.html"
```

也就是加载 exe 旁边复制出来的网页文件。这样打包和演示时，C++ 程序、网页界面和桌宠资源在同一个输出目录中。

API key 建议使用环境变量：

```powershell
$env:DESKTOPPET_API_KEY="你的 API key"
.\build\DesktopPet.exe
```
