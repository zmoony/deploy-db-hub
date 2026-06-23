# 已知问题

- 已确认 Qt/CMake/Ninja/MinGW 位于 `D:\systemInsall\Qt`，Release 构建、单元测试和 windeployqt 打包可运行。
- Stub 远程实现已移除；Linux 使用 OpenSSH CLI，Windows 使用 winrs。后续可切换为 libssh/libcurl 原生实现。
- 2026-06-17 已修复：Windows 客户端通过 OpenSSH 访问 Linux 时，若启用 `ControlMaster/ControlPath/ControlPersist`，服务器监控和远程文件页可能报 `getsockname failed: Not a socket`；现 Windows 下禁用连接复用参数。
- 2026-06-18 已修复：服务状态 `pgrep -f` 会匹配 SSH 执行检测命令时的 shell 自身，导致显示不存在的 PID/假运行；已改为 bracket pattern + PID 存活校验。
- 2026-06-22 已修复：启动时部署页自动刷新远程日志列表与服务状态会多次弹密码/host key 确认框；根因是构造函数、下拉联动和选择器刷新使用了允许 prompt 的远程刷新路径。现启动/自动联动改为静默刷新, 只有用户点击「刷新列表」「刷新状态」、查看日志或部署等主动操作才允许弹框。
- 2026-06-23 已修复：启动时若服务器密码已保存, `refreshServiceStatus(false)` 仍会同步 SSH 探测并阻塞构造; 静默模式现仅显示「未检测」。主界面模块 Tab 原位于侧栏上方整行, 已移入右侧内容区以匹配参考图左右布局。
- 2026-06-23 已修复：启动时出现多个临时窗口/弹框；根因是通用工具、大数据、数据库页面从旧 `QStackedWidget` 转移到主窗口前调用了 `page->show()`，以及 `PageLayout::wrapContentPanel` 在页面拥有主窗口父级前提前显示页面，导致 Qt 将其短暂显示为顶层窗口。现移除这些提前 `show()`，由主窗口布局统一显示。
