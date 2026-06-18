# Deploy Hub UI and Deploy Flow Plan

## Goal
Implement server defaults, monitor sorting/search, remote terminal/file tabs, JDK selection, and deploy status/log fixes without changing remote protocol choices.

## Phases
- [x] Phase 1: Inspect current terminal, monitor, deploy, and config storage code.
- [x] Phase 2: Add tested support helpers for server defaults, JDK profile persistence, and JDK environment injection.
- [x] Phase 3: Update server config and monitor UI.
- [x] Phase 4: Update remote operation UI with terminal-first tabs and context actions.
- [x] Phase 5: Update deploy page JDK selector, password reuse, and log path behavior.
- [x] Phase 6: Build, test, package if needed, and update agent memory.

## Decisions
- Windows default server fields: username `administrator`, default deploy root `D:/psmp`.
- Linux default server fields: username `root`, default deploy root `/home`.
- JDK selector default: system environment; custom profiles persist locally for reuse.
- Remote operation default view: terminal tab; file list remains available as a full-height tab.

## Errors Encountered
| Error | Attempt | Resolution |
|---|---|---|
| `TerminalTextEdit` ambiguous/incomplete type during build | Class was defined in an anonymous namespace while the header forward-declared a global class | Moved the concrete `TerminalTextEdit` declaration to `RemoteTerminalWidget.h` and kept implementation in `.cpp` |
| New terminal input test expected one `"ls"` signal | `QTest::keyClicks` emits one key event per character | Updated the test to assert `"l"` and `"s"` as separate input byte signals |
