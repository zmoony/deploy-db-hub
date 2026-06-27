#include "ui/tools/AiAssistBar.h"

#include <QHBoxLayout>
#include <QPushButton>

namespace Ui {
namespace Tools {

AiAssistBar::AiAssistBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("aiAssistBar"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_assistButton = new QPushButton(QStringLiteral("AI 辅助"), this);
    m_assistButton->setObjectName(QStringLiteral("toolAiAssistButton"));
    m_assistButton->setFixedHeight(28);
    connect(m_assistButton, &QPushButton::clicked, this, &AiAssistBar::assistClicked);
    layout->addWidget(m_assistButton);

    m_stopButton = new QPushButton(QStringLiteral("停止"), this);
    m_stopButton->setObjectName(QStringLiteral("toolAiStopButton"));
    m_stopButton->setFixedHeight(28);
    m_stopButton->setVisible(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AiAssistBar::stopClicked);
    layout->addWidget(m_stopButton);

    layout->addStretch();
}

QPushButton *AiAssistBar::assistButton() const { return m_assistButton; }
QPushButton *AiAssistBar::stopButton() const { return m_stopButton; }

void AiAssistBar::setRunning(bool running)
{
    m_assistButton->setVisible(!running);
    m_stopButton->setVisible(running);
}

} // namespace Tools
} // namespace Ui
