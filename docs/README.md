# Deploy Hub 文档索引

> 最后更新：2026-06-15

## 文档地图

| 文档 | 读者 | 内容 |
|------|------|------|
| [deploy-hub-prd.md](./deploy-hub-prd.md) | 产品、测试、业务方 | 做什么：用户、功能、UI、数据契约、验收 |
| [deploy-hub-technical-design.md](./deploy-hub-technical-design.md) | 开发、架构 | 怎么做：模块、存储、协议、CI、安全实现 |
| [schemas/](./schemas/) | 开发、工具链 | JSON Schema，配置校验与代码生成参考 |
| [decisions/](./decisions/) | 全员 | 架构决策记录（ADR） |
| [guides/](./guides/) | 运维、验收 | 目标机 SSH / WinRM 环境搭建 |
| [templates/](./templates/) | 运维、开发 | restart 脚本模板（Java / Nginx / 静态） |
| [dev-setup.md](./dev-setup.md) | 开发 | 本地依赖、构建与测试命令 |
| [../AGENTS.md](../AGENTS.md) | 开发 | 项目特有约束与编码约定 |

## 阅读顺序建议

1. **了解产品** → PRD 摘要 + V1 成功标准
2. **准备服务器** → [guides/ssh-linux-setup.md](./guides/ssh-linux-setup.md) 或 [guides/winrm-setup.md](./guides/winrm-setup.md) + [templates/](./templates/)
3. **开始开发** → [dev-setup.md](./dev-setup.md) + 技术设计全文 + `schemas/`
4. **验收** → PRD 测试计划（P0/P1 表）

## 术语表

| 术语 | 含义 |
|------|------|
| Deploy Hub | 本桌面部署工具 |
| `remoteBaseDir` | 远端应用部署根目录，含 `releases/` 与 `current` |
| `current` | 指向当前线上版本的 symlink（Linux）或 junction（Windows） |
| `releases/<version>/` | 单次部署的版本目录，`version` 格式 `YYYYMMDD-HHmmss` |
| restart 脚本 | 用户预置在 `remoteBaseDir` 的 `restart.sh` / `restart.ps1` |
| 保留现场 | 失败策略：不回滚，保留远端状态与日志 |
| 自动回滚 | 失败策略：restart 失败时切回上一 `success` 版本 |
| `credentialRef` | 钥匙串中凭据的逻辑引用，非明文 |
| `schemaVersion` | 配置 JSON 的结构版本，用于迁移 |

## 版本同步

| 文档 | 当前版本 |
|------|----------|
| PRD | 1.6 |
| 技术设计 | 1.3 |
| JSON Schema | 1.2 |
| 搭建指南 | 1.0 |
| 脚本模板 | 1.0 |

变更配置结构时，须同时更新：`schemaVersion`、PRD 示例、对应 `.schema.json`、技术设计 §6。
