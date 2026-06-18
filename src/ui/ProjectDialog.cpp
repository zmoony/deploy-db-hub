#include "ui/ProjectDialog.h"

#include "infra/ConfigValidator.h"
#include "ui/PageLayout.h"
#include "ui/PathPickerWidget.h"
#include "infra/AppBranding.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

void tuneField(QWidget *widget)
{
    PageLayout::configureFormInput(widget);
}

void tuneFormBox(QGroupBox *box, QFormLayout *form)
{
    box->setObjectName(QStringLiteral("dialogFormBox"));
    PageLayout::applyInlineForm(form);
}

}

ProjectDialog::ProjectDialog(const QVector<StoredRecord> &servers, QWidget *parent)
    : QDialog(parent)
    , m_servers(servers)
{
    setWindowTitle(QStringLiteral("项目配置"));
    setModal(true);
    PageLayout::applyModalDialog(this);
    AppBranding::applyWindowIcon(this);
    buildUi();
}

void ProjectDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *content = PageLayout::wrapScrollableBody(layout);
    auto *bodyLayout = new QVBoxLayout(content);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space16);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(PageLayout::Space24);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(PageLayout::Space16);
    auto *rightColumn = new QVBoxLayout;
    rightColumn->setSpacing(PageLayout::Space16);

    auto *basicBox = new QGroupBox(QStringLiteral("基本信息"));
    auto *basicForm = new QFormLayout(basicBox);
    tuneFormBox(basicBox, basicForm);
    m_id = new QLineEdit;
    m_id->setPlaceholderText(QStringLiteral("例如 app-demo"));
    m_name = new QLineEdit;
    m_type = new QComboBox;
    m_type->addItem(QStringLiteral("Java Maven"), QStringLiteral("java-maven"));
    m_type->addItem(QStringLiteral("前端静态包"), QStringLiteral("frontend-static"));
    m_type->addItem(QStringLiteral("Python"), QStringLiteral("python"));
    m_type->addItem(QStringLiteral("自定义"), QStringLiteral("custom"));
    tuneField(m_id);
    tuneField(m_name);
    tuneField(m_type);
    basicForm->addRow(QStringLiteral("项目 ID"), m_id);
    basicForm->addRow(QStringLiteral("名称"), m_name);
    basicForm->addRow(QStringLiteral("类型"), m_type);
    leftColumn->addWidget(basicBox);

    auto *sourceBox = new QGroupBox(QStringLiteral("源码来源"));
    auto *sourceForm = new QFormLayout(sourceBox);
    tuneFormBox(sourceBox, sourceForm);
    m_sourceKind = new QComboBox;
    m_sourceKind->addItem(QStringLiteral("Local 本地目录"), QStringLiteral("local"));
    m_sourceKind->addItem(QStringLiteral("GitHub"), QStringLiteral("github"));
    tuneField(m_sourceKind);
    sourceForm->addRow(QStringLiteral("来源"), m_sourceKind);
    connect(m_sourceKind, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProjectDialog::onSourceKindChanged);
    leftColumn->addWidget(sourceBox);

    auto *buildBox = new QGroupBox(QStringLiteral("构建"));
    auto *buildForm = new QFormLayout(buildBox);
    tuneFormBox(buildBox, buildForm);
    m_buildMode = new QComboBox;
    m_buildMode->addItem(QStringLiteral("本地构建"), QStringLiteral("build"));
    m_buildMode->addItem(QStringLiteral("上传已有 JAR"), QStringLiteral("prebuilt-jar"));
    m_command = new QLineEdit;
    m_command->setPlaceholderText(QStringLiteral("mvn clean package -DskipTests"));
    tuneField(m_buildMode);
    tuneField(m_command);
    buildForm->addRow(QStringLiteral("方式"), m_buildMode);
    buildForm->addRow(QStringLiteral("构建命令"), m_command);
    connect(m_buildMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProjectDialog::syncBuildModeFields);
    rightColumn->addWidget(buildBox);

    auto *deployBox = new QGroupBox(QStringLiteral("部署"));
    auto *deployForm = new QFormLayout(deployBox);
    tuneFormBox(deployBox, deployForm);
    m_serverId = new QComboBox;
    for (const StoredRecord &server : m_servers) {
        const QString label = server.config.value(QStringLiteral("name")).toString() + QStringLiteral(" (") + server.id + QLatin1Char(')');
        m_serverId->addItem(label, server.id);
    }
    m_failureStrategy = new QComboBox;
    m_failureStrategy->addItem(QStringLiteral("自动回滚"), QStringLiteral("rollback"));
    m_failureStrategy->addItem(QStringLiteral("保留现场"), QStringLiteral("keep"));
    tuneField(m_serverId);
    tuneField(m_failureStrategy);
    deployForm->addRow(QStringLiteral("目标服务器"), m_serverId);
    deployForm->addRow(QStringLiteral("失败策略"), m_failureStrategy);
    rightColumn->addWidget(deployBox);

    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);
    bodyLayout->addLayout(columns);

    auto *pathsBox = new QGroupBox(QStringLiteral("路径与目录"));
    auto *pathsForm = new QFormLayout(pathsBox);
    tuneFormBox(pathsBox, pathsForm);

    m_sourceStack = new QStackedWidget;
    PageLayout::configurePathField(m_sourceStack);
    auto *localPanel = new QWidget;
    auto *localLayout = new QVBoxLayout(localPanel);
    localLayout->setContentsMargins(0, 0, 0, 0);
    m_localPath = new PathPickerWidget(PathPickerWidget::Mode::Directory);
    m_localPath->setPlaceholderText(QStringLiteral("D:/projects/demo"));
    localLayout->addWidget(m_localPath);
    m_sourceStack->addWidget(localPanel);

    auto *githubPanel = new QWidget;
    auto *githubForm = new QFormLayout(githubPanel);
    PageLayout::applyInlineForm(githubForm);
    githubForm->setContentsMargins(0, 0, 0, 0);
    m_repoUrl = new QLineEdit;
    m_repoUrl->setPlaceholderText(QStringLiteral("https://github.com/org/repo.git"));
    m_ref = new QLineEdit;
    m_ref->setPlaceholderText(QStringLiteral("main / v1.0.0 / commit SHA"));
    PageLayout::configurePathField(m_repoUrl);
    PageLayout::configurePathField(m_ref);
    githubForm->addRow(QStringLiteral("仓库 URL"), m_repoUrl);
    githubForm->addRow(QStringLiteral("分支/Tag/Commit"), m_ref);
    m_sourceStack->addWidget(githubPanel);
    pathsForm->addRow(QStringLiteral("源码路径"), m_sourceStack);

    m_artifactPath = new PathPickerWidget(PathPickerWidget::Mode::File);
    m_artifactPath->setPlaceholderText(QStringLiteral("target/app.jar"));
    m_prebuiltJarPath = new PathPickerWidget(PathPickerWidget::Mode::File);
    m_prebuiltJarPath->setPlaceholderText(QStringLiteral("D:/build/app.jar"));
    m_workingDir = new PathPickerWidget(PathPickerWidget::Mode::Directory);
    m_workingDir->setPath(QStringLiteral("."));
    m_remoteBaseDir = new QLineEdit;
    m_remoteBaseDir->setPlaceholderText(QStringLiteral("/opt/deploy-hub/apps/demo"));
    m_logDir = new QLineEdit;
    m_logDir->setPlaceholderText(QStringLiteral("/opt/deploy-hub/apps/demo/logs"));
    m_restartScript = new PathPickerWidget(PathPickerWidget::Mode::File);
    m_restartScript->setPath(QStringLiteral("restart.sh"));
    m_targetJarPath = new QLineEdit;
    m_targetJarPath->setPlaceholderText(QStringLiteral("/home/demo/app.jar"));
    m_backupPolicy = new QComboBox;
    m_backupPolicy->addItem(QStringLiteral("备份旧包"), QStringLiteral("backup"));
    m_backupPolicy->addItem(QStringLiteral("直接替换"), QStringLiteral("replace"));
    m_backupDir = new QLineEdit;
    m_backupDir->setPlaceholderText(QStringLiteral("默认：远端目录/bak"));
    PageLayout::configurePathField(m_remoteBaseDir);
    PageLayout::configurePathField(m_logDir);
    PageLayout::configurePathField(m_targetJarPath);
    PageLayout::configurePathField(m_backupDir);
    tuneField(m_backupPolicy);
    pathsForm->addRow(QStringLiteral("产物路径"), m_artifactPath);
    pathsForm->addRow(QStringLiteral("本地 JAR"), m_prebuiltJarPath);
    pathsForm->addRow(QStringLiteral("工作目录"), m_workingDir);
    pathsForm->addRow(QStringLiteral("远端目录"), m_remoteBaseDir);
    pathsForm->addRow(QStringLiteral("目标 JAR"), m_targetJarPath);
    pathsForm->addRow(QStringLiteral("旧包策略"), m_backupPolicy);
    pathsForm->addRow(QStringLiteral("备份目录"), m_backupDir);
    pathsForm->addRow(QStringLiteral("日志目录"), m_logDir);
    pathsForm->addRow(QStringLiteral("重启脚本"), m_restartScript);
    bodyLayout->addWidget(pathsBox);

    auto *serviceBox = new QGroupBox(QStringLiteral("服务控制"));
    auto *serviceForm = new QFormLayout(serviceBox);
    tuneFormBox(serviceBox, serviceForm);
    m_serviceMatch = new QLineEdit;
    m_serviceMatch->setPlaceholderText(QStringLiteral("用于匹配进程命令行，例如 yz-wwa-gateway"));
    m_startCommand = new QLineEdit;
    m_startCommand->setPlaceholderText(QStringLiteral("nohup java -jar {targetJarPath} > {logDir}/app.log 2>&1 &"));
    m_stopCommand = new QLineEdit;
    m_stopCommand->setPlaceholderText(QStringLiteral("可为空，默认结束匹配到的 PID"));
    m_restartCommand = new QLineEdit;
    m_restartCommand->setPlaceholderText(QStringLiteral("可为空，默认执行停止后启动"));
    m_statusCommand = new QLineEdit;
    m_statusCommand->setPlaceholderText(QStringLiteral("可为空，默认按服务匹配查 PID"));
    tuneField(m_serviceMatch);
    tuneField(m_startCommand);
    tuneField(m_stopCommand);
    tuneField(m_restartCommand);
    tuneField(m_statusCommand);
    serviceForm->addRow(QStringLiteral("服务匹配"), m_serviceMatch);
    serviceForm->addRow(QStringLiteral("启动命令"), m_startCommand);
    serviceForm->addRow(QStringLiteral("停止命令"), m_stopCommand);
    serviceForm->addRow(QStringLiteral("重启命令"), m_restartCommand);
    serviceForm->addRow(QStringLiteral("状态命令"), m_statusCommand);
    bodyLayout->addWidget(serviceBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->setCenterButtons(false);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ProjectDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    syncSourceFields();
    syncBuildModeFields();
}

