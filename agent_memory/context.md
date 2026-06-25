# 项目背景

- 项目：Deploy Hub，可视化桌面部署工具。
- 技术方向：C++17 + Qt 6 Widgets，CMake，SQLite，本地构建后部署。
- V1 目标：Windows/Linux 桌面客户端；Linux 目标机走 SSH/SFTP；Windows 目标机走 WinRM；项目来源支持 Local/GitHub；全局同一时刻仅 1 个部署任务。
- 远程协议决策：SSH/SFTP 固定使用 libssh；WinRM 固定使用 libcurl + 平台认证能力（Windows SSPI，Linux libcurl 可用的 GSSAPI/NTLM 后端）；wrm4j 不作为主路径。
- 安全方向：凭据走 Qt Keychain/平台钥匙串；手动凭据仅进程内缓存；日志脱敏。
- 构建：Release 构建脚本 `scripts/build-release.ps1`（Qt 6.11.1 mingw_64，输出 `build-release/deploy-hub.exe`）；用户要求修改 C++ 代码后默认执行编译验证。
