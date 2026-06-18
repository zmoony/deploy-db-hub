#include "ui/PathPickerWidget.h"

#include "ui/PageLayout.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

PathPickerWidget::PathPickerWidget(Mode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
{
    PageLayout::configurePathField(this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    m_path = new QLineEdit(this);
    m_path->setMinimumHeight(PageLayout::DialogFieldHeight);
    layout->addWidget(m_path, 1);

    auto *browseButton = new QPushButton(QStringLiteral("浏览..."), this);
    browseButton->setFixedWidth(72);
    browseButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(browseButton, &QPushButton::clicked, this, &PathPickerWidget::browse);
    layout->addWidget(browseButton);
}

void PathPickerWidget::setPath(const QString &path)
{
    m_path->setText(path);
}

QString PathPickerWidget::path() const
{
    return m_path->text().trimmed();
}

void PathPickerWidget::setPlaceholderText(const QString &text)
{
    m_path->setPlaceholderText(text);
}

void PathPickerWidget::setBrowseHandler(std::function<void()> handler)
{
    m_browseHandler = std::move(handler);
}

void PathPickerWidget::browse()
{
    if (m_browseHandler) {
        m_browseHandler();
        return;
    }

    QString selected;
    if (m_mode == Mode::Directory) {
        selected = QFileDialog::getExistingDirectory(this, QStringLiteral("选择目录"), m_path->text());
    } else {
        selected = QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), m_path->text());
    }

    if (!selected.isEmpty()) {
        m_path->setText(selected);
    }
}
