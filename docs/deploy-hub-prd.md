# Deploy Hub 可视化部署工具 PRD

> 文档版本：1.6 | 状态：已定稿 | 最后更新：2026-06-15
>
> 关联文档：[文档索引](./README.md) · [技术设计](./deploy-hub-technical-design.md) · [JSON Schema](./schemas/)

## 摘要

Deploy Hub 是一个面向个人运维/开发者的跨平台桌面部署工具，使用 **C++ + Qt** 构建，运行于 **Windows 和 Linux 桌面端**。核心目标是把常见项目的发布流程收敛为：

`选择项目 -> 本地打包 -> 上传到服务器 -> 执行统一 restart 脚本 -> 查看结果日志`

目标服务器支持：

- **Linux**：通过 SSH/SFTP 执行命令与上传文件。
- **Windows**：优先使用 WinRM / PowerShell 原生能力执行命令与上传部署包。
- `wrm4j` 不作为主路径；如后续确需复用 Java 生态，可作为外部 helper 的可选扩展方案。

项目来源支持：

- **Local**：选择本地项目目录。
- **GitHub**：配置仓库地址、分支/tag/commit、访问 token，拉取到本地工作区后构建。

首个完整闭环按 **单应用单服务器** 设计；全量产品路线中保留多服务器、环境编排、任务队列和团队化能力。

### 目标用户

Deploy Hub V1 面向以下用户：

1. **个人开发者**
   在本地完成构建，管理 1–3 台自有或租用的 Linux/Windows 服务器，希望用桌面工具完成「打包 → 上传 → 重启」，不依赖 Jenkins/GitHub Actions 等 CI 平台。

2. **个人运维 / 小团队技术负责人**
   熟悉 SSH、WinRM、Nginx、Java 部署流程，需要可重复的发布操作、完整日志与失败后可追溯（含可选自动回滚），但不需要多租户、审批流或云端调度。

V1 **不面向**：需要大规模并发发布、严格权限审计、或「在远程服务器上构建」的团队 CI 场景。

### V1 成功标准

满足以下全部条件，视为 V1 达到「个人可用完整闭环」：

- [ ] **Windows 客户端** → 将 **Java Maven** 项目部署到 **Linux 服务器**（SSH），全流程成功，历史记录可查。
- [ ] **Linux 客户端** → 将 **前端静态包** 部署到 **Windows 服务器**（WinRM），全流程成功，历史记录可查。
- [ ] 部署**失败**时，可查看**完整日志**（构建 / 上传 / 远程执行分阶段可见）。
- [ ] 配置 **保留现场** 时，失败后不回滚，远端状态与日志可核对。
- [ ] 配置 **自动回滚** 时，启动失败可恢复至**上一成功版本**，且界面同时展示**原始失败原因**与回滚结果。
- [ ] 凭据选择「不保存」时，每次部署前要求输入，且日志中无明文密码/token。

### V1 明确不做（范围外）

与路线图 V1 一致，首版**不包含**：

- 同一项目**同时**部署到多台服务器（批量 / 灰度）。
- 同一项目**并行**多次部署（重复点击需排队或拒绝）。
- 在**远程服务器**上执行构建（仅本地构建后部署）。
- 团队账号、多租户、审批流、操作审计平台。
- 定时部署、Web 控制台、插件化协议。
- `wrm4j` 作为默认 Windows 远程实现（仅保留为后续可选扩展）。
- 项目/服务器配置**导入导出**（换机迁移）。
- 服务器**标签与分组**（V2 环境分组前置）。
- 界面**多语言**与**无障碍**专项适配。
- 内置 **JSON / 配置文件编辑器**（仅表单编辑 + 日志查看）。

### 并发与反例

- V1 允许：**不同项目**顺序部署（一次只跑一个部署任务）。
- V1 不允许：单项目并行部署；单次发布多台服务器。

### 客户端运行环境（V1）

| 项 | 要求 |
|----|------|
| Windows 客户端 | Windows 10 21H2 或更高（x64） |
| Linux 客户端 | Ubuntu 22.04 LTS 或同等 glibc 发行版（x64） |
| 本机 Git | 必须安装且在 `PATH` 中（GitHub 来源） |
| 构建工具 | 按项目类型自行安装（如 Maven、Node.js、Python）；工具仅检测不存在时提示 |
| OpenSSH 客户端 | Linux 客户端自带；Windows 10+ 可选（用于本地 git ssh 协议） |

