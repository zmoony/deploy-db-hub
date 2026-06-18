# Deploy Hub 技术设计

> 文档版本：1.3 | 状态：已定稿 | 最后更新：2026-06-15
>
> 关联文档：[文档索引](./README.md) · [deploy-hub-prd.md](./deploy-hub-prd.md) · [JSON Schema](./schemas/)

## 1. 概述

本文档描述 Deploy Hub V1 的**技术实现方案**，包括架构、模块划分、存储、远程协议、并发模型与安全实现。产品行为与验收标准以 PRD 为准；若冲突，PRD 优先。

## 2. 技术栈（V1 定论）

| 决策项 | 选型 | 说明 |
|--------|------|------|
| 语言 | C++17 | 与 Qt 6 兼容 |
| UI | Qt 6 Widgets | 工具型桌面应用 |
| 构建系统 | CMake 3.21+ | 跨平台 |
| 配置与记录 | SQLite 3 | 单文件 `deploy-hub.db` |
| 部署日志 | 纯文本文件 UTF-8 | 与 DB 索引分离 |
| SSH/SFTP | libssh | 连接、执行、SFTP 上传 |
| WinRM | libcurl + 平台认证能力 | HTTPS + NTLM/Negotiate；Windows 用 SSPI，Linux 用 libcurl 可用的 GSSAPI/NTLM 后端 |
| Git | 系统 `git` CLI | `QProcess` 调用，不用 libgit2 |
| 本地构建 | `QProcess` | 捕获 stdout/stderr |
| 单元测试 | Qt Test | `ctest` 集成 |
| 钥匙串 | Qt Keychain；Linux 失败时回退 libsecret | Win Credential Manager / Secret Service |

### 2.1 数据目录

| 平台 | 路径 |
|------|------|
| Windows | `%APPDATA%/deploy-hub/` |
| Linux | `~/.local/share/deploy-hub/` |

目录结构：

```
<dataDir>/
  deploy-hub.db
  logs/
    <deploy-id>.log
  known_hosts          # SSH host key 缓存（若不用系统 known_hosts）
```

工作区（可配置，非 dataDir）：

```
<workspaceRoot>/
  repos/
    <projectId>/       # GitHub clone 目录
```

默认 `workspaceRoot`：Windows `%USERPROFILE%/deploy-hub-workspace`；Linux `~/deploy-hub-workspace`。

## 3. 架构

### 3.1 分层

```
┌─────────────────────────────────────────┐
│  UI Layer (Qt Widgets)                  │
│  MainWindow, DeployPage, ProjectForm... │
└─────────────────┬───────────────────────┘
                  │ signals/slots
┌─────────────────▼───────────────────────┐
│  Application / DeployOrchestrator       │
│  DeployJob lifecycle, state machine       │
└─────────────────┬───────────────────────┘
                  │
    ┌─────────────┼─────────────┬──────────────┐
    ▼             ▼             ▼              ▼
 LocalBuilder  GitSource   SshRemote    WinRmRemote
               Provider    Executor     Executor
    │             │             │              │
    └─────────────┴─────────────┴──────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  Infrastructure                          │
│  ConfigStore, LogWriter, LogSanitizer,   │
│  CredentialStore, ArtifactMatcher        │
└─────────────────────────────────────────┘
```

### 3.2 核心适配器

| 类 | 职责 |
|----|------|
| `LocalSourceProvider` | 校验 `localPath`，返回构建根目录 |
| `GitSourceProvider` | shallow clone / fetch+checkout，注入 token |
| `LocalBuilder` | `QProcess` 执行构建命令，超时与 env |
| `ArtifactMatcher` | glob 匹配、`fail-if-multiple` / `newest` |
| `SshRemoteExecutor` | SSH 连接、远程命令、SFTP 上传、symlink |
| `WinRmRemoteExecutor` | WinRM 命令、分块上传、junction |
| `DeployJob` | 串联各阶段，维护状态与取消标志 |
| `ConfigStore` | SQLite CRUD，JSON 序列化 |
| `CredentialStore` | 钥匙串读写，`credentialRef` 映射 |
| `LogWriter` | 分阶段写日志文件，调用 `LogSanitizer` |
| `LogSanitizer` | 脱敏规则（见第 11.2 节） |

### 3.3 建议代码目录

```
deploy-hub/
  CMakeLists.txt
  src/
    app/              # main, Application
    ui/               # Widgets, dialogs
    core/             # DeployJob, DeployOrchestrator, state machine
    adapters/
      ssh/
      winrm/
      build/
      git/
    infra/            # ConfigStore, LogWriter, CredentialStore
  tests/
    unit/
    integration/      # 需 SSH/WinRM 环境，nightly
  docs/
```

## 4. 部署状态机（实现）

与 PRD 状态枚举一致。`DeployJob` 内部流转：

