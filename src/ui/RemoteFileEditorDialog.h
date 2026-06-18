#pragma once

#include <QDialog>
#include <functional>

class QPlainTextEdit;

class RemoteFileEditorDialog final : public QDialog
{
    Q_OBJECT

public:
    using SaveHandler = std::function<bool(const QString &content, QString *error)>;

    explicit RemoteFileEditorDialog(const QString &remotePath,
                                    const QString &content,
                                    bool readOnly,
                                    const SaveHandler &saveHandler,
                                    QWidget *parent = nullptr);

private slots:
    void onSave();

private:
    QString m_remotePath;
    SaveHandler m_saveHandler;
    QPlainTextEdit *m_editor = nullptr;
};
