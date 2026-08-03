# MultiStart

MultiStart 是一个兼容 Qt 5.15.2 和 Qt 6 的 Windows 程序启动中心。它从 JSON 配置文件动态读取项目，统一展示并启动多个 EXE、BAT/CMD 文件或目标目录。

## 功能

- 根据配置动态显示任意数量的启动项
- 支持每个项目配置独立图标、名称和说明
- EXE 未配置图标时自动读取文件自身图标
- 支持启动 EXE、执行 BAT/CMD、打开目录
- 支持相对路径、启动参数和工作目录
- 启动时自动读取 EXE 同目录的配置文件
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