```
pending -> building -> uploading -> restarting -> success
                    \-> failed (各阶段均可)
restarting (fail + rollback policy) -> rollbacking -> rolled_back | rollback_failed
任意运行阶段 + user cancel -> canceled
```

- 状态变更写入 SQLite `deployments.status` 并发射 Qt 信号刷新 UI。
- `failureStep` 取值：`validate` | `building` | `uploading` | `current_switch` | `restarting` | `rollback`。

## 5. 进程与并发模型

- UI 主线程：**仅** Qt 事件循环与用户交互。
- 每次部署：一个 `DeployJob` 运行在 `QThread` worker。
- 日志与进度：`DeployJob` → `DeployOrchestrator` → UI（`Qt::QueuedConnection`）。
- 取消：共享 `std::atomic<bool> cancelRequested`；各阶段轮询，构建 kill `QProcess`，SSH/WinRM 断开或等待当前块结束。
- **全局单任务**：`DeployOrchestrator::hasActiveJob()` 为 true 时拒绝新任务（PRD 要求）。

### 5.1 超时（实现常量）

| 阶段 | 秒 | 配置来源 |
|------|-----|----------|
| 构建 | `build.timeoutSec`（默认 600） | 项目 JSON |
| 上传 | 1800 | 代码常量 `UPLOAD_TIMEOUT_SEC` |
| restart | 300 | 代码常量 `RESTART_TIMEOUT_SEC` |
| SSH ping | 10 | `SshRemoteExecutor` |
| WinRM ping | 15 | `WinRmRemoteExecutor` |

## 6. 存储设计

### 6.1 SQLite 表

数据库：`<dataDir>/deploy-hub.db`，`schemaVersion` 与配置 JSON 同步演进。

| 表 | 字段 | 说明 |
|----|------|------|
| `projects` | `id` PK, `config_json` TEXT, `updated_at` TEXT | 项目配置整存 |
| `servers` | `id` PK, `config_json` TEXT, `updated_at` TEXT | 服务器配置整存 |
| `deployments` | `id` PK, `project_id`, `server_id`, `status`, `record_json` TEXT, `started_at`, `finished_at` | 列表索引 + 详情 JSON |
| `settings` | `key` PK, `value` TEXT | 全局设置 |
| `schema_meta` | `key`, `value` | DB schema 版本号 |

`record_json` 结构与 PRD「部署记录」JSON 一致，字段约束见 [deployment-record.schema.json](./schemas/deployment-record.schema.json)。

项目/服务器配置分别见 [project-config.schema.json](./schemas/project-config.schema.json)、[server-config.schema.json](./schemas/server-config.schema.json)。

### 6.2 日志文件

- 磁盘路径：`<dataDir>/logs/<deploy-id>.log`
- 部署记录 `logPath` 字段存**相对 dataDir** 的路径，格式：`logs/<deploy-id>.log`（与 PRD 示例一致）
- 阶段分隔符行：`=== BUILD ===`、`=== UPLOAD ===`、`=== RESTART ===`
- 单文件上限 **50MB**，超出截断并追加 `=== LOG TRUNCATED ===`
- UI 内存缓冲最近 **2000** 行；完整内容按需读文件

### 6.3 日志保留策略（实现）

- 启动时清理：删除 `started_at` 早于 30 天的 `deployments` 索引，并同步删除对应 log 文件；删除失败时保留文件并写入应用日志。
- 索引条数超过 100 时，删除最旧记录索引，并同步删除对应 log 文件；若 log 文件缺失，不视为错误。

## 7. 远程协议实现

### 7.1 SSH / SFTP（Linux 目标机）

**连接**

- libssh `ssh_connect` + 密码或公钥认证。
- Host key：`knownHostsPolicy = strict-with-prompt` 时，未知 key 回调 UI 展示 SHA256 fingerprint，确认后写入 storage。

**远程命令**

```cpp
// 伪代码
session.exec(remoteCommand, timeoutSec, &stdout, &stderr, &exitCode);
```

**上传**

- SFTP 递归上传目录或单文件到 `releases/<version>/`。
- 进度回调：`bytesSent / totalBytes` → UI。

**`current` 切换（Linux）**

```bash
ln -sfn <remoteBaseDir>/releases/<version> <remoteBaseDir>/current
```

通过远程 `ssh` 执行；失败则 `failureStep = current_switch`。

### 7.2 WinRM（Windows 目标机）

**连接**

- HTTPS 默认端口 5986；XML SOAP over HTTP（WS-Management）。
- HTTP/TLS 由 libcurl 实现；Windows 认证使用 SSPI，Linux 认证使用 libcurl 可用的 GSSAPI/NTLM 后端。
- 认证：NTLM 或 Negotiate；禁用 Basic over HTTP。
- `tlsVerify = false` 或指纹匹配时使用用户已信任的 `trustedCertFingerprint`。

