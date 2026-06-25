#include "ui/PathPickerWidget.h"

#include "ui/PageLayout.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>

PathPickerWidget::PathPickerWidget(Mode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    PageLayout::configurePathField(this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space8);

    m_path = new QLineEdit(this);
    m_path->setMinimumHeight(PageLayout::DialogFieldHeight);
    m_path->setMinimumWidth(0);
    m_path->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_path, 1);

    auto *browseButton = new QPushButton(QStringLiteral("浏览..."), this);
    browseButton->setObjectName(QStringLiteral("pathBrowseButton"));
    browseButton->setMinimumWidth(52);
    browseButton->setFixedHeight(PageLayout::DialogFieldHeight);
    browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(browseButton, &QPushButton::clicked, this, &PathPickerWidget::browse);
    layout->addWidget(browseButton);
    connect(m_path, &QLineEdit::textChanged, this, &PathPickerWidget::pathChanged);
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

void PathPickerWidget::setMode(Mode mode)
{
    m_mode = mode;
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
