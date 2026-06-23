#pragma once

#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;

class CommonToolsWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit CommonToolsWidget(QWidget *parent = nullptr);
    QStringList toolLabels() const;
    QWidget *takeToolPage(int index);
    int toolPageCount() const;

private:
    QWidget *buildTextToolPage(const QString &title,
                               const QString &subtitle,
                               const QString &primaryAction,
                               const QString &secondaryAction,
                               const QString &placeholder);
    QWidget *buildHttpRequestPage();
    QWidget *buildJsonViewerPage();
    QWidget *buildRegexPage();
    QWidget *buildTimestampPage();
    QWidget *buildHtmlEntityPage();
    QWidget *buildHttpStatusPage();
    QWidget *buildDiffPage();
    QWidget *buildCronPage();
    QWidget *buildImageBase64Page();
    QWidget *buildWebSocketPage();
    QWidget *buildUuidPage();
    QWidget *buildHashPage();
    QWidget *buildUrlCodecPage();
    QWidget *buildBase64TextPage();
    QWidget *buildJwtPage();
    QWidget *buildNumberBasePage();
    QWidget *buildCasePage();
    void setOutput(QPlainTextEdit *output, QLabel *message, const QString &text, const QString &error);

    QStackedWidget *m_stack = nullptr;
};
