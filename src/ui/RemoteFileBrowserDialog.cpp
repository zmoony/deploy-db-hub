#include "ui/RemoteFileBrowserDialog.h"

#include "adapters/remote/RemoteFileBrowser.h"
#include "infra/AppBranding.h"
#include "ui/PageLayout.h"
#include "ui/RemoteFileEditorDialog.h"
#include "ui/RemoteFileViewerDialog.h"
#include "ui/RemoteTerminalWidget.h"
#include "ui/RemoteUiHelpers.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QMenu>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

#include <QFutureWatcher>
#include <QtConcurrent>

namespace {

QString fileTypeLabel(RemoteFileType type)
{
    switch (type) {
    case RemoteFileType::Directory:
        return QStringLiteral("目录");
    case RemoteFileType::File:
        return QStringLiteral("文件");
    case RemoteFileType::Symlink:
        return QStringLiteral("链接");
    default:
        return QStringLiteral("未知");
    }
}

bool isNavEntry(const QString &name)
{
    return name == QStringLiteral(".") || name == QStringLiteral("..");
}

}

RemoteFileBrowserDialog::RemoteFileBrowserDialog(const RemoteConnectionContext &connectionContext,
                                                 QWidget *parent,
                                                 Purpose purpose)
    : QDialog(parent)
    , m_connectionContext(connectionContext)
    , m_serverConfig(connectionContext.serverConfig)
    , m_browser(createRemoteFileBrowser(connectionContext, makeHostKeyPromptHandler(this)))
    , m_purpose(purpose)
    , m_currentPath(QStringLiteral("/"))
    , m_bookmarkPath(connectionContext.serverConfig.value(QStringLiteral("remoteFiles")).toObject()
                         .value(QStringLiteral("defaultBrowsePath")).toString())
{
    const QString host = connectionContext.serverConfig.value(QStringLiteral("host")).toString();
    const QString user = connectionContext.serverConfig.value(QStringLiteral("username")).toString();
    if (m_purpose == Purpose::PickDirectory) {
        setWindowTitle(QStringLiteral("选择远程目录 - %1@%2").arg(user, host));
    } else {
        setWindowTitle(QStringLiteral("SFTP 文件浏览器 - %1@%2").arg(user, host));
    }
    setModal(m_purpose == Purpose::PickDirectory);
    if (m_purpose == Purpose::Browse) {
        // MobaXterm 式集成操作台：更大的工作区，容纳目录树 + 文件列表 + 终端。
        PageLayout::applyRemoteToolDialog(this, 760, 460, 1280, 820);
    } else {
        PageLayout::applyRemoteToolDialog(this);
    }
    AppBranding::applyWindowIcon(this);
    buildUi();
    if (!m_browser) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("远程文件浏览仅支持 Linux 服务器。"));
        reject();
        return;
    }

    ensureTreeBranch(QStringLiteral("/"));
    setCurrentPath(m_currentPath);
    refreshListing();
    if (m_terminal != nullptr) {
        m_terminal->start();
    }
}

void RemoteFileBrowserDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);
    if (m_purpose == Purpose::Browse) {
        layout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space12);
        layout->setSpacing(PageLayout::Space12);
    }

    auto *header = new QFrame;
    header->setObjectName(QStringLiteral("remoteFileHeader"));
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(PageLayout::Space24,
                                     m_purpose == Purpose::Browse ? PageLayout::Space16 : PageLayout::Space24,
                                     PageLayout::Space24,
                                     PageLayout::Space16);
    headerLayout->setSpacing(m_purpose == Purpose::Browse ? PageLayout::Space8 : PageLayout::Space12);

    m_connectionLabel = new QLabel;
    m_connectionLabel->setObjectName(QStringLiteral("remoteConnectionLabel"));
    const QString user = m_serverConfig.value(QStringLiteral("username")).toString();
    const QString host = m_serverConfig.value(QStringLiteral("host")).toString();
    m_connectionLabel->setText(QStringLiteral("Linux 远程操作 · %1@%2").arg(user, host));
    headerLayout->addWidget(m_connectionLabel);

    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(PageLayout::Space12);
    auto *pathLabel = new QLabel(QStringLiteral("远程路径"));
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText(QStringLiteral("/opt/deploy-hub"));
    PageLayout::configurePathField(m_pathEdit);
    auto *goButton = new QPushButton(QStringLiteral("前往"));
    goButton->setObjectName(QStringLiteral("primaryButton"));
    connect(goButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::refreshListing);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &RemoteFileBrowserDialog::refreshListing);
    pathRow->addWidget(pathLabel);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(goButton);
    headerLayout->addLayout(pathRow);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(PageLayout::Space12);
    auto *homeButton = new QPushButton(QStringLiteral("根目录 /"));
    auto *bookmarkButton = new QPushButton(QStringLiteral("部署目录"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    auto *upButton = new QPushButton(QStringLiteral("上级"));
    m_viewButton = new QPushButton(QStringLiteral("查看"));
    m_editButton = new QPushButton(QStringLiteral("编辑"));
    m_deleteButton = new QPushButton(QStringLiteral("删除"));
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    m_mkdirButton = new QPushButton(QStringLiteral("新建目录"));
    m_uploadButton = new QPushButton(QStringLiteral("上传"));
    m_uploadButton->setObjectName(QStringLiteral("primaryButton"));
    connect(homeButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::goHome);
    connect(bookmarkButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::goBookmark);
    connect(refreshButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::refreshListing);
    connect(upButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::goUp);
    connect(m_viewButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::viewSelectedFile);
    connect(m_editButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::editSelectedFile);
    connect(m_deleteButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::deleteSelectedEntry);
    connect(m_mkdirButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::createDirectory);
    connect(m_uploadButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::uploadFile);
    toolbar->addWidget(homeButton);
    if (!m_bookmarkPath.isEmpty() && m_bookmarkPath != QStringLiteral("/")) {
        bookmarkButton->setToolTip(m_bookmarkPath);
        toolbar->addWidget(bookmarkButton);
    } else {
        delete bookmarkButton;
    }
    toolbar->addWidget(refreshButton);
    toolbar->addWidget(upButton);
    toolbar->addWidget(m_viewButton);
    toolbar->addWidget(m_editButton);
    toolbar->addWidget(m_deleteButton);
    toolbar->addWidget(m_mkdirButton);
    toolbar->addWidget(m_uploadButton);
    toolbar->addStretch();
    headerLayout->addLayout(toolbar);
    layout->addWidget(header);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setMinimumHeight(m_purpose == Purpose::Browse ? 320 : 360);
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_tree = new QTreeWidget;
    m_tree->setObjectName(QStringLiteral("remoteDirTree"));
    m_tree->setHeaderHidden(true);
    m_tree->setMinimumWidth(160);
    m_tree->setMaximumWidth(220);
    m_tree->setAnimated(true);
    m_tree->setIndentation(20);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::itemExpanded, this, &RemoteFileBrowserDialog::onTreeItemExpanded);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &RemoteFileBrowserDialog::onTreeSelectionChanged);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &RemoteFileBrowserDialog::showTreeContextMenu);

    auto *listPanel = new QWidget;
    auto *listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(PageLayout::Space8);

    auto *listTitle = new QLabel(QStringLiteral("文件列表"));
    listTitle->setObjectName(QStringLiteral("sectionLabel"));
    listLayout->addWidget(listTitle);

    m_table = new QTableWidget(0, 5);
    m_table->setObjectName(QStringLiteral("remoteFileTable"));
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("大小"),
        QStringLiteral("权限"),
        QStringLiteral("路径")
    });
    m_table->verticalHeader()->setVisible(false);
    PageLayout::configureDataTable(m_table);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setAcceptDrops(true);
    m_table->setDragDropMode(QAbstractItemView::DropOnly);
    m_table->viewport()->setAcceptDrops(true);
    m_table->viewport()->installEventFilter(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::doubleClicked, this, &RemoteFileBrowserDialog::onTableDoubleClicked);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &RemoteFileBrowserDialog::showTableContextMenu);
    listLayout->addWidget(m_table, 1);

    if (m_purpose == Purpose::Browse) {
        auto *listStatusRow = new QHBoxLayout;
        listStatusRow->setSpacing(PageLayout::Space12);
        m_statusLabel = new QLabel;
        m_statusLabel->setObjectName(QStringLiteral("remoteStatusLabel"));
        m_uploadProgress = new QProgressBar;
        m_uploadProgress->setObjectName(QStringLiteral("remoteUploadProgress"));
        m_uploadProgress->setVisible(false);
        m_uploadProgress->setTextVisible(true);
        m_uploadProgress->setFixedWidth(180);
        m_uploadProgress->setMaximumHeight(18);
        listStatusRow->addWidget(m_statusLabel, 1);
        listStatusRow->addWidget(m_uploadProgress);
        listLayout->addLayout(listStatusRow);
    }

    m_splitter->addWidget(m_tree);
    if (m_purpose == Purpose::Browse) {
        m_tabs = new QTabWidget;
        m_tabs->setObjectName(QStringLiteral("remoteOperationTabs"));
        m_tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_terminal = new RemoteTerminalWidget(m_connectionContext);
        m_terminal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_tabs->addTab(m_terminal, QStringLiteral("终端"));
        m_tabs->addTab(listPanel, QStringLiteral("文件列表"));
        m_tabs->setCurrentIndex(0);
        buildTerminalToolbar();
        connect(m_tabs, &QTabWidget::currentChanged, this, &RemoteFileBrowserDialog::syncTerminalToolbar);
        m_splitter->addWidget(m_tabs);
    } else {
        m_splitter->addWidget(listPanel);
    }
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 4);
    m_splitter->setSizes({240, 1040});
    layout->addWidget(m_splitter, 1);

    if (m_purpose == Purpose::PickDirectory) {
        auto *statusRow = new QHBoxLayout;
        statusRow->setSpacing(PageLayout::Space12);
        m_statusLabel = new QLabel;
        m_statusLabel->setObjectName(QStringLiteral("remoteStatusLabel"));
        m_uploadProgress = new QProgressBar;
        m_uploadProgress->setObjectName(QStringLiteral("remoteUploadProgress"));
        m_uploadProgress->setVisible(false);
        m_uploadProgress->setTextVisible(true);
        m_uploadProgress->setFixedWidth(180);
        m_uploadProgress->setMaximumHeight(18);
        statusRow->addWidget(m_statusLabel, 1);
        statusRow->addWidget(m_uploadProgress);
        layout->addLayout(statusRow);
    }

    if (m_purpose == Purpose::PickDirectory) {
        auto *footer = new QHBoxLayout;
        footer->setContentsMargins(0, PageLayout::Space8, 0, 0);
        footer->setSpacing(PageLayout::Space12);
        footer->addStretch();
        auto *pickButton = new QPushButton(QStringLiteral("选择此目录"));
        pickButton->setObjectName(QStringLiteral("primaryButton"));
        connect(pickButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::pickCurrentDirectory);
        auto *cancelButton = new QPushButton(QStringLiteral("取消"));
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        footer->addWidget(pickButton);
        footer->addWidget(cancelButton);
        m_viewButton->hide();
        m_editButton->hide();
        m_deleteButton->hide();
        m_uploadButton->hide();
        m_mkdirButton->hide();
        layout->addLayout(footer);
    }

    m_mkdirButton->setEnabled(operationEnabled(QStringLiteral("mkdir")));
    m_uploadButton->setEnabled(operationEnabled(QStringLiteral("upload")));
    m_viewButton->setEnabled(operationEnabled(QStringLiteral("read")));
    m_editButton->setEnabled(operationEnabled(QStringLiteral("write")));
    m_deleteButton->setEnabled(operationEnabled(QStringLiteral("delete")));
}

