#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTreeWidget;

struct HttpHistoryEntry {
    QString group;
    QString name;
    QString method;
    QString url;
    QString headers;
    QString body;
};

class HttpRequestWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit HttpRequestWidget(QWidget *parent = nullptr);

private:
    void addHeaderRow(const QString &key = QString(), const QString &value = QString());
    QString serializeHeaders() const;
    void applyHeaders(const QString &serialized);
    void sendRequest();
    HttpHistoryEntry currentEntry() const;
    void applyEntry(const HttpHistoryEntry &entry);
    void loadHistory();
    void saveHistory();
    void refreshHistoryTree();
    QString historyFilePath() const;

    QComboBox *m_method = nullptr;
    QLineEdit *m_url = nullptr;
    QLineEdit *m_group = nullptr;
    QTableWidget *m_headers = nullptr;
    QPlainTextEdit *m_body = nullptr;
    QPlainTextEdit *m_response = nullptr;
    QTreeWidget *m_historyTree = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_status = nullptr;

    QVector<HttpHistoryEntry> m_history;
};
