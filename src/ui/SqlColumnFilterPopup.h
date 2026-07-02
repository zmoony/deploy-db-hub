#pragma once

#include <QFrame>
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

struct SqlColumnFilterState {
    QString searchText;
    QSet<QString> excludedValues;

    bool isActive() const { return !searchText.trimmed().isEmpty() || !excludedValues.isEmpty(); }
};

class QCheckBox;
class QLineEdit;
class QListWidget;
class QLabel;

class SqlColumnFilterPopup final : public QFrame
{
    Q_OBJECT

public:
    explicit SqlColumnFilterPopup(QWidget *parent = nullptr);

    void openForColumn(int column,
                       const QString &columnName,
                       const QVector<QPair<QString, int>> &valueCounts,
                       const SqlColumnFilterState &state,
                       const QPoint &globalAnchor);

signals:
    void filterApplied(int column, const SqlColumnFilterState &state);
    void filterCleared(int column);

private slots:
    void onSearchChanged(const QString &text);
    void onValueToggled();
    void onClear();

private:
    void rebuildValueList();
    void applyCurrentState();
    SqlColumnFilterState currentState() const;

    int m_column = -1;
    QString m_columnName;
    QVector<QPair<QString, int>> m_allValueCounts;
    QHash<QString, QCheckBox *> m_valueChecks;

    QLabel *m_titleLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_valueList = nullptr;
};