目标服务器最低要求：Linux 支持 SSH/SFTP；Windows Server 2016+ 或 Windows 10+ 且已启用 WinRM。

## 产品能力

### 1. 项目管理

每个项目保存一份部署配置，包括：

- 项目名称、项目类型、源码来源。
- 构建方式：仅支持 **本地构建后部署**。
- 构建命令和产物路径。
- 目标服务器。
- 远端部署目录。
- 重启脚本路径与参数。
- 失败策略：可选择「保留现场」或「自动回滚上一版」。

内置项目类型：

- **Java Maven**：默认构建命令 `mvn clean package -DskipTests`，产物通常为 `target/*.jar` 或 `target/*.war`。
- **前端静态包 / Nginx 部署**：默认构建命令可配置为 `npm run build` / `pnpm build`，产物目录如 `dist/`。
- **Python 项目**：必须配置构建命令；若 `artifactPath` 指向目录，则将该目录打包为 zip 后上传。
- **自定义项目**：用户完全配置构建命令和产物路径。

#### 源码来源字段约定

| 来源 | 必填字段 | 说明 |
|------|----------|------|
| Local | `localPath` | 本地项目根目录绝对路径 |
| GitHub | `repoUrl`, `ref` | `ref` 支持分支名、tag、`commit SHA`（全写或短 SHA，至少 7 位） |

