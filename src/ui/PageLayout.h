#pragma once

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QUrl>

class QButtonGroup;
class QMainWindow;
class QDialog;
class QGroupBox;
class QFormLayout;
class QFrame;
class QLabel;
class QListWidget;
class QScrollArea;
class QStackedWidget;
class LineTabBarController;
class QPushButton;
class QTableWidget;
class QVBoxLayout;
class QWidget;

namespace PageLayout {

constexpr int Space4 = 4;
constexpr int Space6 = 6;
constexpr int Space8 = 8;
constexpr int Space10 = 10;
constexpr int Space12 = 12;
constexpr int Space14 = 12;
constexpr int Space16 = 12;
constexpr int Space20 = 16;
constexpr int Space24 = 16;
constexpr int DialogMinWidth = 860;
constexpr int DialogMinHeight = 480;
constexpr int DialogDefaultWidth = 920;
constexpr int DialogDefaultHeight = 540;
constexpr int CompactDialogMinWidth = 480;
constexpr int CompactDialogMinHeight = 280;
constexpr int CompactDialogDefaultWidth = 520;
constexpr int CompactDialogDefaultHeight = 300;
constexpr int FormDialogMinWidth = 600;
constexpr int FormDialogMinHeight = 440;
constexpr int FormDialogDefaultWidth = 680;
constexpr int FormDialogDefaultHeight = 540;
constexpr int MainWindowDefaultWidth = 1464;
constexpr int MainWindowDefaultHeight = 820;
constexpr int MainWindowMinWidth = 960;
constexpr int MainWindowMinHeight = 640;
constexpr int SidebarNavWidth = 160;
constexpr int DialogFieldMinWidth = 240;
constexpr int DialogFieldHeight = 32;

void applyPage(QVBoxLayout *layout);
void applyDialog(QVBoxLayout *layout);
void applyForm(QFormLayout *form);
void applyDialogForm(QFormLayout *form);
void applyInlineForm(QFormLayout *form);
void applyMetricCard(QVBoxLayout *layout);
void applyModalDialog(QDialog *dialog);
void applyCompactModalDialog(QDialog *dialog,
                             int preferredWidth = CompactDialogDefaultWidth,
                             int preferredHeight = CompactDialogDefaultHeight);
void applyFormModalDialog(QDialog *dialog,
                          int preferredWidth = FormDialogDefaultWidth,
                          int preferredHeight = FormDialogDefaultHeight);
void applyRemoteToolDialog(QDialog *dialog,
                           int minWidth = 800,
                           int minHeight = 460,
                           int preferredWidth = 1100,
                           int preferredHeight = 640);
void enableResizableWindow(QWidget *window, bool applyExpandingPolicy = true);
void fitWindowToScreen(QWidget *window, int preferredWidth, int preferredHeight, qreal maxScreenRatio = 0.88);
void applyMainWindowGeometry(QMainWindow *window);
void configureFormInput(QWidget *widget);
void configurePathField(QWidget *widget);
void configureHorizontalFormRow(QWidget *row);
void tuneDialogFormBox(QGroupBox *box, QFormLayout *form);
QWidget *wrapScrollableBody(QVBoxLayout *dialogLayout);
QWidget *wrapDialogFormSection(const QString &title, QWidget *parent, QFormLayout **formOut = nullptr);
QWidget *wrapDialogHintSection(const QString &title, QWidget *parent, QLabel **textOut = nullptr);
QWidget *makeDialogFooter(QWidget *parent);

QLabel *makePageTitle(const QString &text, QWidget *parent);
QLabel *makePageSubtitle(const QString &text, QWidget *parent);
QLabel *makeSectionLabel(const QString &text, QWidget *parent);
QLabel *makeRequiredFormLabel(const QString &text, QWidget *parent);
QWidget *makeHeaderBlock(const QString &title, const QString &subtitle, QWidget *parent);
QWidget *makeModuleTabBar(const QStringList &labels,
                          QWidget *parent,
                          QButtonGroup **groupOut,
                          QStackedWidget *stack = nullptr,
                          int defaultIndex = 0);
QWidget *makeLineTabBar(const QStringList &labels,
                        QWidget *parent,
                        LineTabBarController **controllerOut = nullptr,
                        QStackedWidget *stack = nullptr,
                        int defaultIndex = 0);
QWidget *makeTabBar(const QStringList &labels,
                    QWidget *parent,
                    QButtonGroup **groupOut,
                    QStackedWidget *stack,
                    int defaultIndex = 0);

QWidget *makeQmlContentPage(const QUrl &source,
                            const QList<QPair<QString, QObject *>> &contextObjects,
                            QWidget *parent);

QWidget *wrapContentPanel(QWidget *page);
QWidget *wrapScrollableContentPanel(QWidget *page);
QListWidget *createSidebarNavigationList(QWidget *parent = nullptr);
QWidget *wrapSidebarNavigation(QListWidget *navigation, QPushButton **settingsButtonOut = nullptr);
void configureDataTable(QTableWidget *table);
void configureListingTable(QTableWidget *table, int stretchColumn = -1);
void configureListingTableWithActionColumn(QTableWidget *table, int actionColumn, int stretchColumn = 1);
void refreshListingTableColumns(QTableWidget *table, int stretchColumn = -1);
void ensureTableActionColumnWidth(QTableWidget *table, int actionColumn);
QPushButton *makeTableActionButton(const QString &label, const QString &objectName, QWidget *parent);
QWidget *wrapTableSection(QTableWidget *table, QLabel **emptyStateOut, const QString &emptyMessage);
void updateTableEmptyState(QTableWidget *table, QLabel *emptyState, int rowCount);

}
