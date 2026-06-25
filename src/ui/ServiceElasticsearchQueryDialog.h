#pragma once

#include "infra/ServiceNodeConnection.h"

#include <QDialog>
#include <QString>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

class ServiceElasticsearchQueryDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceElasticsearchQueryDialog(QWidget *parent = nullptr);

    void setEndpoint(const ServiceEndpoint &endpoint);
    void setIndexName(const QString &indexName);

private slots:
    void onQueryModeChanged(int index);
    void onSearch();
    void onCopyResult();

private:
    QJsonObject buildSearchBody(int *fromOut, int *sizeOut, QString *error) const;

    ServiceEndpoint m_endpoint;
    QLineEdit *m_indexName = nullptr;
    QComboBox *m_queryMode = nullptr;
    QPlainTextEdit *m_queryText = nullptr;
    QSpinBox *m_page = nullptr;
    QSpinBox *m_pageSize = nullptr;
    QPlainTextEdit *m_result = nullptr;
    QPushButton *m_searchButton = nullptr;
};
