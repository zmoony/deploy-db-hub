# Linux 目标机 SSH 环境搭建指南

> 适用：Deploy Hub V1 将应用部署到 Linux 服务器（SSH/SFTP）
>
> 关联：[PRD restart 规范](../deploy-hub-prd.md#4-统一-restart-脚本规范) · [模板库](../templates/README.md)

## 1. 环境要求

| 项 | 最低要求 |
|----|----------|
| 系统 | Ubuntu 22.04 LTS（其他发行版步骤类似） |
| 服务 | OpenSSH Server |
| 账户 | 专用部署用户（示例：`deploy`） |
| 网络 | 客户端可访问 TCP 22（或自定义 SSH 端口） |

## 2. 创建部署用户与目录

在目标机上以 root 或 sudo 执行：

```bash
sudo useradd -m -s /bin/bash deploy
sudo mkdir -p /opt/deploy-hub/apps/demo/releases
sudo chown -R deploy:deploy /opt/deploy-hub
```

若使用其他路径（如 `/var/www/myapp`），将 `remoteBaseDir` 与下文路径替换为实际值。

## 3. 安装并启动 SSH

```bash
sudo apt update
sudo apt install -y openssh-server
sudo systemctl enable --now ssh
sudo systemctl status ssh
```

## 4. 认证方式（二选一或组合）

### 4.1 密码认证

编辑 `/etc/ssh/sshd_config`（按需）：

```
PasswordAuthentication yes
```

重载：

```bash
sudo systemctl reload ssh
```

在 Deploy Hub 中使用钥匙串或「每次手动输入」保存密码。

### 4.2 公钥认证（推荐）

在**客户端**生成密钥（若尚无）：

```bash
ssh-keygen -t ed25519 -f ~/.ssh/deploy_hub_ed25519 -C "deploy-hub"
```

将公钥写入目标机：

```bash
ssh-copy-id -i ~/.ssh/deploy_hub_ed25519.pub deploy@<HOST>
```

Deploy Hub 服务器配置：

- `auth.mode`: `ssh-key`
- `sshPrivateKeyPath`: 私钥绝对路径
- 私钥有 passphrase 时，每次部署在 UI 输入（不落盘）

## 5. 预置目录结构

Deploy Hub 首次成功部署前，建议手动准备：

```bash
sudo -u deploy mkdir -p /opt/deploy-hub/apps/demo/releases
# current 由 Deploy Hub 在部署时创建 symlink，无需预先创建
```

最终结构（部署后）：

```
/opt/deploy-hub/apps/demo/
  restart.sh          # 用户预置，见模板库
  releases/
    20260615-153000/
      app.jar
  current -> releases/20260615-153000
```

## 6. 安装 restart 脚本

从 [templates/restart-linux-java.sh](../templates/restart-linux-java.sh) 或 [restart-linux-nginx.sh](../templates/restart-linux-nginx.sh) 复制到 `remoteBaseDir`，按注释修改后：

```bash
sudo cp restart-linux-java.sh /opt/deploy-hub/apps/demo/restart.sh
sudo chown deploy:deploy /opt/deploy-hub/apps/demo/restart.sh
sudo chmod +x /opt/deploy-hub/apps/demo/restart.sh
```

## 7. 连通性自检

在**客户端**执行（模拟 Deploy Hub 测试）：

```bash
ssh deploy@<HOST> "echo deploy-hub-ping"
ssh deploy@<HOST> "whoami && pwd"
ssh deploy@<HOST> "test -x /opt/deploy-hub/apps/demo/restart.sh && echo restart-ok"
```

Deploy Hub 中「连通性测试」「命令执行测试」应全部通过。

## 8. 首次 Host Key

Deploy Hub 首次连接未知主机时，UI 会展示 fingerprint 并要求确认。确认后写入本机 known_hosts（见技术设计 §7.1）。

命令行预信任（可选，仅测试环境）：

```bash
ssh-keyscan -H <HOST> >> ~/.ssh/known_hosts
```

## 9. 防火墙

```bash
sudo ufw allow OpenSSH
# 或自定义端口: sudo ufw allow 22/tcp
sudo ufw status
```

## 10. 常见问题

| 现象 | 可能原因 | 处理 |
|------|----------|------|
| 连接超时 | 防火墙/安全组未放行 22 | 检查云厂商安全组与 ufw |
| Permission denied | 密钥或密码错误 | 核对用户、`authorized_keys` |
| restart 脚本不存在 | 未预置或路径与配置不一致 | 对齐 `deploy.restartScript` 与 `remoteBaseDir` |
| SFTP 上传失败 | 目录无写权限 | `chown deploy:deploy` 整个 `remoteBaseDir` |
| Host key 变更 | 重装系统或 IP 复用 | 在客户端清理旧 known_hosts 条目后重新确认 |

## 11. 与 Deploy Hub 配置对照

| 服务器配置字段 | 示例值 |
|----------------|--------|
| `os` | `linux` |
| `port` | `22` |
| `username` | `deploy` |
| `defaultRemoteBaseDir` | `/opt/deploy-hub` |
| 项目 `remoteBaseDir` | `/opt/deploy-hub/apps/demo` |
| 项目 `restartScript` | `restart.sh` |
