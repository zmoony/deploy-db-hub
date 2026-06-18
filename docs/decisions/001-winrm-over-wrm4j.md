# ADR-001: Windows 远程优先 WinRM，wrm4j 不作为主路径

- **状态**：已接受
- **日期**：2026-06-15
- **决策者**：项目方案评审

## 背景

Windows 目标机需要通过远程协议执行 PowerShell、上传部署包。可选方案包括：

1. **WinRM / PowerShell 原生**（HTTPS + NTLM/Negotiate）
2. **wrm4j**（Java 生态 WinRM 客户端，可作为外部 helper）

## 决策

V1 默认实现 **基于 libcurl 的 WinRM 客户端**（见技术设计 §7.2），负责 WS-Management SOAP 请求、PowerShell 命令执行与分块上传。认证使用平台能力：Windows 走 SSPI，Linux 走 libcurl 可用的 GSSAPI/NTLM 后端。`wrm4j` 不作为主路径，仅在后续若 C++ WinRM 维护成本过高时评估为可选外部 helper。

## 理由

- 主客户端为 **C++ + Qt**，内嵌 WinRM 可减少 JVM 依赖与分发体积。
- WinRM 为 Windows 官方远程管理标准，与 PowerShell 脚本契约一致。
- `wrm4j` 引入 Java 运行时，与「个人桌面工具轻量分发」目标冲突。

## 后果

- 正面：单一技术栈、安装包更小、与 `restart.ps1` 路径一致；HTTP/TLS 与认证能力复用成熟库和平台实现。
- 负面：仍需实现 WinRM SOAP 消息、PowerShell 命令生命周期、分块上传与跨平台认证测试。
- 缓解：V1 验收包含「Linux 客户端 → Windows WinRM」端到端用例（PRD T2）。

## 关联

- PRD：摘要、`wrm4j` 范围外说明
- 技术设计：§7.2 WinRM
