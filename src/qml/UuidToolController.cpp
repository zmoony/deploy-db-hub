#include "qml/UuidToolController.h"

#include "tools/CommonTools.h"

#include <QApplication>
#include <QClipboard>

UuidToolController::UuidToolController(QObject *parent)
    : QObject(parent)
{
}

int UuidToolController::count() const
{
    return m_count;
}

void UuidToolController::setCount(int count)
{
    const int clamped = qBound(1, count, 1000);
    if (m_count == clamped) {
        return;
    }
    m_count = clamped;
    emit countChanged();
}

bool UuidToolController::uppercase() const
{
    return m_uppercase;
}

void UuidToolController::setUppercase(bool value)
{
    if (m_uppercase == value) {
        return;
    }
    m_uppercase = value;
    emit uppercaseChanged();
}

bool UuidToolController::withoutDashes() const
{
    return m_withoutDashes;
}

void UuidToolController::setWithoutDashes(bool value)
{
    if (m_withoutDashes == value) {
        return;
    }
    m_withoutDashes = value;
    emit withoutDashesChanged();
}

QString UuidToolController::output() const
{
    return m_output;
}

void UuidToolController::generate()
{
    const QStringList ids = CommonTools::generateUuids(m_count, m_uppercase, m_withoutDashes);
    m_output = ids.join(QLatin1Char('\n'));
    emit outputChanged();
}

void UuidToolController::copyAll()
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(m_output);
    }
}
