# Deploy Hub

Deploy Hub 是一个基于 C++17 和 Qt 6 的桌面部署工具，QML 构建主壳 + Widget 业务页渐进迁移。目标是把个人开发者和小团队常见的发布流程收敛到一个可视化客户端里：

```text
选择项目 -> 本地构建或选择已有产物 -> 上传到服务器 -> 执行启动/重启命令 -> 查看日志和历史
```

当前工程已实现 Windows 桌面端的主要管理界面、真实远程命令通道、部署日志、配置持久化和 Qt Test 单元测试。产品与技术边界以 `docs/deploy-hub-prd.md`、`docs/deploy-hub-technical-design.md`、`docs/schemas/` 为准。

## 主要功能

- 仪表盘：展示部署概览、服务器状态、最近部署和快捷入口。
- 项目管理：维护 Local/GitHub 项目、构建命令、产物路径、目标服务器、远端目录、日志目录、启动/停止/重启命令、备份策略。
- 服务器管理：维护 Linux/Windows 服务器，支持默认目录、账号、端口和凭据配置。
- 一键部署：支持本地构建、上传已有 JAR、Maven 目录/本地仓库配置、JDK 配置选择、部署输出实时查看。
- 远程操作：Linux 通过 libssh 执行命令、SFTP 文件浏览、上传、编辑、删除、重命名；Windows 通过 WinRM + libcurl 执行命令并支持部署日志只读查看。
- 服务器监控：查看 CPU、内存、磁盘和进程列表，支持 PID/CPU/内存排序与 PID/命令行搜索。
- 日志与历史：本地部署日志写入 `config/logs/`；应用日志可从项目配置的远程日志目录读取，默认查看最后 100 行。
- 配置存储：项目、服务器、部署记录使用 SQLite；JDK 配置写入 `config/jdk-profiles.json`。
- 安全处理：支持会话凭据复用、日志脱敏；配置不应保存明文密码、token 或私钥 passphrase。

## 技术栈

| 项 | 说明 |
|----|------|
| 语言 | C++17 |
| UI | Qt 6 Quick（QML 主壳）+ Widgets（业务页渐进迁移） |
| 构建 | CMake 3.21+ |
| 存储 | SQLite + 本地日志文件 |
| 测试 | Qt Test + CTest |
| Linux 远程 | libssh + SFTP |
| Windows 远程 | WinRM + libcurl |

> Windows 远程默认路线为 WinRM + libcurl + 平台认证能力；Linux 远程默认路线为 libssh。

## 目录结构

```text
deploy-hub/
  src/
    app/              # 应用入口
    core/             # 部署状态、部署任务、编排逻辑
    infra/            # 配置、日志、路径、凭据、JDK/Maven 等基础设施
    adapters/         # build/git/ssh/winrm/remote 适配器
    ui/               # Qt Widgets 页面、对话框、样式
  tests/unit/         # Qt Test 单元测试
  docs/               # PRD、技术设计、schema、指南和脚本模板
  scripts/            # Windows 构建与打包脚本
  resources/          # Qt resource
  images/             # 应用图标
  agent_memory/       # 项目进度与约束记忆
```

## 环境要求

Windows 开发环境：

- Qt 6，需包含 Core、Gui、Widgets、Sql、Test、Svg、Concurrent。
- CMake、Ninja、MinGW 或其他 Qt 兼容 C++ 工具链。
- 本机当前验证路径：`D:\systemInsall\Qt\6.11.1\mingw_64`。

Linux 开发环境参考：

```bash
sudo apt install cmake qt6-base-dev libssh-dev libcurl4-openssl-dev libsecret-1-dev
```

运行部署能力还需要：

- GitHub 来源项目：本机安装 `git` 并加入 `PATH`。
- Maven 项目：本机安装 JDK 和 Maven，或在设置页配置 Maven 目录和 JDK 配置。
- Linux 目标机：可通过 SSH 访问，并具备部署目录写权限。
- Windows 目标机：启用 WinRM，客户端可使用 `winrs` 访问。

## 构建、测试和打包

Windows 推荐直接使用仓库脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
powershell -ExecutionPolicy Bypass -File scripts/package-windows.ps1
```

构建完成后运行：

```powershell
.\dist\windows\deploy-hub.exe
```

通用 CMake 流程：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

当前 `.gitignore` 会排除 `build*/`、`dist/`、`deps/`、`Testing/`、运行时 `config/` 和日志等本地生成物。

## 基本使用流程

1. 在服务器管理中新增目标服务器。
2. 配置 Linux SSH 或 Windows WinRM 连接信息，按需保存或临时输入凭据。
3. 在项目管理中新增项目，选择 Local 或 GitHub 来源。
4. 配置构建方式：
   - Maven/普通命令构建：填写构建命令和产物路径。
   - 上传已有 JAR：选择本地 JAR，跳过构建。
5. 配置远端部署目录、目标 JAR、备份策略、启动/停止/重启命令和应用日志目录。
6. 到一键部署页选择项目、服务器、JDK 配置并开始部署。
7. 在部署输出中查看本地构建/上传/远程执行日志，在应用日志中查看远端服务日志。

## 配置和数据

应用运行时默认使用程序目录下的 `config/`：

```text
config/
  deploy-hub.db
  logs/
  jdk-profiles.json
```

设置页可配置 Maven 目录、本地 Maven 仓库、JDK 配置和配置目录覆盖。配置目录变更通常需要重启应用后生效。

## 文档索引

- 产品需求：`docs/deploy-hub-prd.md`
- 技术设计：`docs/deploy-hub-technical-design.md`
- Schema：`docs/schemas/`
- Linux SSH 指南：`docs/guides/ssh-linux-setup.md`
- Windows WinRM 指南：`docs/guides/winrm-setup.md`
- restart 脚本模板：`docs/templates/`
- 开发说明：`docs/dev-setup.md`

## 当前边界

- 当前远程实现使用系统 OpenSSH 和 `winrs`，不是最终设计中的 libssh/libcurl 原生实现。
- 全局同一时刻只执行一个部署任务。
- 远程构建、团队审批、多租户、定时部署、Web 控制台不在当前 V1 范围内。
- 自动回滚能力以文档和数据结构为边界，真实部署场景仍需按项目启动/停止/重启命令和备份策略验证。
