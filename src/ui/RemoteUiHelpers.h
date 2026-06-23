#pragma once

#include "adapters/remote/RemoteConnection.h"

#include <QWidget>

HostKeyPromptHandler makeHostKeyPromptHandler(QWidget *parent);
HostKeyPromptHandler makeThreadSafeHostKeyPromptHandler(QWidget *parent);
