#pragma once

#include <QString>

namespace DataPaths {
void initialize();
QString configDir();
QString databaseFile();
QString logsDir();
QString workspaceDir();
QString credentialsFile();
QString jdkProfilesFile();
}
