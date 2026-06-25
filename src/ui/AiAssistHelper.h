#pragma once

#include "infra/AiSettingsStore.h"

class CredentialStore;
class QString;

namespace AiAssistHelper {

bool resolveCredentials(AiSettingsStore *aiSettings,
                        CredentialStore *credentials,
                        AiSettings *settings,
                        QString *apiKey,
                        QString *error);

QString prepareLogForAiAnalysis(const QString &rawLog);
QString deploymentLogAnalysisSystemPrompt();

} // namespace AiAssistHelper