**TLS 证书指纹**

- 字段 `winrm.trustedCertFingerprint` 存 **SHA-1** 证书指纹（40 位十六进制，**无冒号**，大小写不敏感比较）。
- 与 Linux SSH host key 展示的 **SHA256** 不同；二者用途与算法均不同，不可混用。
- 获取方式（目标机 PowerShell）：`(Get-ChildItem Cert:\LocalMachine\My).Thumbprint`

**远程命令**

- PowerShell 脚本或 `cmd` 通过 WinRM `Command` 资源执行。

**文件上传**

V1 固定采用 WinRM + PowerShell 分块上传协议：

1. 客户端按 **1MB 原始字节**切分文件，每块 Base64 编码后通过 WinRM 执行远端 PowerShell 片段。
2. 上传前远端创建临时文件：`<target>.deployhub.tmp`；若已存在同名临时文件，先删除。
3. 每块在远端执行：

```powershell
$bytes = [Convert]::FromBase64String("<chunk-base64>")
$stream = [System.IO.File]::Open("<target>.deployhub.tmp", [System.IO.FileMode]::Append)
try {
  $stream.Write($bytes, 0, $bytes.Length)
} finally {
  $stream.Dispose()
}
```

4. 全部块上传后，客户端传入本地计算的 SHA-256；远端执行：

```powershell
$actual = (Get-FileHash "<target>.deployhub.tmp" -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne "<sha256>") { exit 2 }
Move-Item -Force "<target>.deployhub.tmp" "<target>"
```

5. 任一块失败、校验失败或取消时，远端尝试删除 `<target>.deployhub.tmp`，部署阶段标记为 `uploading` 失败；若清理失败，在日志中记录临时文件路径。

- 单文件上限 **500MB**；无断点续传。
- 目录上传时先在客户端枚举文件并逐文件上传，保持相对路径；任一文件失败则整个上传阶段失败。

**`current` 切换（Windows）**

```powershell
# junction: 删除旧链接后创建
cmd /c rmdir "<remoteBaseDir>\current" 2>nul
cmd /c mklink /J "<remoteBaseDir>\current" "<remoteBaseDir>\releases\<version>"
```

### 7.3 连通性 / 命令测试

实现与 PRD 一致：

| OS | 连通性 | 命令测试 |
|----|--------|----------|
| Linux | `echo deploy-hub-ping` | `whoami` + `pwd` |
| Windows | `hostname` | `$PSVersionTable.PSVersion` |

## 8. Git 来源实现

### 8.1 命令序列

**判断 `ref` 类型**

- **分支 / tag**：匹配 `refs/heads/*` 或 tag 名（不含 40 位纯十六进制 SHA）。
- **commit SHA**：7–40 位十六进制（全写或短 SHA）。

**首次（工作区不存在）— 分支 / tag**

```bash
git clone --depth 1 --branch <ref> <auth-url> <workspaceRoot>/repos/<projectId>
```

**首次 — commit SHA**

```bash
git clone --depth 1 <auth-url> <workspaceRoot>/repos/<projectId>
cd <workspaceRoot>/repos/<projectId>
git fetch --depth 1 origin <sha>
git checkout -f <sha>
```

若 `git fetch --depth 1 origin <sha>` 失败，尝试 `git fetch --unshallow` 后重新 fetch checkout；仍失败则提示用户检查 SHA 与网络。

**已存在 — 分支**

```bash
cd <repo> && git fetch origin && git checkout -f <ref> && git pull --ff-only
```

**已存在 — tag / commit**

```bash
cd <repo> && git fetch origin && git checkout -f <ref>
```

对 commit SHA，若 shallow 导致 checkout 失败，同样走 unshallow 回退逻辑（§8.2）。

**Token 注入（HTTPS）**

- 内存构造 `https://<token>@github.com/org/repo.git` 仅用于子进程环境，**不写入**日志与配置。
- 或使用 `GIT_ASKPASS` 临时脚本（执行后删除）。

### 8.2 shallow / unshallow

- 首次 `--depth 1`。
- checkout 非 tip commit 失败时尝试 `git fetch --unshallow`，仍失败则返回用户可读错误。

### 8.3 错误映射

| 检测条件 | 用户提示 |
|----------|----------|
| `git` 启动失败 | 请安装 Git 并确保已加入 PATH |
| exit 128 + auth | 检查 GitHub token 或手动凭据 |
| exit 1 + unknown revision | 分支/tag/commit 无效，请检查 ref |
| 超时 | 拉取超时，请检查网络后重试 |
| `git status --porcelain` 非空 | UI 警告后将执行 `git checkout -f` |

## 9. 构建与产物

### 9.1 LocalBuilder

