#pragma once

#include <QObject>

class RemoteTerminalWidgetTest final : public QObject
{
    Q_OBJECT

private slots:
    void terminalTextEditEmitsInteractiveInputBytes();
    void terminalStreamPreservesCrlfOutput();
    void terminalStreamOverwritesPromptOnBareCarriageReturn();
    void terminalStreamAppliesAnsiColors();
    void terminalStreamAppliesTrueColor();
    void terminalStreamStripsOscWindowTitle();
};
