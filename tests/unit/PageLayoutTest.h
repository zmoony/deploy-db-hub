#pragma once

#include <QObject>

class PageLayoutTest final : public QObject
{
    Q_OBJECT

private slots:
    void fitWindowToScreenCentersWindow();
    void pageTransferDoesNotShowDetachedPages();
    void desktopShellTokensMatchReferenceLayout();
    void sidebarNavigationRemainsModuleScoped();
    void sidebarNavigationUsesIcons();
    void aiConfigPageUsesUnifiedTemplate();
    void aiChatPageMatchesPixsoLayout();
    void logAiAnalysisShowsResultFirst();
    void projectManagerSelectionIsProminent();
    void contentButtonsStayCompact();
    void serviceModuleButtonsKeepConsistentSize();
    void projectServiceControlCopyExplainsRemoteCommands();
    void legacyGroupBoxCardsUseDeploySectionTitleStyle();
    void deployPageMatchesStitchLayout();
};
