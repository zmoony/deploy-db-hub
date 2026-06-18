#include "ui/JdkProfileDialog.h"

#include "infra/AppBranding.h"
#include "infra/DataPaths.h"
#include "ui/PageLayout.h"

#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

JdkProfileDialog::JdkProfileDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("JDK 配置"));
    setModal(true);
    PageLayout::applyModalDialog(this);
    AppBranding::applyWindowIcon(this);

    QString error;
    JdkProfileStore store(DataPaths::jdkProfilesFile());
    if (!store.load(&m_profiles, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载 JDK 配置失败"), error);
    }

    buildUi();
    populateTable();
}

QVector<JdkProfile> JdkProfileDialog::profiles() const
{
    return m_profiles;
}

void JdkProfileDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(PageLayout::Space12);
    auto *addButton = new QPushButton(QStringLiteral("新增"));
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    auto *removeButton = new QPushButton(QStringLiteral("删除"));
    removeButton->setObjectName(QStringLiteral("dangerButton"));
    connect(addButton, &QPushButton::clicked, this, &JdkProfileDialog::addProfile);
    connect(editButton, &QPushButton::clicked, this, &JdkProfileDialog::editProfile);
    connect(removeButton, &QPushButton::clicked, this, &JdkProfileDialog::removeProfile);
    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(removeButton);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_table = new QTableWidget(0, 2);
    m_table->setHorizontalHeaderLabels({QStringLiteral("版本"), QStringLiteral("目录")});
    PageLayout::configureDataTable(m_table);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    connect(m_table, &QTableWidget::doubleClicked, this, &JdkProfileDialog::editProfile);
    layout->addWidget(m_table, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QString error;
        JdkProfileStore store(DataPaths::jdkProfilesFile());
        if (!store.save(m_profiles, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void JdkProfileDialog::populateTable()
{
    m_table->setRowCount(m_profiles.size());
    for (int row = 0; row < m_profiles.size(); ++row) {
        const JdkProfile &profile = m_profiles.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(profile.version));
        m_table->setItem(row, 1, new QTableWidgetItem(profile.path));
    }
    if (!m_profiles.isEmpty()) {
        m_table->selectRow(0);
    }
}

int JdkProfileDialog::selectedRow() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

QString JdkProfileDialog::makeId(const QString &version, const QString &path) const
{
    const QByteArray seed = (version + QLatin1Char('|') + path).toUtf8();
    return QStringLiteral("jdk-") + QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha1).toHex().left(10));
}

void JdkProfileDialog::addProfile()
{
    bool ok = false;
    const QString version = QInputDialog::getText(this,
                                                  QStringLiteral("JDK 版本"),
                                                  QStringLiteral("版本名称"),
                                                  QLineEdit::Normal,
                                                  QStringLiteral("JDK 17"),
                                                  &ok).trimmed();
    if (!ok || version.isEmpty()) {
        return;
    }
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 JDK 目录"));
    if (path.isEmpty()) {
        return;
    }
    m_profiles.append({makeId(version, path), version, QDir::fromNativeSeparators(path)});
    populateTable();
}

void JdkProfileDialog::editProfile()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_profiles.size()) {
        return;
    }
    JdkProfile &profile = m_profiles[row];
    bool ok = false;
    const QString version = QInputDialog::getText(this,
                                                  QStringLiteral("JDK 版本"),
                                                  QStringLiteral("版本名称"),
                                                  QLineEdit::Normal,
                                                  profile.version,
                                                  &ok).trimmed();
    if (!ok || version.isEmpty()) {
        return;
    }
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择 JDK 目录"), profile.path);
    if (path.isEmpty()) {
        return;
    }
    profile.version = version;
    profile.path = QDir::fromNativeSeparators(path);
    profile.id = makeId(profile.version, profile.path);
    populateTable();
    m_table->selectRow(row);
}

void JdkProfileDialog::removeProfile()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_profiles.size()) {
        return;
    }
    m_profiles.removeAt(row);
    populateTable();
}
