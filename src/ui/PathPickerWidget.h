#pragma once

#include <functional>

#include <QString>

#include <QWidget>

class QLineEdit;

class PathPickerWidget final : public QWidget
{
    Q_OBJECT

public:
    enum class Mode {
        Directory,
        File
    };

    explicit PathPickerWidget(Mode mode, QWidget *parent = nullptr);

    void setPath(const QString &path);
    QString path() const;
    void setPlaceholderText(const QString &text);
    void setBrowseHandler(std::function<void()> handler);
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

signals:
    void pathChanged(const QString &path);

private slots:
    void browse();

private:
    Mode m_mode = Mode::Directory;
    QLineEdit *m_path = nullptr;
    std::function<void()> m_browseHandler;
};
