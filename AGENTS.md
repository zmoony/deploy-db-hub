# Deploy Hub 项目约束

## 项目定位

- 本项目实现 Deploy Hub V1：C++17 + Qt 6 Widgets 桌面部署工具。
- 产品与技术边界以 `docs/deploy-hub-prd.md`、`docs/deploy-hub-technical-design.md`、`docs/schemas/` 为准。
- Windows 远程默认路线为 WinRM + libcurl + 平台认证能力；Linux 远程默认路线为 libssh。

## 实现约定

- 构建系统使用 CMake 3.21+。
- UI 使用 Qt 6 Widgets。
- 单元测试使用 Qt Test 并接入 CTest。
- 配置结构字段名必须与 `docs/schemas/` 保持一致。
- 远程协议先通过接口隔离，真实 SSH/WinRM 实现不得泄漏到核心状态机。

## 验证

- 修改 C++ 行为代码时，优先补 Qt Test。
- 本机没有 Qt/CMake 时，至少做文本、JSON、路径与文档一致性验证，并在输出中明示未能构建。
