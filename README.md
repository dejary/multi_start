# MultiStart

MultiStart 是一个兼容 Qt 5.15.2 和 Qt 6 的 Windows 程序启动中心。它从 JSON 配置文件动态读取项目，统一展示并启动多个 EXE、BAT/CMD 文件或目标目录。

## 功能

- 根据配置动态显示任意数量的启动项
- 支持每个项目配置独立图标、名称和说明
- EXE 未配置图标时自动读取文件自身图标
- 支持启动 EXE、执行 BAT/CMD、打开目录
- 支持相对路径、启动参数和工作目录
- 启动时自动读取 EXE 同目录的配置文件
- 只有一个可见启动项时，窗口首次显示后自动启动该项目
- 启动时检测新版本，支持更新说明、下载安装包、SHA-256 校验和强制升级
- 配置错误、目标缺失和启动失败均有明确提示

## 构建

需要 Qt 5.15.2 或 Qt 6、CMake 3.22 或更高版本，以及兼容的 C++17 编译器。项目推荐通过 `dux build` 构建，仓库中的 `dux.toml` 默认选择 Qt 5.15.2 x86。

```powershell
dux build
```

默认构建目录可通过 `dux env .` 查看。构建过程会把示例配置、脚本和数据目录复制到程序旁边。

指定 Qt 或架构：

```powershell
dux build --qt 5.15.2 --arch x86
```

进入对应构建目录运行 `ctest --output-on-failure` 可执行测试，并生成 `multistart-smoke.png` 检查主窗口布局。

生成包含 Qt 运行库的独立部署目录：

```powershell
dux build --install
```

默认安装目录同样可通过 `dux env .` 查看。部署完成后可直接运行安装目录下的 `bin/MultiStart.exe`，也可以将整个安装目录交给安装包制作工具。

## 配置

默认配置文件为 `MultiStart.exe` 同目录下的 `apps.json`，也可以通过命令行指定：

```powershell
MultiStart.exe --config D:/my-tools/apps.json
```

示例：

```json
{
  "window": {
    "title": "程序启动中心",
    "width": 800,
    "height": 600
  },
  "updates": {
    "enabled": true,
    "serverBaseUrl": "http://106.52.175.174:8080",
    "appId": "multi_start",
    "platform": "windows-x86",
    "channel": "stable"
  },
  "items": [
    {
      "name": "工具一",
      "type": "exe",
      "path": "tools/tool1.exe",
      "icon": "icons/tool1.ico",
      "workingDirectory": "tools",
      "arguments": ["--mode", "safe"],
      "description": "启动工具一",
      "visible": true
    },
    {
      "name": "初始化",
      "type": "bat",
      "path": "scripts/init.bat"
    },
    {
      "name": "数据目录",
      "type": "folder",
      "path": "data"
    }
  ]
}
```

所有相对路径均以 `apps.json` 所在目录为基准。`type` 支持 `exe`、`bat` 和 `folder`。

`updates.enabled` 为 `true` 且配置了 `serverBaseUrl` 时，程序会在显示启动中心前检测更新。更新服务使用 `appId`、`platform` 和 `channel` 定位版本；检测失败或没有更新不会阻塞程序。用户选择跳过的版本保存在 Qt `QSettings` 中。

首次克隆后需要初始化自动升级子模块：

```powershell
git submodule update --init --recursive
```

## 下载升级服务器上的最新版本

`scripts/download-latest.ps1` 可按应用、平台和通道查询升级服务器，下载最新发布文件，并校验服务端返回的文件大小和 SHA-256：

```powershell
.\scripts\download-latest.ps1 `
  -ServerUrl "https://update.example.com" `
  -App "multi_start" `
  -Platform "windows-x86" `
  -Channel "stable" `
  -DestinationDirectory ".\downloads"
```

`Channel` 默认为 `stable`，`Platform` 默认为本项目使用的 `windows-x86`。如果目标文件已存在且校验值相同，脚本不会重复下载；内容不同时默认拒绝覆盖，确认覆盖可添加 `-Force`。
