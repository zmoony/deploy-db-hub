#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteFileBrowser.h"

#include <QDialog>
#include <QJsonObject>

#include <memory>

class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QTabWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class RemoteFileBrowser;
class RemoteTerminalWidget;

class RemoteFileBrowserDialog final : public QDialog
{
    Q_OBJECT

public:
    enum class Purpose {
        Browse,
        PickDirectory
    };

    explicit RemoteFileBrowserDialog(const RemoteConnectionContext &connectionContext,
                                     QWidget *parent = nullptr,
                                     Purpose purpose = Purpose::Browse);

    void setInitialPath(const QString &path);
    QString selectedPath() const;

private slots:
    void pickCurrentDirectory();
    void refreshListing();
    void enterSelected();
    void onTableDoubleClicked(const QModelIndex &index);
    void openTerminalAtSelectedTreePath();
    void openTerminalAtSelectedEntryPath();
    void goUp();
    void goHome();
    void goBookmark();
    void viewSelectedFile();
    void editSelectedFile();
    void deleteSelectedEntry();
    void createDirectory();
    void uploadFile();
    void onTreeItemExpanded(QTreeWidgetItem *item);
    void onTreeSelectionChanged();
    void copySelectedEntry();
    void cutSelectedEntry();
    void pasteToCurrentDirectory();
    void renameSelectedEntry();
    void showSelectedProperties();
    void showTableContextMenu(const QPoint &pos);
    void showTreeContextMenu(const QPoint &pos);
    void syncTerminalToolbar();
    void buildTerminalToolbar();
    void syncPathFromTerminal();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    void setCurrentPath(const QString &path);
    void populateTable(const QVector<RemoteFileEntry> &entries);
    void applyListingResult(const RemoteFileListResult &result);
    void hydrateTreeNode(QTreeWidgetItem *item, const QVector<RemoteFileEntry> &entries);
    void ensureTreeBranch(const QString &path);
    void reloadTreeNode(const QString &path);
    void syncTreeSelection();
    QString selectedEntryPath() const;
    RemoteFileType selectedEntryType() const;
    bool selectedIsDirectory() const;
    bool selectedIsNavEntry() const;
    bool operationEnabled(const QString &name) const;
    bool hasPasteSource() const;
    QString formatSize(qint64 sizeBytes) const;
    void openFileEditor(const QString &remotePath, bool readOnly);
    void openTerminalAtPath(const QString &path, bool pathIsFile);
    void uploadLocalFiles(const QStringList &localPaths);
    QString remotePathInDirectory(const QString &name) const;
    QTreeWidgetItem *findTreeItem(const QString &path) const;

    RemoteConnectionContext m_connectionContext;
    QJsonObject m_serverConfig;
    std::unique_ptr<RemoteFileBrowser> m_browser;
    Purpose m_purpose = Purpose::Browse;
    QString m_currentPath;
    QString m_selectedPath;
    QString m_bookmarkPath;
    QString m_clipboardRemotePath;
    bool m_clipboardCut = false;
    int m_listingGeneration = 0;

    QLineEdit *m_pathEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QProgressBar *m_uploadProgress = nullptr;
    QTableWidget *m_table = nullptr;
    QTreeWidget *m_tree = nullptr;
    QSplitter *m_splitter = nullptr;
    QTabWidget *m_tabs = nullptr;
    QWidget *m_terminalToolbar = nullptr;
    RemoteTerminalWidget *m_terminal = nullptr;
    QPushButton *m_mkdirButton = nullptr;
    QPushButton *m_uploadButton = nullptr;
    QPushButton *m_viewButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_goButton = nullptr;
};