bool RemoteFileBrowserDialog::operationEnabled(const QString &name) const
{
    const QJsonObject operations = m_serverConfig.value(QStringLiteral("remoteFiles")).toObject()
                                       .value(QStringLiteral("operations")).toObject();
    return operations.value(name).toBool(true);
}

void RemoteFileBrowserDialog::buildTerminalToolbar()
{
    if (m_terminal == nullptr || m_tabs == nullptr) {
        return;
    }

    m_terminalToolbar = new QWidget(m_tabs);
    m_terminalToolbar->setObjectName(QStringLiteral("terminalTabToolbar"));
    auto *toolbarLayout = new QHBoxLayout(m_terminalToolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    toolbarLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    for (QWidget *widget : m_terminal->takeToolbarWidgets()) {
        toolbarLayout->addWidget(widget, 0, Qt::AlignVCenter);
    }

    auto *syncPathButton = new QPushButton(QStringLiteral("同步路径"));
    syncPathButton->setObjectName(QStringLiteral("terminalToolbarButton"));
    syncPathButton->setToolTip(QStringLiteral("获取终端当前工作目录并填充到路径栏"));
    connect(syncPathButton, &QPushButton::clicked, this, &RemoteFileBrowserDialog::syncPathFromTerminal);
    toolbarLayout->addWidget(syncPathButton, 0, Qt::AlignVCenter);

    connect(m_terminal, &RemoteTerminalWidget::currentDirectoryReceived,
            this, [this](const QString &path) {
                setCurrentPath(path);
                refreshListing();
            });

    m_tabs->setCornerWidget(m_terminalToolbar, Qt::TopRightCorner);
    syncTerminalToolbar();
}

void RemoteFileBrowserDialog::syncTerminalToolbar()
{
    if (m_terminalToolbar != nullptr && m_tabs != nullptr) {
        m_terminalToolbar->setVisible(m_tabs->currentWidget() == m_terminal);
    }
}

void RemoteFileBrowserDialog::syncPathFromTerminal()
{
    if (m_terminal == nullptr) {
        return;
    }
    m_terminal->requestCurrentWorkingDirectory();
}

QString RemoteFileBrowserDialog::formatSize(qint64 sizeBytes) const
{
    if (sizeBytes < 0) {
        return QStringLiteral("-");
    }
    if (sizeBytes < 1024) {
        return QStringLiteral("%1 B").arg(sizeBytes);
    }
    if (sizeBytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(sizeBytes / 1024);
    }
    return QStringLiteral("%1 MB").arg(sizeBytes / (1024 * 1024));
}

void RemoteFileBrowserDialog::setInitialPath(const QString &path)
{
    m_currentPath = path.trimmed().isEmpty() ? QStringLiteral("/") : path.trimmed();
    if (m_pathEdit == nullptr) {
        return;
    }
    ensureTreeBranch(m_currentPath);
    setCurrentPath(m_currentPath);
    refreshListing();
}

QString RemoteFileBrowserDialog::selectedPath() const
{
    return m_selectedPath;
}

void RemoteFileBrowserDialog::pickCurrentDirectory()
{
    m_selectedPath = m_currentPath;
    accept();
}

void RemoteFileBrowserDialog::setCurrentPath(const QString &path)
{
    m_currentPath = path;
    m_pathEdit->setText(path);
    syncTreeSelection();
}

void RemoteFileBrowserDialog::ensureTreeBranch(const QString &path)
{
    const QString normalized = path.isEmpty() ? QStringLiteral("/") : path;
    if (m_tree->topLevelItemCount() == 0) {
        auto *rootItem = new QTreeWidgetItem(QStringList{QStringLiteral("/")});
        rootItem->setData(0, Qt::UserRole, QStringLiteral("/"));
        rootItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        auto *placeholder = new QTreeWidgetItem(rootItem, {QStringLiteral("...")});
        placeholder->setData(0, Qt::UserRole, QString());
        m_tree->addTopLevelItem(rootItem);
    }

    QTreeWidgetItem *rootItem = m_tree->topLevelItem(0);
    if (normalized == QStringLiteral("/")) {
        return;
    }

    QStringList segments = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QTreeWidgetItem *current = rootItem;
    QString built = QStringLiteral("/");

    for (const QString &segment : segments) {
        built = built == QStringLiteral("/") ? built + segment : built + QLatin1Char('/') + segment;
        QTreeWidgetItem *next = nullptr;
        for (int i = 0; i < current->childCount(); ++i) {
            QTreeWidgetItem *child = current->child(i);
            if (child->data(0, Qt::UserRole).toString() == built) {
                next = child;
                break;
            }
        }
        if (next == nullptr) {
            next = new QTreeWidgetItem(current, {segment});
            next->setData(0, Qt::UserRole, built);
            next->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            auto *placeholder = new QTreeWidgetItem(next, {QStringLiteral("...")});
            placeholder->setData(0, Qt::UserRole, QString());
        }
        current = next;
    }
}

void RemoteFileBrowserDialog::reloadTreeNode(const QString &path)
{
    QTreeWidgetItem *item = findTreeItem(path);
    if (item == nullptr) {
        return;
    }

    while (item->childCount() > 0) {
        delete item->takeChild(0);
    }
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    auto *placeholder = new QTreeWidgetItem(item, {QStringLiteral("...")});
    placeholder->setData(0, Qt::UserRole, QString());

    if (item->isExpanded()) {
        onTreeItemExpanded(item);
    }
}

QTreeWidgetItem *RemoteFileBrowserDialog::findTreeItem(const QString &path) const
{
    if (m_tree->topLevelItemCount() == 0) {
        return nullptr;
    }

    const QString normalized = path.isEmpty() ? QStringLiteral("/") : path;
    QTreeWidgetItem *rootItem = m_tree->topLevelItem(0);
    if (normalized == QStringLiteral("/")) {
        return rootItem;
    }

    QStringList segments = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QTreeWidgetItem *current = rootItem;
    QString built = QStringLiteral("/");

    for (const QString &segment : segments) {
        built = built == QStringLiteral("/") ? built + segment : built + QLatin1Char('/') + segment;
        QTreeWidgetItem *next = nullptr;
        for (int i = 0; i < current->childCount(); ++i) {
            QTreeWidgetItem *child = current->child(i);
            if (child->data(0, Qt::UserRole).toString() == built) {
                next = child;
                break;
            }
        }
        if (next == nullptr) {
            return nullptr;
        }
        current = next;
    }
    return current;
}

void RemoteFileBrowserDialog::onTreeItemExpanded(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    const QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) {
        return;
    }

    if (item->childCount() == 1 && item->child(0)->data(0, Qt::UserRole).toString().isEmpty()) {
        delete item->takeChild(0);
    } else if (item->childCount() > 0) {
        return;
    }

    const RemoteFileListResult result = m_browser->listDirectory(path);
    if (!result.ok) {
        return;
    }

    for (const RemoteFileEntry &entry : result.entries) {
        if (entry.type != RemoteFileType::Directory || isNavEntry(entry.name)) {
            continue;
        }
        auto *child = new QTreeWidgetItem(item, {entry.name});
        child->setData(0, Qt::UserRole, entry.path);
        child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        auto *placeholder = new QTreeWidgetItem(child, {QStringLiteral("...")});
        placeholder->setData(0, Qt::UserRole, QString());
    }
}