```cpp
QProcess process;
process.setWorkingDirectory(projectRoot + "/" + workingDir);
process.setProcessEnvironment(baseEnv + build.env);
process.start(shell, {"-c", command});  // Windows: cmd /c
// 超时: kill + failed(building)
```

### 9.2 ArtifactMatcher

- 使用 `QDir::entryList` + `QRegularExpression` 将 glob 转为匹配。
- `fail-if-multiple`：匹配数 > 1 则失败并列出路径。
- `newest`：按 `QFileInfo::lastModified` 取最新。

### 9.3 Python 目录产物

- 若匹配结果为目录：先打 zip 至临时目录 `%TEMP%/deploy-hub/<deploy-id>/artifact.zip`，再上传该 zip。

## 10. 回滚实现

触发条件见 PRD 矩阵。实现步骤：

1. 查询 `deployments` 中 `status=success` 且同 project+server，取最近 `remoteVersionDir`。
2. 远端检查目录存在（SSH `test -d` / PowerShell `Test-Path`）。
3. 切换 `current` 至该版本目录。
4. 调用 restart，传入**上一版本**的 `version-dir` 与对应 artifact 路径。
5. 新部署记录：`rolled_back` 或 `rollback_failed`，填充 `rolledBackFrom`、`failureReason`。

## 11. 安全实现

### 11.1 凭据

| 平台 | API |
|------|-----|
| Windows | Windows Credential Manager |
| Linux | Qt Keychain 默认后端；不可用时使用 Secret Service API（libsecret） |

- `credentialRef` 命名空间：`deploy-hub/<server-id>` 或 `deploy-hub/github-token`。
- `manual` 模式：凭据仅存应用内 `CredentialSessionCache`（可选「本次会话」），进程退出即失；缓存不写入 SQLite、日志或临时文件。

### 11.2 LogSanitizer

写入前对每行应用规则（不区分大小写字段名）：

- `password`, `passwd`, `secret`, `token`, `apikey`, `api_key`, `authorization` → 值替换为 `***`
- 正则：`Authorization:\s*\S+` → `Authorization: ***`
- URL 查询参数 `token`, `access_token` → 值替换
- 私钥 PEM 块（`BEGIN.*PRIVATE KEY`）整段替换为 `***`

### 11.3 威胁模型（V1）

- 操作者为本机**受信任用户**；不防御同机恶意进程读取内存或钥匙串。
- 网络：防 MITM 依赖 SSH host key 校验与 WinRM TLS。

## 12. 测试与 CI

### 12.1 单元测试（Qt Test）

- `ConfigStore` JSON 往返与 `schemaVersion`
- `ArtifactMatcher` glob 与策略
- `LogSanitizer` 规则
- `DeployJob` 状态机流转（mock 适配器）
- 版本号 `YYYYMMDD-HHmmss` 生成与时区

### 12.2 集成测试

需真实环境，**nightly 或手工**：

- Linux SSH 上传 + restart
- Windows WinRM 上传 + restart
- 回滚全流程

### 12.3 测试环境搭建

**Linux 目标机**

```bash
# Ubuntu 22.04
sudo apt install openssh-server
sudo useradd -m deploy
# 配置 remoteBaseDir 写权限，放置 restart.sh
```

**Windows 目标机**

```powershell
# 启用 WinRM HTTPS（管理员）
winrm quickconfig
# 或参考 Microsoft 文档配置 5986 监听器与证书
Set-Item WSMan:\localhost\Service\Auth\Basic $false
```

**客户端 CI（GitHub Actions / 本地）**

```yaml
# 概念流水线
- cmake -B build -DCMAKE_BUILD_TYPE=Debug
- cmake --build build
- ctest --test-dir build --output-on-failure
# integration  job: if: github.event_name == 'schedule'
```

## 13. 依赖与构建说明

### 13.1 主要第三方依赖

| 依赖 | 用途 |
|------|------|
| Qt 6 Core, Gui, Widgets, Sql | UI + SQLite |
| libssh | SSH/SFTP |
| libcurl | WinRM HTTP/TLS 与 NTLM/Negotiate 传输 |
| OpenSSL | libssh / libcurl TLS 后端 |

### 13.2 平台包（开发参考）

- Ubuntu: `qt6-base-dev`, `libssh-dev`, `libcurl4-openssl-dev`, `libsecret-1-dev`
- Windows: Qt 在线安装器 + vcpkg `libssh`, `curl`

## 14. 变更记录

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-06-15 | 从 PRD 1.2 拆分为独立技术设计 |
| 1.1 | 2026-06-15 | 关联 JSON Schema；文档索引链接 |
| 1.2 | 2026-06-15 | 固定 SSH/WinRM/凭据实现；补齐 WinRM 分块上传协议 |
| 1.3 | 2026-06-15 | Code review 修订：logPath 约定、WinRM SHA-1 指纹、Git SHA 流程 |
