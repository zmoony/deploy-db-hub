# ADR-002: 静态部署采用 current symlink，restart 仅 reload

- **状态**：已接受
- **日期**：2026-06-15
- **决策者**：文档 Code Review（C1）

## 背景

PRD 规定上传完成后**先切换 `current`**（symlink/junction 指向 `releases/<version>`），再执行 restart。原 Nginx/静态模板在 restart 阶段使用 rsync/robocopy 将 `version-dir` 同步到 `current-dir`，切换后两路径为同一目录，导致语义冲突与 T2 验收风险。

## 决策

采用 **方案 A**：

1. Deploy Hub 负责上传至 `releases/<version>/` 并切换 `current`。
2. **静态 / Nginx** restart 脚本**仅**执行服务 reload（`nginx -t && reload` 或 IIS AppPool 回收），**不**在 restart 阶段复制文件。
3. **Java** restart 仍可将 jar 复制/链接到 `current/` 下约定文件名，供 systemd 使用。

## 后果

- PRD Nginx 示例、`restart-linux-nginx.sh`、`restart-windows-static.ps1` 已同步修订。
- Nginx `root` 必须指向 `current`（或 `current` 下子路径），由用户预配置。

## 关联

- PRD §前端静态包示例、§3 步骤 7
- [templates/README.md](../templates/README.md)