void RemoteFileBrowserDialog::onTreeSelectionChanged()
{
    const QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString path = selected.first()->data(0, Qt::UserRole).toString();
    if (path.isEmpty() || path == m_currentPath) {
        return;
    }
    setCurrentPath(path);
    refreshListing();
}

void RemoteFileBrowserDialog::syncTreeSelection()
{
    if (m_tree->topLevelItemCount() == 0) {
        return;
    }
    ensureTreeBranch(m_currentPath);

    QTreeWidgetItem *rootItem = m_tree->topLevelItem(0);
    QTreeWidgetItem *target = rootItem;
    if (m_currentPath != QStringLiteral("/")) {
        QStringList segments = m_currentPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QString built = QStringLiteral("/");
        for (const QString &segment : segments) {
            built = built == QStringLiteral("/") ? built + segment : built + QLatin1Char('/') + segment;
            QTreeWidgetItem *next = nullptr;
            for (int i = 0; i < target->childCount(); ++i) {
                QTreeWidgetItem *child = target->child(i);
                if (child->data(0, Qt::UserRole).toString() == built) {
                    next = child;
                    break;
                }
            }
            if (next == nullptr) {
                break;
            }
            target = next;
        }
    }

    m_tree->setCurrentItem(target);
    QTreeWidgetItem *ancestor = target;
    while (ancestor != nullptr) {
        m_tree->expandItem(ancestor);
        ancestor = ancestor->parent();
    }
    m_tree->scrollToItem(target);
}

void RemoteFileBrowserDialog::hydrateTreeNode(QTreeWidgetItem *item, const QVector<RemoteFileEntry> &entries)
{
    if (item == nullptr) {
        return;
    }

    if (item->childCount() == 1 && item->child(0)->data(0, Qt::UserRole).toString().isEmpty()) {
        delete item->takeChild(0);
    } else if (item->childCount() > 0) {
        return;
    }

    for (const RemoteFileEntry &entry : entries) {
        if (entry.type != RemoteFileType::Directory || isNavEntry(entry.name)) {
            continue;
        }
        auto *child = new QTreeWidgetItem(item, {entry.name});
        child->setData(0, Qt::UserRole, entry.path);
        child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        auto *placeholder = new QTreeWidgetItem(child, {QStringLiteral("...")});
        placeholder->setData(0, Qt::UserRole, QString());
    }
}

void RemoteFileBrowserDialog::applyListingResult(const RemoteFileListResult &result)
{
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("无法列出目录"), result.error);
        m_statusLabel->setText(result.error);
        return;
    }

    setCurrentPath(result.currentPath);
    ensureTreeBranch(result.currentPath);
    hydrateTreeNode(findTreeItem(result.currentPath), result.entries);
    syncTreeSelection();
    populateTable(result.entries);
    m_statusLabel->setText(QStringLiteral("当前目录 %1，共 %2 项").arg(result.currentPath).arg(result.entries.size()));
}

