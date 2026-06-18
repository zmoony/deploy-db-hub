# 已知问题

- 已确认 Qt/CMake/Ninja/MinGW 位于 `D:\systemInsall\Qt`，Release 构建、单元测试和 windeployqt 打包可运行。
- Stub 远程实现已移除；Linux 使用 OpenSSH CLI，Windows 使用 winrs。后续可切换为 libssh/libcurl 原生实现。
- 2026-06-17 已修复：Windows 客户端通过 OpenSSH 访问 Linux 时，若启用 `ControlMaster/ControlPath/ControlPersist`，服务器监控和远程文件页可能报 `getsockname failed: Not a socket`；现 Windows 下禁用连接复用参数。
- 2026-06-18 已修复：服务状态 `pgrep -f` 会匹配 SSH 执行检测命令时的 shell 自身，导致显示不存在的 PID/假运行；已改为 bracket pattern + PID 存活校验。
