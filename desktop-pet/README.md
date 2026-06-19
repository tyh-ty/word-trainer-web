# DesktopPet C++ 桌宠

这是整合项目里的 C++17 桌宠程序。它运行在 Windows 桌面上，并为背单词网页提供本地增强服务。

## 功能

- 透明悬浮桌宠窗口
- Direct2D / WIC 图片绘制
- 点击、拖拽、右键菜单和热键
- Windows SAPI TTS 英文朗读
- WinSock 本地 HTTP 服务
- 多线程复习提醒和后台朗读
- WinHTTP 调用聊天 API / 天气接口
- Boost.JSON 配置和学习记录读写

## 构建

```powershell
cmake -S . -B build
cmake --build build
```

运行：

```powershell
.\build\DesktopPet.exe
```

## 配置

`config.json` 会在构建后复制到 `build/` 目录。真实 API key 不建议写入仓库，可以使用环境变量覆盖：

```powershell
$env:DESKTOPPET_API_KEY="你的 API key"
.\build\DesktopPet.exe
```

`word_trainer.web_path` 默认指向整合项目里的 `web/index.html`。右键桌宠可以打开背单词网页。

## 本地接口

默认监听：

```text
http://127.0.0.1:18080
```

主要接口：

- `GET /status`：网页检测桌宠服务是否在线
- `POST /speak`：Windows TTS 朗读单词
- `POST /record`：同步网页答题记录
- `POST /study-state`：网页触发桌宠动作和状态
- `POST /bubble`：显示桌宠气泡
- `GET /due`：读取待复习记录