void ProjectDialog::setProject(const QJsonObject &project, bool editMode)
{
    m_editMode = editMode;
    m_id->setText(project.value(QStringLiteral("id")).toString());
    m_id->setReadOnly(editMode);
    m_name->setText(project.value(QStringLiteral("name")).toString());

    const QString type = project.value(QStringLiteral("type")).toString();
    const int typeIndex = m_type->findData(type);
    if (typeIndex >= 0) {
        m_type->setCurrentIndex(typeIndex);
    }

    const QJsonObject source = project.value(QStringLiteral("source")).toObject();
    const QString kind = source.value(QStringLiteral("kind")).toString();
    m_sourceKind->setCurrentIndex(kind == QStringLiteral("github") ? 1 : 0);
    m_localPath->setPath(source.value(QStringLiteral("localPath")).toString());
    m_repoUrl->setText(source.value(QStringLiteral("repoUrl")).toString());
    m_ref->setText(source.value(QStringLiteral("ref")).toString());

    const QJsonObject build = project.value(QStringLiteral("build")).toObject();
    const QString buildMode = build.value(QStringLiteral("mode")).toString() == QStringLiteral("prebuilt-jar")
        ? QStringLiteral("prebuilt-jar")
        : QStringLiteral("build");
    const int buildModeIndex = m_buildMode->findData(buildMode);
    if (buildModeIndex >= 0) {
        m_buildMode->setCurrentIndex(buildModeIndex);
    }
    m_command->setText(build.value(QStringLiteral("command")).toString());
    m_artifactPath->setPath(build.value(QStringLiteral("artifactPath")).toString());
    m_prebuiltJarPath->setPath(build.value(QStringLiteral("prebuiltJarPath")).toString());
    m_workingDir->setPath(build.value(QStringLiteral("workingDir")).toString(QStringLiteral(".")));

    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString serverId = deploy.value(QStringLiteral("serverId")).toString();
    const int serverIndex = m_serverId->findData(serverId);
    if (serverIndex >= 0) {
        m_serverId->setCurrentIndex(serverIndex);
    }
    m_remoteBaseDir->setText(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    m_logDir->setText(deploy.value(QStringLiteral("logDir")).toString());
    m_restartScript->setPath(deploy.value(QStringLiteral("restartScript")).toString(QStringLiteral("restart.sh")));
    m_targetJarPath->setText(deploy.value(QStringLiteral("targetJarPath")).toString());
    m_backupDir->setText(deploy.value(QStringLiteral("backupDir")).toString());
    const int backupIndex = m_backupPolicy->findData(deploy.value(QStringLiteral("backupPolicy")).toString(QStringLiteral("backup")));
    if (backupIndex >= 0) {
        m_backupPolicy->setCurrentIndex(backupIndex);
    }
    m_serviceMatch->setText(deploy.value(QStringLiteral("serviceMatch")).toString(project.value(QStringLiteral("id")).toString()));
    m_startCommand->setText(deploy.value(QStringLiteral("startCommand")).toString());
    m_stopCommand->setText(deploy.value(QStringLiteral("stopCommand")).toString());
    m_restartCommand->setText(deploy.value(QStringLiteral("restartCommand")).toString());
    m_statusCommand->setText(deploy.value(QStringLiteral("statusCommand")).toString());
    const QString strategy = deploy.value(QStringLiteral("failureStrategy")).toString();
    m_failureStrategy->setCurrentIndex(strategy == QStringLiteral("keep") ? 1 : 0);

    syncSourceFields();
    syncBuildModeFields();
}

QJsonObject ProjectDialog::project() const
{
    return QJsonObject{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), m_id->text().trimmed()},
        {QStringLiteral("name"), m_name->text().trimmed()},
        {QStringLiteral("type"), m_type->currentData().toString()},
        {QStringLiteral("source"), [&]() {
            const QString kind = m_sourceKind->currentData().toString();
            if (kind == QStringLiteral("github")) {
                return QJsonObject{
                    {QStringLiteral("kind"), kind},
                    {QStringLiteral("repoUrl"), m_repoUrl->text().trimmed()},
                    {QStringLiteral("ref"), m_ref->text().trimmed()},
                    {QStringLiteral("localPath"), QJsonValue::Null}
                };
            }
            return QJsonObject{
                {QStringLiteral("kind"), kind},
                {QStringLiteral("localPath"), m_localPath->path()}
            };
        }()},
        {QStringLiteral("build"), QJsonObject{
            {QStringLiteral("location"), QStringLiteral("local")},
            {QStringLiteral("mode"), m_buildMode->currentData().toString()},
            {QStringLiteral("workingDir"), m_workingDir->path().isEmpty() ? QStringLiteral(".") : m_workingDir->path()},
            {QStringLiteral("command"), m_command->text().trimmed()},
            {QStringLiteral("artifactPath"), m_buildMode->currentData().toString() == QStringLiteral("prebuilt-jar")
                ? m_prebuiltJarPath->path()
                : m_artifactPath->path()},
            {QStringLiteral("prebuiltJarPath"), m_prebuiltJarPath->path()},
            {QStringLiteral("artifactMatchPolicy"), QStringLiteral("fail-if-multiple")},
            {QStringLiteral("env"), QJsonObject{}},
            {QStringLiteral("timeoutSec"), 600}
        }},
        {QStringLiteral("deploy"), [&]() {
            QJsonObject deploy{
                {QStringLiteral("serverId"), m_serverId->currentData().toString()},
                {QStringLiteral("remoteBaseDir"), m_remoteBaseDir->text().trimmed()},
                {QStringLiteral("restartScript"), m_restartScript->path()},
                {QStringLiteral("failureStrategy"), m_failureStrategy->currentData().toString()},
                {QStringLiteral("backupPolicy"), m_backupPolicy->currentData().toString()}
            };
            const QList<QPair<QString, QString>> optionalFields{
                {QStringLiteral("logDir"), m_logDir->text().trimmed()},
                {QStringLiteral("serviceMatch"), m_serviceMatch->text().trimmed()},
                {QStringLiteral("startCommand"), m_startCommand->text().trimmed()},
                {QStringLiteral("stopCommand"), m_stopCommand->text().trimmed()},
                {QStringLiteral("restartCommand"), m_restartCommand->text().trimmed()},
                {QStringLiteral("statusCommand"), m_statusCommand->text().trimmed()},
                {QStringLiteral("targetJarPath"), m_targetJarPath->text().trimmed()},
                {QStringLiteral("backupDir"), m_backupDir->text().trimmed()}
            };
            for (const auto &field : optionalFields) {
                if (!field.second.isEmpty()) {
                    deploy.insert(field.first, field.second);
                }
            }
            return deploy;
        }()}
    };
}

