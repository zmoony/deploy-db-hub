# Windows 目标机 WinRM 环境搭建指南

> 适用：Deploy Hub V1 将应用部署到 Windows 服务器（WinRM HTTPS 5986）
>
> 关联：[PRD](../deploy-hub-prd.md) · [技术设计 §7.2](../deploy-hub-technical-design.md#72-winrmwindows-目标机) · [模板库](../templates/README.md)

## 1. 环境要求

| 项 | 最低要求 |
|----|----------|
| 系统 | Windows Server 2019+ 或 Windows 11 Pro/Enterprise |
| WinRM | HTTPS 监听器，端口 **5986** |
| 认证 | NTLM 或 Negotiate（域环境） |
| 账户 | 本地或域用户，具备目标目录写权限与重启服务权限 |
| 网络 | 客户端可访问 TCP 5986 |

V1 **不支持** Basic over HTTP（5985 仅建议本地开发调试）。

## 2. 快速启用 WinRM（测试环境）

在目标机 **以管理员身份** 打开 PowerShell：

```powershell
# 启用 WinRM 服务
Enable-PSRemoting -Force -SkipNetworkProfileCheck

# 查看监听器
winrm enumerate winrm/config/Listener
```

生产环境请使用下文 HTTPS 5986 步骤，勿长期依赖 HTTP 5985。

## 3. 配置 HTTPS 监听器（5986）

### 3.1 创建自签名证书（测试用）

```powershell
$dns = $env:COMPUTERNAME
$cert = New-SelfSignedCertificate -DnsName $dns -CertStoreLocation Cert:\LocalMachine\My
$thumbprint = $cert.Thumbprint
$thumbprint
```

### 3.2 创建 HTTPS 监听器

```powershell
winrm create winrm/config/Listener?Address=*+Transport=HTTPS "@{Hostname=`"$dns`";CertificateThumbprint=`"$thumbprint`"}"
```

确认：

```powershell
winrm enumerate winrm/config/Listener
# 应看到 Transport = HTTPS, Port = 5986
```

### 3.3 防火墙

```powershell
New-NetFirewallRule -DisplayName "WinRM HTTPS" -Direction Inbound -LocalPort 5986 -Protocol TCP -Action Allow
```

## 4. 认证与服务配置

```powershell
# 禁用 Basic（V1 要求）
Set-Item -Path WSMan:\localhost\Service\Auth\Basic -Value $false
Set-Item -Path WSMan:\localhost\Service\Auth\Negotiate -Value $true
Set-Item -Path WSMan:\localhost\Service\Auth\Kerberos -Value $true

# 允许未加密仅本地；远程必须 HTTPS
Set-Item -Path WSMan:\localhost\Service\AllowUnencrypted -Value $false

# 执行策略（允许远程脚本）
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine
```

## 5. 部署目录与 restart 脚本

```powershell
$base = "C:\deploy-hub\apps\demo"
New-Item -ItemType Directory -Force -Path "$base\releases" | Out-Null

# 从模板库复制 restart-windows-java.ps1 或 restart-windows-static.ps1 到 $base\restart.ps1 并按注释修改
# current junction 由 Deploy Hub 在部署时创建
```

目录结构（部署后）：

```
C:\deploy-hub\apps\demo\
  restart.ps1
  releases\
    20260615-153000\
      app.jar
  current\          # junction -> releases\20260615-153000
```

为部署用户授予目录写权限（示例本地用户 `deploy`）：

```powershell
icacls "C:\deploy-hub" /grant deploy:(OI)(CI)M /T
```

## 6. Deploy Hub 侧 TLS 信任

使用自签名证书时，首次连接 Deploy Hub 会提示证书指纹。在服务器配置中可持久化 `winrm.trustedCertFingerprint`（**SHA-1**，40 位十六进制，无冒号）：

```powershell
# 在目标机查看证书指纹
Get-ChildItem Cert:\LocalMachine\My | Format-List Subject, Thumbprint
```

生产环境建议使用受信任 CA 签发的证书并设置 `tlsVerify: true`。

## 7. 连通性自检

在**另一台 Windows 客户端**（已加入 WinRM 信任列表）测试：

```powershell
$cred = Get-Credential   # 输入 deploy 用户
Test-WSMan -ComputerName <HOST> -UseSSL -Credential $cred -Authentication Negotiate
Invoke-Command -ComputerName <HOST> -UseSSL -Credential $cred -Authentication Negotiate -ScriptBlock { hostname }
```

Linux 客户端上的 Deploy Hub 将通过内置 WinRM 客户端完成同等测试。

## 8. 在 Deploy Hub 中填写配置

| 字段 | 示例 |
|------|------|
| `os` | `windows` |
| `port` | `5986` |
| `username` | `deploy` 或 `.\deploy` |
| `winrm.scheme` | `https` |
| `winrm.tlsVerify` | `true`（自签名首次连接后信任指纹） |
| 项目 `remoteBaseDir` | `C:\deploy-hub\apps\demo` |
| 项目 `restartScript` | `restart.ps1` |

## 9. 常见问题

| 现象 | 可能原因 | 处理 |
|------|----------|------|
| 连接被拒绝 5986 | 监听器未创建或防火墙拦截 | 检查 `winrm enumerate` 与防火墙规则 |
| SSL 错误 | 证书主机名不匹配 | 证书 DNS 与连接主机名一致，或信任指纹 |
| 401 未授权 | 用户/密码错误或非管理员 WinRM 限制 | 将用户加入 `Remote Management Users` 组 |
| 上传失败 | 目录无写权限 | `icacls` 授权 |
| restart 失败 | 服务名错误或无重启权限 | 检查 `restart.ps1` 内服务名与账户权限 |

将用户加入远程管理组（管理员 PowerShell）：

```powershell
Add-LocalGroupMember -Group "Remote Management Users" -Member "deploy"
```

## 10. 安全建议（生产）

- 使用正式 TLS 证书，关闭 HTTP 5985 监听器。
- 限制来源 IP（防火墙仅允许运维网段）。
- 使用专用部署账户，最小权限原则。
- 定期轮换密码；Deploy Hub 侧使用钥匙串而非明文配置。
