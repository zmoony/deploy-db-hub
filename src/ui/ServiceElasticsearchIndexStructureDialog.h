#pragma once

#include "infra/ServiceNodeConnection.h"
#include <QDialog>

class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QComboBox;
class QLineEdit;
class QTextEdit;

class ServiceElasticsearchIndexStructureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServiceElasticsearchIndexStructureDialog(QWidget *parent = nullptr);

    void setEndpoint(const ServiceEndpoint &endpoint);
    void setIndexName(const QString &index);

    int exec() override;

private slots:
    void onAnalyzeText();

private:
    enum Tab { TabMapping = 0, TabSettings, TabAliases, TabTemplates, TabAnalyze };

    void reloadTab(int index);
    void loadMapping();
    void loadSettings();
    void loadAliases();
    void loadTemplates();
    void refreshAnalyzerList();

    static QString prettyJson(const QString &raw);

    ServiceEndpoint m_endpoint;
    QString m_index;
    QString m_cachedSettings;
    QStringList m_analyzers;

    QLabel *m_metaLabel = nullptr;
    QTextEdit *m_metaValue = nullptr;
    QTabWidget *m_tabs = nullptr;
    QPlainTextEdit *m_mappingView = nullptr;
    QPlainTextEdit *m_settingsView = nullptr;
    QTableWidget *m_aliasesTable = nullptr;
    QTableWidget *m_templatesTable = nullptr;

    QComboBox *m_analyzerCombo = nullptr;
    QLineEdit *m_analyzeTextInput = nullptr;
    QPushButton *m_analyzeButton = nullptr;
    QTableWidget *m_tokensTable = nullptr;
    QLabel *m_analyzeHintLabel = nullptr;
    QLabel *m_analyzeStatusLabel = nullptr;

    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_closeButton = nullptr;
};