void RemoteFileBrowserDialog::refreshListing()
{
    const QString requestedPath = m_pathEdit->text().trimmed().isEmpty() ? m_currentPath : m_pathEdit->text().trimmed();
    m_statusLabel->setText(QStringLiteral("加载中..."));

    const int generation = ++m_listingGeneration;
    RemoteFileBrowser *browser = m_browser.get();
    auto future = QtConcurrent::run([browser, requestedPath]() {
        return browser->listDirectory(requestedPath);
    });

    auto *watcher = new QFutureWatcher<RemoteFileListResult>(this);
    connect(watcher, &QFutureWatcher<RemoteFileListResult>::finished, this, [this, watcher, generation]() {
        if (generation != m_listingGeneration) {
            watcher->deleteLater();
            return;
        }
        applyListingResult(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void RemoteFileBrowserDialog::populateTable(const QVector<RemoteFileEntry> &entries)
{
    m_table->setRowCount(entries.size());
    for (int row = 0; row < entries.size(); ++row) {
        const RemoteFileEntry &entry = entries.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(entry.name));
        m_table->setItem(row, 1, new QTableWidgetItem(fileTypeLabel(entry.type)));
        m_table->setItem(row, 2, new QTableWidgetItem(formatSize(entry.sizeBytes)));
        m_table->setItem(row, 3, new QTableWidgetItem(entry.writable ? QStringLiteral("rw") : QStringLiteral("r")));
        auto *pathItem = new QTableWidgetItem(entry.path);
        pathItem->setData(Qt::UserRole, static_cast<int>(entry.type));
        m_table->setItem(row, 4, pathItem);
    }
    if (!entries.isEmpty()) {
        m_table->selectRow(0);
    }
}

QString RemoteFileBrowserDialog::selectedEntryPath() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    return m_table->item(rows.first().row(), 4)->text();
}

RemoteFileType RemoteFileBrowserDialog::selectedEntryType() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return RemoteFileType::Unknown;
    }
    const int typeValue = m_table->item(rows.first().row(), 4)->data(Qt::UserRole).toInt();
    return static_cast<RemoteFileType>(typeValue);
}

bool RemoteFileBrowserDialog::selectedIsDirectory() const
{
    return selectedEntryType() == RemoteFileType::Directory;
}

void RemoteFileBrowserDialog::onTableDoubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    if (selectedIsDirectory()) {
        enterSelected();
        return;
    }
    if (m_purpose == Purpose::PickDirectory) {
        return;
    }
    if (selectedEntryType() == RemoteFileType::File) {
        viewSelectedFile();
    }
}

void RemoteFileBrowserDialog::enterSelected()
{
    if (!selectedIsDirectory()) {
        return;
    }

    const QString path = selectedEntryPath();
    if (path.endsWith(QStringLiteral("/."))) {
        refreshListing();
        return;
    }
    if (path.endsWith(QStringLiteral("/.."))) {
        goUp();
        return;
    }

    setCurrentPath(path);
    refreshListing();
}

void RemoteFileBrowserDialog::openTerminalAtSelectedTreePath()
{
    const QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString path = selected.first()->data(0, Qt::UserRole).toString();
    openTerminalAtPath(path, false);
}

void RemoteFileBrowserDialog::openTerminalAtSelectedEntryPath()
{
    const QString path = selectedEntryPath();
    if (path.isEmpty()) {
        return;
    }
    openTerminalAtPath(path, selectedEntryType() != RemoteFileType::Directory);
}

void RemoteFileBrowserDialog::openTerminalAtPath(const QString &path, bool pathIsFile)
{
    if (m_terminal == nullptr) {
        return;
    }

    QString directory = path.trimmed().isEmpty() ? m_currentPath : path;
    if (pathIsFile) {
        const int slashIndex = directory.lastIndexOf(QLatin1Char('/'));
        directory = slashIndex <= 0 ? QStringLiteral("/") : directory.left(slashIndex);
    }
    if (directory.endsWith(QStringLiteral("/."))) {
        directory.chop(2);
    } else if (directory.endsWith(QStringLiteral("/.."))) {
        const QString parent = directory.left(directory.size() - 3);
        const int slashIndex = parent.lastIndexOf(QLatin1Char('/'));
        directory = slashIndex <= 0 ? QStringLiteral("/") : parent.left(slashIndex);
    }
    if (directory.isEmpty()) {
        directory = QStringLiteral("/");
    }

    if (m_tabs != nullptr) {
        m_tabs->setCurrentIndex(0);
    }
    QString escaped = directory;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    m_terminal->sendCommand(QStringLiteral("cd '%1'").arg(escaped));
}

void RemoteFileBrowserDialog::goHome()
{
    setCurrentPath(QStringLiteral("/"));
    refreshListing();
}

void RemoteFileBrowserDialog::goBookmark()
{
    if (m_bookmarkPath.isEmpty()) {
        return;
    }
    setCurrentPath(m_bookmarkPath);
    refreshListing();
}