void ProjectDialog::onSourceKindChanged(int)
{
    syncSourceFields();
}

void ProjectDialog::syncSourceFields()
{
    m_sourceStack->setCurrentIndex(m_sourceKind->currentIndex());
    syncSourceStackHeight();
}

void ProjectDialog::syncSourceStackHeight()
{
    if (m_sourceStack == nullptr || m_sourceStack->currentWidget() == nullptr) {
        return;
    }

    const int height = m_sourceStack->currentWidget()->sizeHint().height();
    m_sourceStack->setMinimumHeight(height);
    m_sourceStack->setMaximumHeight(height);
    m_sourceStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ProjectDialog::syncBuildModeFields()
{
    const bool prebuilt = m_buildMode != nullptr
        && m_buildMode->currentData().toString() == QStringLiteral("prebuilt-jar");
    if (m_command != nullptr) {
        m_command->setEnabled(!prebuilt);
    }
    if (m_artifactPath != nullptr) {
        m_artifactPath->setEnabled(!prebuilt);
    }
    if (m_workingDir != nullptr) {
        m_workingDir->setEnabled(!prebuilt);
    }
    if (m_prebuiltJarPath != nullptr) {
        m_prebuiltJarPath->setEnabled(prebuilt);
    }
}

void ProjectDialog::onAccept()
{
    if (m_serverId->count() == 0) {
        QMessageBox::warning(this, QStringLiteral("无法保存"), QStringLiteral("请先新增至少一台服务器。"));
        return;
    }

    const QJsonObject config = project();
    const ValidationResult validation = ConfigValidator::validateProject(config);
    if (!validation.ok) {
        QMessageBox::warning(this, QStringLiteral("配置无效"), validation.errors.join(QStringLiteral("\n")));
        return;
    }
    accept();
}
