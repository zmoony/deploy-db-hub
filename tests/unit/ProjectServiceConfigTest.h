#pragma once

#include <QObject>

class ProjectServiceConfigTest final : public QObject
{
    Q_OBJECT

private slots:
    void derivesServiceMatchFromProjectId();
    void buildsDefaultTargetJarAndBackupPath();
    void rendersCommandPlaceholders();
    void pgrepSafePatternAvoidsSelfMatch();
    void buildsLinuxStatusCommandWithBracketPattern();
    void buildsLinuxStatusCommandForTargetJar();
    void buildsLinuxStatusCommandForLatestJarInRemoteBaseDir();
    void buildsWindowsStatusCommandExcludesShell();
    void detectsLocalRestartScriptPath();
    void buildsRestartPlanForLocalScript();
    void prefersCustomServiceControlOverRestartScript();
    void honorsRestartScriptModeExplicitly();
    void wrapsLinuxCommandWithWorkingDirectory();
    void wrapsWindowsCommandWithWorkingDirectory();
    void doesNotWrapWhenWorkingDirEmpty();
    void backupPathIsJoinedUnderRemoteBaseDir();
    void backupPathAcceptsAbsoluteLookingInputAsSubdir();
};