void RemoteFileBrowserDialog::openFileEditor(const QString &remotePath, bool readOnly)
{
    if (readOnly) {
        RemoteFileViewerDialog viewer(m_browser.get(), remotePath, nullptr, nullptr, this);
        viewer.exec();
        return;
    }

    const RemoteFileReadResult readResult = m_browser->readFile(remotePath);
    if (!readResult.ok) {
        QMessageBox::warning(this, QStringLiteral("无法读取文件"), readResult.error);
        return;
    }

    RemoteFileEditorDialog::SaveHandler saveHandler;
    if (!readOnly) {
        saveHandler = [this, remotePath](const QString &content, QString *error) {
            const RemoteFileWriteResult writeResult = m_browser->writeFile(remotePath, content);
            if (!writeResult.ok) {
                if (error != nullptr) {
                    *error = writeResult.error;
                }
                return false;
            }
            m_statusLabel->setText(QStringLiteral("已保存：%1").arg(remotePath));
            refreshListing();
            return true;
        };
    }

    RemoteFileEditorDialog editor(remotePath, readResult.content, readOnly, saveHandler, this);
    editor.exec();
}

void RemoteFileBrowserDialog::viewSelectedFile()
{
    if (selectedIsDirectory()) {
        QMessageBox::information(this, QStringLiteral("无法查看"), QStringLiteral("请选择文件而非目录。"));
        return;
    }
    const QString path = selectedEntryPath();
    if (path.isEmpty()) {
        return;
    }
    openFileEditor(path, true);
}

void RemoteFileBrowserDialog::editSelectedFile()
{
    if (selectedIsDirectory()) {
        QMessageBox::information(this, QStringLiteral("无法编辑"), QStringLiteral("请选择文件而非目录。"));
        return;
    }
    const QString path = selectedEntryPath();
    if (path.isEmpty()) {
        return;
    }
    openFileEditor(path, false);
}

void RemoteFileBrowserDialog::deleteSelectedEntry()
{
    const QString path = selectedEntryPath();
    if (path.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要删除的文件或目录。"));
        return;
    }
    if (path == QStringLiteral("/")) {
        QMessageBox::warning(this, QStringLiteral("无法删除"), QStringLiteral("不能删除根目录。"));
        return;
    }

    const QString typeLabel = selectedIsDirectory() ? QStringLiteral("目录") : QStringLiteral("文件");
    const auto answer = QMessageBox::question(this,
                                              QStringLiteral("确认删除"),
                                              QStringLiteral("确定删除%1：\n%2").arg(typeLabel, path));
    if (answer != QMessageBox::Yes) {
        return;
    }

    const RemoteFileOperationResult result = m_browser->deleteEntry(path);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), result.error);
        return;
    }

    const int slashIndex = path.lastIndexOf(QLatin1Char('/'));
    const QString parentPath = slashIndex <= 0 ? QStringLiteral("/") : path.left(slashIndex);
    reloadTreeNode(parentPath);
    if (m_currentPath == path || m_currentPath.startsWith(path + QLatin1Char('/'))) {
        setCurrentPath(parentPath);
    }
    m_statusLabel->setText(QStringLiteral("已删除：%1").arg(path));
    refreshListing();
}

void RemoteFileBrowserDialog::goUp()
{
    if (m_currentPath == QStringLiteral("/")) {
        return;
    }

    const int slashIndex = m_currentPath.lastIndexOf(QLatin1Char('/'));
    if (slashIndex <= 0) {
        setCurrentPath(QStringLiteral("/"));
    } else {
        setCurrentPath(m_currentPath.left(slashIndex));
    }
    refreshListing();
}

void RemoteFileBrowserDialog::createDirectory()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this,
                                               QStringLiteral("新建目录"),
                                               QStringLiteral("目录名称"),
                                               QLineEdit::Normal,
                                               QString(),
                                               &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    QString remotePath = m_currentPath;
    if (!remotePath.endsWith(QLatin1Char('/'))) {
        remotePath += QLatin1Char('/');
    }
    remotePath += name.trimmed();

    const RemoteFileOperationResult result = m_browser->createDirectory(remotePath);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("创建失败"), result.error);
        return;
    }

    m_statusLabel->setText(QStringLiteral("已创建目录：%1").arg(remotePath));
    reloadTreeNode(m_currentPath);
    refreshListing();
}

void RemoteFileBrowserDialog::uploadFile()
{
    const QString localPath = QFileDialog::getOpenFileName(this, QStringLiteral("选择要上传的本地文件"));
    if (localPath.isEmpty()) {
        return;
    }
    uploadLocalFiles({localPath});
}

bool RemoteFileBrowserDialog::selectedIsNavEntry() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return false;
    }
    const QString name = m_table->item(rows.first().row(), 0)->text();
    return isNavEntry(name);
}

bool RemoteFileBrowserDialog::hasPasteSource() const
{
    if (!m_clipboardRemotePath.isEmpty()) {
        return true;
    }
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr || clipboard->mimeData() == nullptr) {
        return false;
    }
    for (const QUrl &url : clipboard->mimeData()->urls()) {
        if (url.isLocalFile()) {
            return true;
        }
    }
    return false;
}

QString RemoteFileBrowserDialog::remotePathInDirectory(const QString &name) const
{
    QString remotePath = m_currentPath;
    if (!remotePath.endsWith(QLatin1Char('/'))) {
        remotePath += QLatin1Char('/');
    }
    remotePath += name;
    return remotePath;
}

