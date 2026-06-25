#pragma once

#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QTimer>

// Buffers streaming delta tokens from the AI client and flushes them into a
// QPlainTextEdit in coalesced batches. This avoids doing
// moveCursor/insertPlainText for every single token (which would hammer the
// widget with repaint/layout events and make streaming feel laggy).
class AiStreamBuffer final : public QObject
{
    Q_OBJECT

public:
    explicit AiStreamBuffer(QPlainTextEdit *target, QObject *parent = nullptr);

    void append(const QString &chunk);
    void flush();
    void reset();

private:
    QPointer<QPlainTextEdit> m_target;
    QTimer m_flushTimer;
    QString m_pending;
};
