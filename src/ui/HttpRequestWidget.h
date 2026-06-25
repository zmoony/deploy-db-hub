#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QButtonGroup;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QListWidget;

struct HttpHistoryEntry {
    QString group;
    QString name;
    QString method;
    QString url;
    QString headers;
    QString params;
    QString body;
    QString authType;
    QString authValue;
    int statusCode = 0;
    qint64 elapsedMs = 0;
};

class HttpRequestWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit HttpRequestWidget(QWidget *parent = nullptr);

private:
    void addKeyValueRow(QTableWidget *table, const QString &key = QString(), const QString &value = QString());
    void removeSelectedKeyValueRow(QTableWidget *table);
    QString serializeKeyValueTable(const QTableWidget *table) const;
    void applyKeyValueTable(QTableWidget *table, const QString &serialized);
    QString buildRequestUrl() const;
    void applyMethodStyle();
    void sendRequest();
    void exportRequestJson();
    HttpHistoryEntry currentEntry() const;
    void applyEntry(const HttpHistoryEntry &entry);
    void loadHistory();
    void saveHistory();
    void refreshHistoryList();
    void updateLatestHistoryResult(int statusCode, qint64 elapsedMs);
    QString historyFilePath() const;
    QWidget *makeCapsuleTabBar(const QStringList &labels, QStackedWidget *stack, QButtonGroup **groupOut);
    QWidget *makeKeyValueSection(const QString &addLabel, QTableWidget **tableOut, QWidget *parent);

    QComboBox *m_method = nullptr;
    QLineEdit *m_url = nullptr;
    QTableWidget *m_headers = nullptr;
    QTableWidget *m_params = nullptr;
    QPlainTextEdit *m_body = nullptr;
    QComboBox *m_authType = nullptr;
    QLineEdit *m_authValue = nullptr;
    QPlainTextEdit *m_responseBody = nullptr;
    QPlainTextEdit *m_responseHeaders = nullptr;
    QPlainTextEdit *m_responseCookies = nullptr;
    QListWidget *m_historyList = nullptr;
    QLineEdit *m_historySearch = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_statusBadge = nullptr;
    QLabel *m_elapsedLabel = nullptr;
    QLabel *m_sizeLabel = nullptr;

    int m_lastStatusCode = 0;
    qint64 m_lastElapsedMs = 0;
    int m_lastSizeBytes = 0;
    QString m_lastResponseBody;
    QString m_lastResponseHeaders;
    QString m_lastResponseCookies;

    QVector<HttpHistoryEntry> m_history;
};