void RemoteFileBrowserDialog::uploadLocalFiles(const QStringList &localPaths)
{
    if (!operationEnabled(QStringLiteral("upload"))) {
        QMessageBox::information(this, QStringLiteral("无法上传"), QStringLiteral("上传操作已在服务器配置中禁用。"));
        return;
    }

    QStringList files;
    for (const QString &localPath : localPaths) {
        const QFileInfo info(localPath);
        if (info.isFile()) {
            files << info.absoluteFilePath();
        }
    }
    if (files.isEmpty()) {
        return;
    }

    if (m_uploadProgress != nullptr) {
        m_uploadProgress->setVisible(true);
        m_uploadProgress->setRange(0, files.size());
        m_uploadProgress->setValue(0);
    }

    int completed = 0;
    for (const QString &localPath : files) {
        const QString fileName = QFileInfo(localPath).fileName();
        if (m_statusLabel != nullptr) {
            m_statusLabel->setText(QStringLiteral("上传中：%1 (%2/%3)")
                                       .arg(fileName)
                                       .arg(completed + 1)
                                       .arg(files.size()));
        }
        if (m_uploadProgress != nullptr) {
            m_uploadProgress->setFormat(QStringLiteral("%1/%2").arg(completed + 1).arg(files.size()));
        }
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        const RemoteFileUploadResult result = m_browser->uploadFile(localPath, remotePathInDirectory(fileName));
        ++completed;
        if (m_uploadProgress != nullptr) {
            m_uploadProgress->setValue(completed);
        }
        if (!result.ok) {
            QMessageBox::warning(this,
                                 QStringLiteral("上传失败"),
                                 QStringLiteral("%1\n%2").arg(localPath, result.error));
            break;
        }
    }

    if (m_uploadProgress != nullptr) {
        m_uploadProgress->setVisible(false);
    }
    if (completed == files.size()) {
        m_statusLabel->setText(QStringLiteral("已上传 %1 个文件到 %2").arg(files.size()).arg(m_currentPath));
    }
    reloadTreeNode(m_currentPath);
    refreshListing();
}

void RemoteFileBrowserDialog::showTableContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    const QModelIndex index = m_table->indexAt(pos);
    if (index.isValid()) {
        m_table->selectRow(index.row());
    }

    if (index.isValid() && !selectedIsNavEntry()) {
        const bool isDirectory = selectedIsDirectory();
        const bool isFile = selectedEntryType() == RemoteFileType::File;

        if (isFile && operationEnabled(QStringLiteral("read"))) {
            menu.addAction(QStringLiteral("查看"), this, &RemoteFileBrowserDialog::viewSelectedFile);
        }
        if (isFile && operationEnabled(QStringLiteral("write"))) {
            menu.addAction(QStringLiteral("编辑"), this, &RemoteFileBrowserDialog::editSelectedFile);
        }
        if (m_terminal != nullptr) {
            menu.addAction(QStringLiteral("在终端打开"), this, &RemoteFileBrowserDialog::openTerminalAtSelectedEntryPath);
        }
        if (!menu.isEmpty()) {
            menu.addSeparator();
        }

        if (operationEnabled(QStringLiteral("upload"))) {
            menu.addAction(QStringLiteral("上传"), this, &RemoteFileBrowserDialog::uploadFile);
        }
        if (operationEnabled(QStringLiteral("rename"))) {
            menu.addAction(QStringLiteral("重命名"), this, &RemoteFileBrowserDialog::renameSelectedEntry);
        }
        menu.addAction(QStringLiteral("复制"), this, &RemoteFileBrowserDialog::copySelectedEntry);
        menu.addAction(QStringLiteral("剪切"), this, &RemoteFileBrowserDialog::cutSelectedEntry);
        if (operationEnabled(QStringLiteral("delete"))) {
            menu.addAction(QStringLiteral("删除"), this, &RemoteFileBrowserDialog::deleteSelectedEntry);
        }
        menu.addSeparator();
        menu.addAction(QStringLiteral("属性"), this, &RemoteFileBrowserDialog::showSelectedProperties);
    } else {
        if (hasPasteSource()) {
            menu.addAction(QStringLiteral("粘贴"), this, &RemoteFileBrowserDialog::pasteToCurrentDirectory);
        }
        if (operationEnabled(QStringLiteral("upload"))) {
            menu.addAction(QStringLiteral("上传"), this, &RemoteFileBrowserDialog::uploadFile);
        }
        if (operationEnabled(QStringLiteral("mkdir"))) {
            menu.addAction(QStringLiteral("新建目录"), this, &RemoteFileBrowserDialog::createDirectory);
        }
        if (m_terminal != nullptr) {
            menu.addAction(QStringLiteral("在终端打开"), this, &RemoteFileBrowserDialog::openTerminalAtSelectedEntryPath);
        }
    }

    if (!menu.isEmpty()) {
        menu.exec(m_table->viewport()->mapToGlobal(pos));
    }
}

void RemoteFileBrowserDialog::showTreeContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (item == nullptr) {
        return;
    }
    m_tree->setCurrentItem(item);

    QMenu menu(this);
    if (m_terminal != nullptr) {
        menu.addAction(QStringLiteral("在终端打开"), this, &RemoteFileBrowserDialog::openTerminalAtSelectedTreePath);
    }
    if (hasPasteSource()) {
        menu.addAction(QStringLiteral("粘贴"), this, &RemoteFileBrowserDialog::pasteToCurrentDirectory);
    }
    if (operationEnabled(QStringLiteral("upload"))) {
        menu.addAction(QStringLiteral("上传"), this, &RemoteFileBrowserDialog::uploadFile);
    }
    if (operationEnabled(QStringLiteral("mkdir"))) {
        menu.addAction(QStringLiteral("新建目录"), this, &RemoteFileBrowserDialog::createDirectory);
    }

    const QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        menu.addSeparator();
        menu.addAction(QStringLiteral("属性"), this, [this, path]() {
            QMessageBox box(this);
            box.setWindowTitle(QStringLiteral("属性"));
            box.setIcon(QMessageBox::Information);
            box.setText(QFileInfo(path).fileName());
            box.setInformativeText(QStringLiteral("类型：目录\n路径：%1").arg(path));
            box.exec();
        });
    }

    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void RemoteFileBrowserDialog::copySelectedEntry()
{
    const QString path = selectedEntryPath();
    if (path.isEmpty() || selectedIsNavEntry()) {
        return;
    }
    m_clipboardRemotePath = path;
    m_clipboardCut = false;
    m_statusLabel->setText(QStringLiteral("已复制：%1").arg(path));
}

