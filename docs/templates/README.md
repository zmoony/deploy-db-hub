# restart 脚本模板库

Deploy Hub **不会**上传 restart 脚本；部署前须在目标机 `remoteBaseDir` 下预置可执行脚本。

## 模板列表

| 文件 | 场景 | 平台 |
|------|------|------|
| [restart-linux-java.sh](./restart-linux-java.sh) | Java jar + systemd 服务 | Linux |
| [restart-linux-nginx.sh](./restart-linux-nginx.sh) | 前端静态包 + Nginx | Linux |
| [restart-windows-java.ps1](./restart-windows-java.ps1) | Java jar + Windows 服务 | Windows |
| [restart-windows-static.ps1](./restart-windows-static.ps1) | 静态文件目录 | Windows |

## 通用约定（与 PRD 一致）

工具以命令行参数调用：

**Linux**

```text
restart.sh --app <name> --version-dir <path> --current-dir <path> --artifact <path> [--artifact <path> ...]
```

**Windows**

```text
restart.ps1 -App <name> -VersionDir <path> -CurrentDir <path> -Artifact <path> [-Artifact <path> ...]
```

退出码：`0` 成功，`1` 启动失败，`2` 配置错误，`3` 健康检查失败。

## 静态 / Nginx 部署模型（symlink current）

Deploy Hub 将产物上传到 `releases/<version>/`，切换 `current` 指向该目录后执行 restart。此时 `--version-dir` 与 `--current-dir` **解析为同一目录**。

- **Nginx / 静态模板**：restart **仅 reload 服务**，不做 rsync/robocopy。
- **Java 模板**：仍将 jar 复制到 `current/<jar>`（`current` 已指向版本目录时，等价于确保文件名符合 systemd 配置）。

## 使用步骤

1. 复制模板到服务器 `remoteBaseDir`（如 `/opt/deploy-hub/apps/demo/restart.sh`）。
2. 修改脚本顶部 **CONFIG** 区域的变量（服务名、路径等）。
3. `chmod +x`（Linux）或设置执行策略（Windows）。
4. 在 Deploy Hub 项目配置中填写 `restartScript` 文件名。
5. 手动执行一次脚本（传入假参数）验证语法，再通过 Deploy Hub 连通性测试。

## 幂等性

同一 `--version-dir` 重复执行应安全（回滚场景会二次调用）。模板实现均支持重复执行。
