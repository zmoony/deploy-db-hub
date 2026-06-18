#pragma once

#include <QString>

class QDialog;
class QFormLayout;
class QFrame;
class QLabel;
class QScrollArea;
class QTableWidget;
class QVBoxLayout;
class QWidget;

namespace PageLayout {

constexpr int Space8 = 8;
constexpr int Space12 = 12;
constexpr int Space16 = 16;
constexpr int Space24 = 24;
constexpr int DialogMinWidth = 860;
constexpr int DialogMinHeight = 480;
constexpr int DialogDefaultWidth = 920;
constexpr int DialogDefaultHeight = 540;
constexpr int MainWindowDefaultWidth = 1280;
constexpr int MainWindowDefaultHeight = 720;
constexpr int DialogFieldMinWidth = 240;
constexpr int DialogFieldHeight = 36;

void applyPage(QVBoxLayout *layout);
void applyDialog(QVBoxLayout *layout);
void applyForm(QFormLayout *form);
void applyDialogForm(QFormLayout *form);
void applyInlineForm(QFormLayout *form);
void applyMetricCard(QVBoxLayout *layout);
void applyModalDialog(QDialog *dialog);
void applyRemoteToolDialog(QDialog *dialog,
                           int minWidth = 800,
                           int minHeight = 460,
                           int preferredWidth = 1100,
                           int preferredHeight = 640);
void enableResizableWindow(QWidget *window);
void fitWindowToScreen(QWidget *window, int preferredWidth, int preferredHeight, qreal maxScreenRatio = 0.88);
void configureFormInput(QWidget *widget);
void configurePathField(QWidget *widget);
QWidget *wrapScrollableBody(QVBoxLayout *dialogLayout);

QLabel *makePageTitle(const QString &text, QWidget *parent);
QLabel *makePageSubtitle(const QString &text, QWidget *parent);
QLabel *makeSectionLabel(const QString &text, QWidget *parent);
QWidget *makeHeaderBlock(const QString &title, const QString &subtitle, QWidget *parent);

QWidget *wrapContentPanel(QWidget *page);
void configureDataTable(QTableWidget *table);
QWidget *wrapTableSection(QTableWidget *table, QLabel **emptyStateOut, const QString &emptyMessage);
void updateTableEmptyState(QTableWidget *table, QLabel *emptyState, int rowCount);

}