void RemoteFileBrowserDialog::cutSelectedEntry()
{
    const QString path = selectedEntryPath();
    if (path.isEmpty() || selectedIsNavEntry()) {
        return;
    }
    m_clipboardRemotePath = path;
    m_clipboardCut = true;
    m_statusLabel->setText(QStringLiteral("已剪切：%1").arg(path));
}

void RemoteFileBrowserDialog::pasteToCurrentDirectory()
{
    if (!m_clipboardRemotePath.isEmpty()) {
        const QString fileName = QFileInfo(m_clipboardRemotePath).fileName();
        const QString destPath = remotePathInDirectory(fileName);
        const RemoteFileOperationResult result = m_clipboardCut
            ? (operationEnabled(QStringLiteral("rename"))
                   ? m_browser->renameEntry(m_clipboardRemotePath, destPath)
                   : RemoteFileOperationResult{false, QStringLiteral("移动操作已在服务器配置中禁用")})
            : m_browser->copyEntry(m_clipboardRemotePath, destPath);
        if (!result.ok) {
            QMessageBox::warning(this, QStringLiteral("粘贴失败"), result.error);
            return;
        }

        const QString sourceParent = m_clipboardRemotePath.left(m_clipboardRemotePath.lastIndexOf(QLatin1Char('/')));
        reloadTreeNode(sourceParent.isEmpty() ? QStringLiteral("/") : sourceParent);
        reloadTreeNode(m_currentPath);
        m_statusLabel->setText(QStringLiteral("已粘贴到：%1").arg(destPath));
        if (m_clipboardCut) {
            m_clipboardRemotePath.clear();
            m_clipboardCut = false;
        }
        refreshListing();
        return;
    }

    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr || clipboard->mimeData() == nullptr) {
        return;
    }

    QStringList localPaths;
    for (const QUrl &url : clipboard->mimeData()->urls()) {
        if (url.isLocalFile()) {
            localPaths << url.toLocalFile();
        }
    }
    if (!localPaths.isEmpty()) {
        uploadLocalFiles(localPaths);
    }
}

void RemoteFileBrowserDialog::renameSelectedEntry()
{
    const QString oldPath = selectedEntryPath();
    if (oldPath.isEmpty() || selectedIsNavEntry()) {
        return;
    }

    bool ok = false;
    const QString newName = QInputDialog::getText(this,
                                                  QStringLiteral("重命名"),
                                                  QStringLiteral("新名称"),
                                                  QLineEdit::Normal,
                                                  QFileInfo(oldPath).fileName(),
                                                  &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }

    const QString newPath = remotePathInDirectory(newName.trimmed());
    const RemoteFileOperationResult result = m_browser->renameEntry(oldPath, newPath);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("重命名失败"), result.error);
        return;
    }

    m_statusLabel->setText(QStringLiteral("已重命名：%1 -> %2").arg(oldPath, newPath));
    reloadTreeNode(m_currentPath);
    refreshListing();
}

void RemoteFileBrowserDialog::showSelectedProperties()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return;
    }

    const int row = rows.first().row();
    const QString name = m_table->item(row, 0)->text();
    const QString type = m_table->item(row, 1)->text();
    const QString size = m_table->item(row, 2)->text();
    const QString perm = m_table->item(row, 3)->text();
    const QString path = m_table->item(row, 4)->text();

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("属性"));
    box.setIcon(QMessageBox::Information);
    box.setText(name);
    box.setInformativeText(QStringLiteral("类型：%1\n大小：%2\n权限：%3\n路径：%4").arg(type, size, perm, path));
    box.exec();
}

bool RemoteFileBrowserDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (m_table != nullptr && watched == m_table->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (dragEvent->mimeData() != nullptr && dragEvent->mimeData()->hasUrls()) {
                for (const QUrl &url : dragEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && operationEnabled(QStringLiteral("upload"))) {
                        dragEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
        }
        if (event->type() == QEvent::DragMove) {
            auto *dragEvent = static_cast<QDragMoveEvent *>(event);
            if (dragEvent->mimeData() != nullptr && dragEvent->mimeData()->hasUrls()) {
                for (const QUrl &url : dragEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && operationEnabled(QStringLiteral("upload"))) {
                        dragEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
        }
        if (event->type() == QEvent::Drop) {
            auto *dropEvent = static_cast<QDropEvent *>(event);
            if (dropEvent->mimeData() != nullptr && dropEvent->mimeData()->hasUrls()) {
                QStringList localPaths;
                for (const QUrl &url : dropEvent->mimeData()->urls()) {
                    if (url.isLocalFile()) {
                        localPaths << url.toLocalFile();
                    }
                }
                if (!localPaths.isEmpty()) {
                    dropEvent->acceptProposedAction();
                    uploadLocalFiles(localPaths);
                    return true;
                }
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}
