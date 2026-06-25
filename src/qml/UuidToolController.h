#pragma once

#include <QObject>
#include <QString>

class UuidToolController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(bool uppercase READ uppercase WRITE setUppercase NOTIFY uppercaseChanged)
    Q_PROPERTY(bool withoutDashes READ withoutDashes WRITE setWithoutDashes NOTIFY withoutDashesChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)

public:
    explicit UuidToolController(QObject *parent = nullptr);

    int count() const;
    void setCount(int count);
    bool uppercase() const;
    void setUppercase(bool value);
    bool withoutDashes() const;
    void setWithoutDashes(bool value);
    QString output() const;

    Q_INVOKABLE void generate();
    Q_INVOKABLE void copyAll();

signals:
    void countChanged();
    void uppercaseChanged();
    void withoutDashesChanged();
    void outputChanged();

private:
    int m_count = 5;
    bool m_uppercase = false;
    bool m_withoutDashes = false;
    QString m_output;
};
