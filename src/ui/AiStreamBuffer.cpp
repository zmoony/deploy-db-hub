#include "ui/AiStreamBuffer.h"

#include <QTextCursor>

namespace {

// Coalesce chunks within this window; ~30ms gives smooth visual streaming
// (≈30Hz) without overwhelming the widget with per-token repaints.
constexpr int kFlushIntervalMs = 30;

}

AiStreamBuffer::AiStreamBuffer(QPlainTextEdit *target, QObject *parent)
    : QObject(parent)
    , m_target(target)
{
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(kFlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &AiStreamBuffer::flush);
}

void AiStreamBuffer::append(const QString &chunk)
{
    if (chunk.isEmpty() || m_target.isNull()) {
        return;
    }
    m_pending.append(chunk);
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

void AiStreamBuffer::flush()
{
    m_flushTimer.stop();
    if (m_pending.isEmpty() || m_target.isNull()) {
        return;
    }
    const QString text = m_pending;
    m_pending.clear();

    QPlainTextEdit *target = m_target.data();
    QTextCursor cursor = target->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    target->setTextCursor(cursor);
    target->ensureCursorVisible();
}

void AiStreamBuffer::reset()
{
    m_flushTimer.stop();
    m_pending.clear();
}