- `ref` 对 **Local** 来源无效，配置中可省略。
- GitHub 拉取：首次 shallow clone 指定 `ref`；工作区已存在则 fetch 后 checkout。私有仓库需 token（最小权限 **repo read**）。工作区路径见设置页默认值。
- 工作区脏数据：部署前若检测到未提交修改，警告用户后**强制覆盖**（`checkout -f`）。
- 实现细节（命令序列、错误映射）见 [技术设计 §8](./deploy-hub-technical-design.md#8-git-来源实现)。

#### 前端静态包 / Nginx 部署示例

典型配置：

- `remoteBaseDir`：`/var/www/myapp`（Nginx `root` 指向 `current` 或 `current/dist`）
- `artifactPath`：`dist/`（目录整体上传到 `releases/<version>/`）
- Deploy Hub 上传完成后将 `current` **symlink 到该版本目录**，再执行 restart。
- `restart.sh` 职责：**仅**校验 Nginx 配置并 reload（不在 restart 阶段做 rsync；此时 `version-dir` 与 `current-dir` 为同一目录）：

```bash
#!/bin/bash
set -euo pipefail
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version-dir|--current-dir|--app|--artifact) shift 2 ;;
    *) shift ;;
  esac
done
nginx -t && systemctl reload nginx
```

V1 不提供 Nginx 配置编辑器；用户自行保证 `root` 与 `current` 目录约定一致。

#### 构建约定

| 项 | V1 约定 |
|----|---------|
| 构建位置 | 仅**本地**构建 |
| 工作目录 | 默认项目根；可通过 `build.workingDir` 配置相对子目录 |
| 环境变量 | 支持 `build.env` 键值对，在构建命令执行前注入当前进程环境 |
| 超时 | `build.timeoutSec` 默认 `600`，超时则构建失败，部署中止 |
| 仅部署不构建 | **V1 不支持**；必须执行 `build.command` |

#### 产物匹配规则

`build.artifactPath` 支持 glob（如 `target/*.jar`）。

| 匹配数量 | 行为（由 `build.artifactMatchPolicy` 决定） |
|----------|-----------------------------------------------|
| 0 | **失败**，错误信息：`未找到构建产物` |
| 1 | 使用该产物 |
| 多个 | 默认 **`fail-if-multiple`**：失败并列出匹配路径；可选 **`newest`**：取修改时间最新的一项 |

V1 默认策略：**`fail-if-multiple`**（避免静默传错包）。

### 2. 服务器管理

服务器资产包括：

- 名称、系统类型：`Linux` / `Windows`。
- 地址、端口、用户名。
- 认证方式：
  - 系统钥匙串保存凭据。
  - 部署时手动输入用户名/密码/token。
  - Linux 支持 SSH 密钥。
- 连通性测试。
- 默认部署根目录。
- 命令执行测试。

凭据默认进入系统安全存储（平台 API 见 [技术设计 §11](./deploy-hub-technical-design.md#11-安全实现)）；配置文件只保存凭据引用，不保存明文密码。

#### Linux（SSH / SFTP）

| 项 | V1 约定 |
|----|---------|
| 端口 | 默认 `22`，可配置 |
| 认证 | 密码（钥匙串/手动）、SSH 私钥文件路径；支持带 passphrase 的密钥（部署时提示输入，不落盘） |
| Host Key | 默认**必须校验**；首次未知时弹窗展示 fingerprint，用户确认后写入本机 `known_hosts` |
| 文件上传 | SFTP 至远端版本目录 |

**连通性测试**：建立 SSH 连接并完成认证，执行 `echo deploy-hub-ping`，超时 **10s**。

**命令执行测试**：在默认部署根目录（或 `/tmp`）执行 `whoami` 与 `pwd`，返回 stdout/stderr，超时 **10s**。

#### Windows（WinRM）

| 项 | V1 约定 |
|----|---------|
| 端口 | HTTPS 默认 **5986**；HTTP **5985** 仅开发环境可选 |
| 协议 | `winrm.scheme`：`https`（V1 默认）/ `http` |
| 认证 | **NTLM** 或 **Negotiate**（Kerberos 按环境支持）；V1 不支持 Basic over HTTP |
| TLS | `winrm.tlsVerify` 默认 `true`；自签名证书时用户可勾选「信任证书指纹」并持久化 `winrm.trustedCertFingerprint`（**SHA-1**，40 位十六进制） |
| 文件上传 | 通过 WinRM 上传至远端版本目录；单文件上限 **500MB**（V1 不做断点续传）；实现见 [技术设计 §7.2](./deploy-hub-technical-design.md#72-winrmwindows-目标机) |

**连通性测试**：执行 `hostname`，超时 **15s**。

**命令执行测试**：执行 `$PSVersionTable.PSVersion`，超时 **15s**。

#### 凭据模式

| 模式 | 行为 |
|------|------|
| `system-keychain` | 用户名存配置，`credentialRef` 指向钥匙串中的密码/token |
| `manual` | 每次部署前弹窗输入；可选勾选「仅本次会话记住」 |
| `ssh-key` | 仅 Linux；`sshPrivateKeyPath` 指向私钥文件 |

**模式组合**：

- 用户名始终存配置文件；密码/token 可走钥匙串或 `manual`。
- `ssh-key` 可与 `manual` 组合：私钥文件固定，passphrase 每次手动输入。
- 同一服务器切换认证模式后，旧 `credentialRef` 不自动删除，由用户在设置中清理。

配置文件**永不**保存明文密码、token、passphrase。

### 3. 一键部署流程

标准部署流程：

1. 校验项目配置、服务器连通性和本地构建工具（见下方校验清单）。
2. 获取源码：
   - Local：直接使用本地目录。
   - GitHub：clone/fetch 指定分支、tag 或 commit 到本地工作区。
3. 执行本地构建命令。
4. 定位构建产物。
5. 生成部署版本目录（见下方版本目录约定）。
6. 上传产物到远端版本目录。
7. 更新远端 `current` 指向或等效版本标记。
8. 执行统一重启脚本。
9. 展示部署状态、耗时、日志和结果。

#### 部署前校验清单

以下任一项失败则**阻塞部署**，不进入构建阶段：

| 校验项 | 失败提示要点 |
|--------|----------------|
| 项目必填字段完整 | 指出缺失字段 |
| 目标服务器存在且 OS 与执行器匹配 | 服务器不存在 / 类型不支持 |
| 服务器连通性 | 连接超时 / 认证失败 |
| Local：`localPath` 存在 | 目录不存在 |
| GitHub：`git` 可用 | 未检测到 git |
| 构建工具可用（如 `mvn`、`npm`） | 命令不在 PATH |
| 远端 `remoteBaseDir` 父路径可写（探测命令） | 无权限或路径非法 |
| 远端存在可执行的 restart 脚本 | 脚本不存在或不可执行 |

#### 版本目录与 `current` 语义

**版本目录命名**（与部署记录 `version` 字段一致）：

```
YYYYMMDD-HHmmss
```

时区：使用**客户端本地时区**生成；部署记录 `startedAt`/`finishedAt` 使用 ISO 8601 带时区偏移。

**目录结构示例**：

```
<remoteBaseDir>/
  releases/
    20260615-153000/
      app.jar
  current -> releases/20260615-153000   # Linux: symlink
```

**Windows**：

- `current` 使用**目录联接（junction）**指向 `releases\<version>`；切换时先删除旧 junction 再创建（实现见 [技术设计 §7](./deploy-hub-technical-design.md#7-远程协议实现)）。
- 更新 `current` 与上传产物**全部成功**后，才执行 restart。

#### 部署取消（V1）

- 用户可在 **building / uploading / restarting** 阶段点击「取消」。
- 取消后：
  - 本地构建子进程终止；
  - 已上传的**当前版本目录**保留（不自动删除）；
  - 部署记录状态为 **`canceled`**；
  - **不触发自动回滚**；
  - 若 restart 已发起，则等待远程命令结束或超时后标记 canceled，并提示「可能已部分重启，请人工检查」。

#### 任务并发

- 全局**同一时刻仅 1 个**部署任务执行。
- 另有任务进行中时，新的部署请求提示「当前有部署进行中」并拒绝。

#### 超时与上传进度

| 阶段 | 默认超时 | 可配置 |
|------|----------|--------|
| 构建 | `build.timeoutSec` = 600s | 项目级 |
| 上传 | 1800s（30min） | V1 全局固定 |
| 远程 restart | 300s | V1 全局固定 |

- 上传阶段 UI 展示**字节进度**与百分比（按已传/总大小）；V1 **不支持**断点续传，失败需全量重传。
- 超时后终止当前阶段，按失败策略进入对应终态（上传超时视为上传失败）。

### 4. 统一 restart 脚本规范

每个项目必须在服务器上预先放置统一启动入口：

- Linux：`restart.sh`
- Windows：`restart.bat` 或 `restart.ps1`

工具只负责上传产物、传入参数、执行脚本，不内置业务启动逻辑。

#### 脚本位置（V1）

- restart 脚本位于 **`remoteBaseDir` 根目录**（非版本子目录），例如：
  - Linux: `/opt/deploy-hub/apps/demo/restart.sh`
  - Windows: `C:\deploy-hub\apps\demo\restart.ps1`
- 工具**不上传** restart 脚本；由用户在服务器预先放置或由首次部署前手动维护。
- 部署前校验该路径存在且可执行。

#### 参数传递

工具以**命令行参数**调用（V1 不使用环境变量传参）：

**Linux：**

```bash
/path/to/restart.sh \
  --app demo \
  --version-dir /opt/deploy-hub/apps/demo/releases/20260615-153000 \
  --current-dir /opt/deploy-hub/apps/demo/current \
  --artifact /opt/deploy-hub/apps/demo/releases/20260615-153000/app.jar
```

**Windows：**

```powershell
C:\deploy-hub\apps\demo\restart.ps1 `
  -App demo `
  -VersionDir C:\deploy-hub\apps\demo\releases\20260615-153000 `
  -CurrentDir C:\deploy-hub\apps\demo\current `
  -Artifact C:\deploy-hub\apps\demo\releases\20260615-153000\app.jar
```

- 路径在 Windows 上统一使用**反斜杠**传入；脚本内部自行处理。
- 多个产物时 `--artifact` / `-Artifact` **重复多次**传入。

脚本职责：

- 停止旧进程或服务。
- 应用新产物。
- 启动新进程或服务。
- 返回明确退出码。
- 输出可读日志。
- 可选执行健康检查。

#### 健康检查（V1）

- 健康检查逻辑**全部由脚本实现**；工具**仅依据进程退出码**判断成功/失败。
- 退出码 `3` 表示健康检查失败。

退出码约定：

- `0`：成功。
- `1`：启动失败。
- `2`：配置错误。
- `3`：健康检查失败。
- 其他：未知错误。

#### 幂等性要求

- 对**同一 `version-dir` 重复执行** restart 应安全（回滚场景会触发二次 restart）。
- 脚本不得因「已是当前版本」而返回非 0，除非确实无法服务。

#### 最小示例（Java jar）

**Linux `restart.sh`：**

```bash
#!/usr/bin/env bash
set -euo pipefail
APP=""; VERSION_DIR=""; CURRENT_DIR=""; ARTIFACTS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --app) APP="$2"; shift 2 ;;
    --version-dir) VERSION_DIR="$2"; shift 2 ;;
    --current-dir) CURRENT_DIR="$2"; shift 2 ;;
    --artifact) ARTIFACTS+=("$2"); shift 2 ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac
done
cp -f "${ARTIFACTS[0]}" "$CURRENT_DIR/app.jar"
systemctl restart "$APP" || exit 1
exit 0
```

**Windows `restart.ps1`：**

```powershell
param([string]$App, [string]$VersionDir, [string]$CurrentDir, [string[]]$Artifact)
Copy-Item -Force $Artifact[0] (Join-Path $CurrentDir "app.jar")
try {
    Restart-Service -Name $App -ErrorAction Stop
} catch {
    exit 1
}
exit 0
```

以上仅为最小参考；生产脚本由用户按实际进程管理方式扩展。

脚本模板库见 [templates/](./templates/README.md)（Java / Nginx / Windows 静态等）。

### 5. 失败与回滚

每个项目可配置失败策略：

- **保留现场**：失败后停止部署，不回滚，保留远端版本目录和完整日志。
- **自动回滚**：如果 restart 阶段失败，尝试切回上一成功版本并执行重启脚本。

#### 回滚触发条件矩阵

| 失败阶段 | 保留现场 | 自动回滚 |
|----------|----------|----------|
| 部署前校验失败 | 不部署 | 不部署 |
| 构建失败 | 停止，无远端变更 | **不回滚**（无新版本落盘） |
| 上传失败 | 停止，远端可能残留**未完成**版本目录 | **不回滚**（`current` 未切换） |
| `current` 切换失败 | 停止，保留新版本目录 | 尝试保持 `current` 不变；若已部分切换则提示人工介入 |
| restart 退出码非 0 | 停止，`current` 可能已指向新版本 | **触发回滚**（若存在上一成功版本） |
| 用户取消 | `canceled` | **不回滚** |

仅当 **`failureStrategy = rollback`** 且失败发生在 **restart 阶段（含退出码 1/2/3/其他）** 时尝试自动回滚。

#### 「上一成功版本」定义

- 从部署记录中查询同一 `projectId` + `serverId`，`status = success`，按 `finishedAt` **降序**取第一条。
- 其 `remoteVersionDir` 必须仍存在于远端；若目录已被人工删除，则回滚失败，状态 **`rollback_failed`**。

#### 回滚执行行为

1. 将 `current` 指回上一成功版本的 `remoteVersionDir`。
2. 使用**同一 restart 脚本**与**上一版本目录**作为 `--version-dir` / `-VersionDir` 再次执行。
3. 生成**新的部署记录**：
   - `status`：回滚后 restart 成功为 **`rolled_back`**，失败为 **`rollback_failed`**；
   - 记录 `rolledBackFrom`（失败的那次 version）、`failureReason`（保留原始失败原因）。
4. UI **必须同时展示**：原始失败原因 + 回滚结果，不得覆盖隐藏。

自动回滚要求：

- 至少存在一个上一成功版本。
- 远端部署目录采用版本化结构。
- `restart` 脚本支持用旧版本目录重新启动。
- 回滚失败时必须明确提示，不隐藏原始失败原因。

#### 保留现场时的远端残留

- 上传失败或 restart 失败且策略为 **keep** 时，已创建的 `releases/<version>/` 目录**保留**在远端。
- 对应部署记录状态为 `failed`，`remoteVersionDir` 指向该目录，历史记录可查看日志。
- `current` 未成功切换时，线上服务不受影响；若已切换但 restart 失败，服务可能处于中间态，UI 须醒目提示。

### 6. 桌面 UI

主要页面：

- **仪表盘**：最近部署、成功/失败状态、快捷部署入口。
- **项目管理**：项目列表、项目配置、构建配置、部署配置。
- **服务器管理**：服务器列表、连接测试、凭据配置。
- **部署执行页**：构建日志、上传进度、远程执行日志、最终结果。
- **历史记录**：每次部署的项目、服务器、版本、状态、耗时、日志位置。
- **设置**：工作区路径、GitHub token、默认构建工具路径、日志保留策略。

UI 风格应偏运维工具：

- 信息密度适中。
- 明确状态色。
- 日志区域可搜索、复制、导出。
- 危险操作如删除服务器、清理版本、覆盖部署前必须二次确认。

#### 页面交互约定（V1）

**仪表盘**

| 元素 | 行为 |
|------|------|
| 最近 10 条部署 | 点击跳转历史详情 |
| 快捷部署 | 下拉选项目 → 进入部署执行页 |
| 状态色 | 成功绿 / 失败红 / 进行中蓝 / 取消灰 |

**项目管理**

| 操作 | 说明 |
|------|------|
| 新建/编辑/删除 | 删除前二次确认；删除不级联删除历史记录 |
| 必填项 | 名称、类型、来源、构建命令、产物路径、服务器、`remoteBaseDir`、`restartScript`、失败策略 |
| 测试构建 | V1 **不提供**独立试构建；仅完整部署流程 |

**部署执行页**

- 日志**分栏**：「构建」「上传」「远程执行」三栏；每栏实时追加，支持暂停自动滚动。
- 按钮：**部署**（未运行时）、**取消**（运行中）、**关闭**（结束后）。
- 失败时顶部展示一行用户可读摘要，可展开「技术详情」（含 `failureStep`、`failureReason`）。

**历史记录**

| 操作 | V1 支持 |
|------|---------|
| 查看日志 | 是 |
| 用相同配置重新部署 | 是（新建一条部署记录，非原地重试） |
| 打开远端版本目录 | 否（仅显示路径文本，可复制） |
| 删除历史记录 | 是（不删除远端目录与日志文件，仅删索引） |

**设置默认值**

| 设置项 | 默认值 |
|--------|--------|
| `workspaceRoot` | Windows: `%USERPROFILE%/deploy-hub-workspace`；Linux: `~/deploy-hub-workspace` |
| GitHub token | 全局 1 个，存钥匙串；所有 GitHub 项目共用，UI 可覆盖为手动输入 |
| 日志保留 | 最近 **100** 条部署记录索引；日志文件保留 **30** 天 |
| 构建工具路径 | 空（使用系统 `PATH`） |

#### 错误提示规范

- 面向用户：一句话说明「发生了什么」和「建议下一步」。
- 技术详情：默认折叠，含底层错误码、命令退出码、远端 stderr 摘要。
- V1 **不做**多语言；界面与错误文案使用简体中文。

## 关键接口 / 数据结构

JSON 字段约束见 [schemas/](./schemas/)（`project-config`、`server-config`、`deployment-record`）。实现层校验应与此保持一致。

### 项目配置

```json
{
  "schemaVersion": 1,
  "id": "app-demo",
  "name": "Demo App",
  "type": "java-maven",
  "source": {
    "kind": "github",
    "repoUrl": "https://github.com/org/repo.git",
    "ref": "main",
    "localPath": null
  },
  "build": {
    "location": "local",
    "workingDir": ".",
    "command": "mvn clean package -DskipTests",
    "artifactPath": "target/*.jar",
    "artifactMatchPolicy": "fail-if-multiple",
    "env": {},
    "timeoutSec": 600
  },
  "deploy": {
    "serverId": "prod-linux-1",
    "remoteBaseDir": "/opt/deploy-hub/apps/demo",
    "restartScript": "restart.sh",
    "failureStrategy": "rollback"
  }
}
```

`failureStrategy` 取值：`keep`（保留现场）| `rollback`（自动回滚）。

Local 来源示例：`"kind": "local"`, `"localPath": "D:/projects/demo"`，省略 `repoUrl`/`ref`。

### 服务器配置

```json
{
  "schemaVersion": 1,
  "id": "prod-linux-1",
  "name": "Prod Linux 1",
  "os": "linux",
  "host": "192.168.1.10",
  "port": 22,
  "username": "deploy",
  "auth": {
    "mode": "system-keychain",
    "credentialRef": "deploy-hub/prod-linux-1",
    "sshPrivateKeyPath": null
  },
  "defaultRemoteBaseDir": "/opt/deploy-hub",
  "ssh": {
    "knownHostsPolicy": "strict-with-prompt"
  },
  "winrm": null
}
```

Windows 服务器补充 `winrm` 字段：

```json
"winrm": {
  "scheme": "https",
  "tlsVerify": true,
  "trustedCertFingerprint": null
}
```

### 部署记录

```json
{
  "schemaVersion": 1,
  "id": "deploy-20260615-001",
  "projectId": "app-demo",
  "serverId": "prod-linux-1",
  "version": "20260615-153000",
  "remoteVersionDir": "/opt/deploy-hub/apps/demo/releases/20260615-153000",
  "artifactNames": ["app.jar"],
  "status": "success",
  "failureStep": null,
  "failureReason": null,
  "rolledBackFrom": null,
  "startedAt": "2026-06-15T15:30:00+08:00",
  "finishedAt": "2026-06-15T15:31:20+08:00",
  "logPath": "logs/deploy-20260615-001.log"
}
```

失败且回滚成功示例：`status: "rolled_back"`, `failureStep: "restarting"`, `failureReason: "restart exited with code 1"`, `rolledBackFrom: "20260615-153000"`。

`logPath` 为相对于应用数据目录（`dataDir`）的路径，例如 `logs/deploy-20260615-001.log`；完整路径为 `<dataDir>/logs/<deploy-id>.log`（见 [技术设计 §6.2](./deploy-hub-technical-design.md#62-日志文件)）。

状态枚举：

- `pending`
- `building`
- `uploading`
- `restarting`
- `success`
- `failed`
- `rollbacking`
- `rolled_back`
- `rollback_failed`
- `canceled`

#### 状态 UI 展示名

| 状态 | 展示名 | 颜色 |
|------|--------|------|
| `pending` | 等待中 | 灰 |
| `building` | 构建中 | 蓝 |
| `uploading` | 上传中 | 蓝 |
| `restarting` | 重启中 | 蓝 |
| `success` | 成功 | 绿 |
| `failed` | 失败 | 红 |
| `rollbacking` | 回滚中 | 橙 |
| `rolled_back` | 已回滚 | 橙 |
| `rollback_failed` | 回滚失败 | 红 |
| `canceled` | 已取消 | 灰 |

配置持久化与日志落盘见 [技术设计 §6](./deploy-hub-technical-design.md#6-存储设计)。

## 技术约束（摘要）

产品层面对技术的约束如下；完整实现见 [技术设计](./deploy-hub-technical-design.md)。

| 项 | V1 要求 |
|----|---------|
| 客户端技术栈 | C++ + Qt 6 Widgets |
| 存储 | SQLite + 独立日志文件 |
| 远程协议 | Linux SSH/SFTP；Windows WinRM |
| 并发 | 全局单部署任务 |
| 构建 | 调用本机工具，不内嵌构建器 |

### 安全要求（产品）

- 禁止明文保存密码、token、私钥 passphrase。
- SSH 默认校验 host key；首次未知须用户确认。
- GitHub token 仅用于拉取仓库；全局 1 个默认 token 存钥匙串，可选手动输入。
- 部署日志与 UI 须脱敏（规则见 [技术设计 §11.2](./deploy-hub-technical-design.md#112-logsanitizer)）。
- 高危操作二次确认：删除服务器、删除历史版本、覆盖远端目录。

### 构建与部署边界

- Deploy Hub 不替代 Maven、Node、Python 等构建工具，只调用用户本机已有工具。
- 构建命令在项目配置中可编辑。
- 产物匹配失败时部署中止。
- 所有远端命令输出进入部署日志。

## 测试计划

### 单元测试

- 项目配置解析与校验。
- 产物路径匹配。
- 版本目录生成。
- 部署状态流转。
- 失败策略选择。
- 敏感日志脱敏。

### 集成测试

- Local 项目构建并部署到 Linux 测试机。
- GitHub 项目拉取指定分支后构建。
- Linux SSH 上传文件并执行 `restart.sh`。
- Windows WinRM 执行 PowerShell 命令和 `restart.ps1`。
- 构建失败、上传失败、启动失败、回滚成功、回滚失败场景。

### P0 验收表

| ID | 场景 | 验收标准 |
|----|------|----------|
| T1 | Java → Linux | 状态 `success`，`current` 指向新版本，进程可访问 |
| T2 | 前端静态 → Windows | 状态 `success`，静态文件在目标目录可访问 |
| T3 | 构建失败 | 状态 `failed`，`failureStep=building`，无远端新版本 |
| T4 | 上传失败 | 状态 `failed`，`current` 未变 |
| T5 | restart 失败 + 回滚 | 原始失败可见，终态 `rolled_back`，服务恢复上一版 |
| T6 | 无上一版时回滚 | 状态 `rollback_failed`，原因明确 |
| T7 | 保留现场 | 失败后不回滚，失败版本目录可人工检查 |
| T8 | 手动凭据 | 不落盘，下次部署再次提示 |
| T9 | 取消部署 | 状态 `canceled`，行为符合取消约定 |
| T10 | 多 jar 匹配 | 默认 `fail-if-multiple`，列出候选路径 |

测试环境搭建与 CI 流水线见 [技术设计 §12](./deploy-hub-technical-design.md#12-测试与-ci)。

### P1 补充验收

| ID | 场景 | 验收标准 |
|----|------|----------|
| T11 | 上传进度 | 上传阶段可见百分比，完成后与文件大小一致 |
| T12 | GitHub shallow clone | 首次拉取 main 成功，切换 tag 后构建成功 |
| T13 | 工作区脏改覆盖 | 本地仓库有未提交修改，部署前警告后仍可继续 |
| T14 | 保留现场残留目录 | 上传后 restart 失败，远端 `releases/<ver>` 仍存在 |
| T15 | 全局 GitHub token | 设置 token 后私有仓库拉取成功 |

### 手工验收场景

- 新增一台 Linux 服务器并测试连接成功。
- 新增一台 Windows 服务器并测试连接成功。
- 新增 Java Maven 项目，一键部署成功。
- 新增前端静态项目，部署到 Nginx 目录并重启/重载成功。
- 部署失败时选择保留现场，能查看完整日志。
- 部署失败时选择自动回滚，能恢复上一成功版本。
- 凭据选择「不保存」，每次部署时要求输入。

## 路线图

### V1：个人可用完整闭环

- Windows/Linux 桌面客户端。
- Linux SSH、Windows WinRM。
- Local/GitHub 项目来源。
- Java Maven、前端静态包、Python、自定义项目。
- 单应用单服务器部署。
- 统一 restart 脚本。
- 部署历史与日志。
- 系统钥匙串 + 手动输入凭据。
- 保留现场/自动回滚策略。

### V2：效率增强

> 依赖 V1 稳定的单机构建记录、`current` 切换与 restart 契约。

- 多服务器批量部署。（依赖：部署记录模型、单任务执行器可扩展为队列）
- 环境分组：dev/test/prod。（依赖：服务器/项目配置 schema 扩展）
- 部署模板。（依赖：项目配置 JSON 可复制）
- 健康检查配置。（依赖：工具侧解析脚本输出或 HTTP 探针）
- 日志搜索与导出增强。（依赖：日志文件落盘规范）
- 构建前/部署后 hook。（依赖：`DeployJob` 阶段钩子）

### V3：平台化能力

- 本地 Worker 或后台服务。
- 定时部署。
- 权限与审计。
- 团队共享配置。
- Web 控制台。
- 插件化项目类型和远程协议。

## 假设与默认选择

- 仓库已包含 V1 工程骨架（CMake/Qt）、`AGENTS.md` 与 `agent_memory/`；产品规范以 `docs/` 为准，与代码并行演进。
- 首版不做团队账号、多租户、审批流、CI/CD 云端调度。
- 首版不做远程服务器构建。
- 首版不做单次多服务器发布。
- `wrm4j` 不作为默认实现路径，仅作为后续 Windows 远程适配备选。
- 上述 V1 决策在实现前若变更，须同步更新 `schemaVersion`、本文档、[技术设计](./deploy-hub-technical-design.md) 及 [JSON Schema](./schemas/) 版本记录。

## 附录

### A. 文档与决策

| 资源 | 路径 |
|------|------|
| 文档索引与术语表 | [docs/README.md](./README.md) |
| 技术设计 | [deploy-hub-technical-design.md](./deploy-hub-technical-design.md) |
| JSON Schema | [schemas/](./schemas/) |
| 架构决策记录 | [decisions/](./decisions/) |
| Linux SSH 搭建 | [guides/ssh-linux-setup.md](./guides/ssh-linux-setup.md) |
| Windows WinRM 搭建 | [guides/winrm-setup.md](./guides/winrm-setup.md) |
| restart 脚本模板 | [templates/](./templates/) |
| 本地开发说明 | [dev-setup.md](./dev-setup.md) |
| 项目约束 | [../AGENTS.md](../AGENTS.md) |

### B. `failureStrategy` 与 PRD 用语对照

| 配置值 | PRD 用语 |
|--------|----------|
| `keep` | 保留现场 |
| `rollback` | 自动回滚上一版 |

### C. 无障碍与国际化（V1）

- 界面语言：仅**简体中文**。
- V1 不单独做 WCAG 认证；遵循 Qt 控件默认键盘焦点与系统高对比度主题即可。
