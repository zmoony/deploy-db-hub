# Deploy Hub 开发说明

## 当前代码范围

当前代码是 V1 的第一批工程骨架，包含：

- CMake + Qt 6 Widgets 应用入口。
- 核心状态枚举与 `DeployJob` 状态机基础。
- 版本号生成与校验。
- 构建产物匹配。
- 项目/服务器配置的基础校验。
- 日志脱敏与日志写入。
- SQLite 配置存储表初始化与项目/服务器 upsert。
- 本地构建、Git 来源、SSH、WinRM 的接口骨架。
- Qt Test 单元测试骨架。

SSH/WinRM 真实连接尚未实现；当前 UI 已提供仪表盘、项目、服务器、部署、历史和设置页，以及演示部署流程。

## 本地依赖

Windows：

- Visual Studio 2022 C++ 工具链。
- Qt 6，包含 Core、Gui、Widgets、Sql、Test。
- 当前本机 Qt 路径：`D:\systemInsall\Qt\6.11.1\mingw_64`。
- 当前本机可使用 Qt 自带工具：`Tools\CMake_64`、`Tools\Ninja`、`Tools\mingw1310_64`。

Linux：

```bash
sudo apt install cmake qt6-base-dev libssh-dev libcurl4-openssl-dev libsecret-1-dev
```

## 构建与测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows 当前路径可直接使用脚本：

```powershell
.\scripts\build-release.ps1
.\scripts\package-windows.ps1
.\dist\windows\deploy-hub.exe
```

打包输出目录：

```text
dist\windows\
```

其中 `deploy-hub.exe` 已由 `windeployqt` 收集动态链接 DLL，可直接运行。

## 开发顺序建议

1. 完成 `DeployOrchestrator`，把 `LocalBuilder`、`ArtifactMatcher`、`RemoteExecutor` 串成单机部署流程。
2. 实现项目/服务器 SQLite 读取列表与表单 UI。
3. 接入真实 `SshRemoteExecutor`。
4. 接入真实 `WinRmRemoteExecutor`。
5. 补齐部署历史、日志查看、取消与回滚。
