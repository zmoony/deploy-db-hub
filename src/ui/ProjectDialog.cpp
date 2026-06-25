#include "ui/ProjectDialog.h"

#include "infra/ConfigValidator.h"
#include "infra/ProjectServiceConfig.h"
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
    PageLayout::tuneDialogFormBox(box, form);
}

QLabel *addFormRow(QFormLayout *form, const QString &labelText, QWidget *field)
{
    auto *label = new QLabel(labelText);
    form->addRow(label, field);
    return label;
}

void setFormRowVisible(QLabel *label, QWidget *field, bool visible)
{
    if (label != nullptr) {
        label->setVisible(visible);
    }
    if (field != nullptr) {
        field->setVisible(visible);
    }
}

}

ProjectDialog::ProjectDialog(const QVector<StoredRecord> &servers, QWidget *parent)
    : QDialog(parent)
    , m_servers(servers)
{
    setWindowTitle(QStringLiteral("项目配置"));
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
    m_group = new QLineEdit;
    m_group->setPlaceholderText(QStringLiteral("例如 网关 / WVA / 公共"));
    m_type = new QComboBox;
    m_type->addItem(QStringLiteral("Java Maven"), QStringLiteral("java-maven"));
    m_type->addItem(QStringLiteral("前端静态包"), QStringLiteral("frontend-static"));
    m_type->addItem(QStringLiteral("Python"), QStringLiteral("python"));
    m_type->addItem(QStringLiteral("自定义"), QStringLiteral("custom"));
    tuneField(m_id);
    tuneField(m_name);
    tuneField(m_group);
    tuneField(m_type);
    basicForm->addRow(QStringLiteral("项目 ID"), m_id);
    basicForm->addRow(QStringLiteral("名称"), m_name);
    basicForm->addRow(QStringLiteral("分组"), m_group);
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
    m_buildModeLabel = addFormRow(buildForm, QStringLiteral("方式"), m_buildMode);
    addFormRow(buildForm, QStringLiteral("构建命令"), m_command);
    connect(m_buildMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProjectDialog::syncBuildModeFields);
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProjectDialog::onProjectTypeChanged);
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
    m_sourceStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    PageLayout::configureHorizontalFormRow(m_sourceStack);
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
    m_artifactRename = new QLineEdit;
    m_artifactRename->setPlaceholderText(QStringLiteral("留空则直接部署到远端目录，例如 myapp"));
    m_remoteBaseDir = new QLineEdit;
    m_remoteBaseDir->setPlaceholderText(QStringLiteral("/opt/deploy-hub/apps/demo"));
    m_logDir = new QLineEdit;
    m_logDir->setPlaceholderText(QStringLiteral("/opt/deploy-hub/apps/demo/logs"));
    m_targetJarPath = new QLineEdit;
    m_targetJarPath->setPlaceholderText(QStringLiteral("/home/demo/app.jar"));
    m_backupPolicy = new QComboBox;
    m_backupPolicy->addItem(QStringLiteral("备份旧包"), QStringLiteral("backup"));
    m_backupPolicy->addItem(QStringLiteral("直接替换"), QStringLiteral("replace"));
    m_backupDir = new QLineEdit;
    m_backupDir->setPlaceholderText(QStringLiteral("远端目录下的子目录名，留空默认 bak"));
    PageLayout::configurePathField(m_remoteBaseDir);
    PageLayout::configurePathField(m_logDir);
    PageLayout::configurePathField(m_targetJarPath);
    PageLayout::configurePathField(m_backupDir);
    PageLayout::configurePathField(m_artifactRename);
    tuneField(m_backupPolicy);
    tuneField(m_artifactRename);

    m_artifactPathLabel = addFormRow(pathsForm, QStringLiteral("产物路径"), m_artifactPath);
    m_prebuiltJarPathLabel = addFormRow(pathsForm, QStringLiteral("本地 JAR"), m_prebuiltJarPath);
    addFormRow(pathsForm, QStringLiteral("工作目录"), m_workingDir);
    m_artifactRenameLabel = addFormRow(pathsForm, QStringLiteral("部署目录名"), m_artifactRename);
    pathsForm->addRow(QStringLiteral("远端目录"), m_remoteBaseDir);
    m_targetJarPathLabel = addFormRow(pathsForm, QStringLiteral("目标 JAR"), m_targetJarPath);
    m_backupPolicyLabel = addFormRow(pathsForm, QStringLiteral("旧包策略"), m_backupPolicy);
    m_backupDirLabel = addFormRow(pathsForm, QStringLiteral("备份目录"), m_backupDir);
    m_logDirLabel = addFormRow(pathsForm, QStringLiteral("日志目录"), m_logDir);
    bodyLayout->addWidget(pathsBox);

    connect(m_backupPolicy, qOverload<int>(&QComboBox::currentIndexChanged), this, &ProjectDialog::syncBuildModeFields);

    m_restartModeBox = new QGroupBox(QStringLiteral("启动方式"));
    auto *restartModeLayout = new QVBoxLayout(m_restartModeBox);
    restartModeLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    restartModeLayout->setSpacing(PageLayout::Space12);

    m_restartMode = new QComboBox(m_restartModeBox);
    m_restartMode->addItem(QStringLiteral("重启脚本"), QStringLiteral("restart-script"));
    m_restartMode->addItem(QStringLiteral("自定义命令"), QStringLiteral("service-command"));
    tuneField(m_restartMode);
    auto *modeForm = new QFormLayout;
    PageLayout::applyInlineForm(modeForm);
    modeForm->addRow(QStringLiteral("方式"), m_restartMode);
    restartModeLayout->addLayout(modeForm);

    m_restartModeStack = new QStackedWidget(m_restartModeBox);
    m_restartModeStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *scriptPage = new QWidget;
    auto *scriptForm = new QFormLayout(scriptPage);
    PageLayout::applyInlineForm(scriptForm);
    scriptForm->setContentsMargins(0, 0, 0, 0);
    m_restartScript = new PathPickerWidget(PathPickerWidget::Mode::File);
    m_restartScript->setPath(QStringLiteral("restart.sh"));
    scriptForm->addRow(QStringLiteral("脚本路径"), m_restartScript);
    m_restartModeStack->addWidget(scriptPage);

    auto *servicePage = new QWidget;
    auto *serviceForm = new QFormLayout(servicePage);
    PageLayout::applyInlineForm(serviceForm);
    serviceForm->setContentsMargins(0, 0, 0, 0);
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
    m_restartModeStack->addWidget(servicePage);

    restartModeLayout->addWidget(m_restartModeStack);
    bodyLayout->addWidget(m_restartModeBox);

    connect(m_restartMode, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        syncRestartModePanel();
    });

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);
    footerLayout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, footer);
    buttons->setCenterButtons(false);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ProjectDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    syncSourceFields();
    syncBuildModeFields();
    syncRestartModePanel();
    syncProjectTypeFields();
}

QString ProjectDialog::currentProjectType() const
{
    return m_type != nullptr ? m_type->currentData().toString() : QStringLiteral("java-maven");
}

bool ProjectDialog::isFrontendStatic() const
{
    return currentProjectType() == QStringLiteral("frontend-static");
}

bool ProjectDialog::isJavaMaven() const
{
    return currentProjectType() == QStringLiteral("java-maven");
}

void ProjectDialog::onProjectTypeChanged(int)
{
    const QString type = currentProjectType();
    if (type == QStringLiteral("java-maven")) {
        if (m_command->text().trimmed().isEmpty() || m_command->text().trimmed() == QStringLiteral("npm run build")) {
            m_command->setText(QStringLiteral("mvn clean package -DskipTests"));
        }
        m_command->setPlaceholderText(QStringLiteral("mvn clean package -DskipTests"));
        if (m_artifactPath->path().trimmed().isEmpty() || m_artifactPath->path().trimmed() == QStringLiteral("dist")) {
            m_artifactPath->setPath(QStringLiteral("target/*.jar"));
        }
        m_artifactPath->setPlaceholderText(QStringLiteral("target/*.jar"));
        m_artifactPath->setMode(PathPickerWidget::Mode::File);
        m_buildMode->setCurrentIndex(0);
    } else if (type == QStringLiteral("frontend-static")) {
        if (m_command->text().trimmed().isEmpty() || m_command->text().trimmed() == QStringLiteral("mvn clean package -DskipTests")) {
            m_command->setText(QStringLiteral("npm run build"));
        }
        m_command->setPlaceholderText(QStringLiteral("npm run build"));
        if (m_artifactPath->path().trimmed().isEmpty() || m_artifactPath->path().trimmed() == QStringLiteral("target/*.jar")) {
            m_artifactPath->setPath(QStringLiteral("dist"));
        }
        m_artifactPath->setPlaceholderText(QStringLiteral("dist"));
        m_artifactPath->setMode(PathPickerWidget::Mode::Directory);
        m_buildMode->setCurrentIndex(0);
    } else {
        m_command->setPlaceholderText(QStringLiteral("例如 npm run build / python setup.py install"));
        m_artifactPath->setPlaceholderText(QStringLiteral("例如 dist/ / target/*.whl"));
        m_artifactPath->setMode(PathPickerWidget::Mode::File);
        m_buildMode->setCurrentIndex(0);
    }
    syncProjectTypeFields();
}

void ProjectDialog::syncProjectTypeFields()
{
    const bool frontend = isFrontendStatic();
    const bool java = isJavaMaven();

    setFormRowVisible(m_buildModeLabel, m_buildMode, java);
    setFormRowVisible(m_prebuiltJarPathLabel, m_prebuiltJarPath, java && m_buildMode->currentData().toString() == QStringLiteral("prebuilt-jar"));
    setFormRowVisible(m_targetJarPathLabel, m_targetJarPath, java);
    setFormRowVisible(m_backupPolicyLabel, m_backupPolicy, java);
    setFormRowVisible(m_backupDirLabel, m_backupDir, java && m_backupPolicy->currentData().toString() == QStringLiteral("backup"));
    setFormRowVisible(m_artifactRenameLabel, m_artifactRename, frontend);
    if (m_restartModeBox != nullptr) {
        m_restartModeBox->setVisible(!frontend);
    }

    if (m_artifactPathLabel != nullptr) {
        m_artifactPathLabel->setText(frontend ? QStringLiteral("产物目录") : QStringLiteral("产物路径"));
    }

    syncBuildModeFields();
}

void ProjectDialog::setProject(const QJsonObject &project, bool editMode)
{
    m_editMode = editMode;
    m_id->setText(project.value(QStringLiteral("id")).toString());
    m_id->setReadOnly(editMode);
    m_name->setText(project.value(QStringLiteral("name")).toString());
    m_group->setText(project.value(QStringLiteral("group")).toString());

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
    m_artifactRename->setText(build.value(QStringLiteral("artifactRename")).toString());

    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString serverId = deploy.value(QStringLiteral("serverId")).toString();
    const int serverIndex = m_serverId->findData(serverId);
    if (serverIndex >= 0) {
        m_serverId->setCurrentIndex(serverIndex);
    }
    m_remoteBaseDir->setText(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    m_logDir->setText(deploy.value(QStringLiteral("logDir")).toString());
    m_restartScript->setPath(deploy.value(QStringLiteral("restartScript")).toString(QStringLiteral("restart.sh")));
    const QString restartMode = deploy.value(QStringLiteral("restartMode")).toString();
    if (restartMode == QStringLiteral("service-command")
        || (restartMode.isEmpty() && usesCustomServiceControl(project))) {
        m_restartMode->setCurrentIndex(m_restartMode->findData(QStringLiteral("service-command")));
    } else {
        m_restartMode->setCurrentIndex(m_restartMode->findData(QStringLiteral("restart-script")));
    }
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

    onProjectTypeChanged(m_type->currentIndex());
    syncSourceFields();
    syncBuildModeFields();
    syncRestartModePanel();
}

QJsonObject ProjectDialog::project() const
{
    const bool frontend = isFrontendStatic();
    const bool java = isJavaMaven();
    const QString buildMode = java ? m_buildMode->currentData().toString() : QStringLiteral("build");
    const bool prebuilt = buildMode == QStringLiteral("prebuilt-jar");

    QJsonObject buildConfig{
        {QStringLiteral("location"), QStringLiteral("local")},
        {QStringLiteral("mode"), buildMode},
        {QStringLiteral("workingDir"), m_workingDir->path().isEmpty() ? QStringLiteral(".") : m_workingDir->path()},
        {QStringLiteral("command"), m_command->text().trimmed()},
        {QStringLiteral("artifactPath"), prebuilt ? m_prebuiltJarPath->path() : m_artifactPath->path()},
        {QStringLiteral("artifactMatchPolicy"), QStringLiteral("fail-if-multiple")},
        {QStringLiteral("env"), QJsonObject{}},
        {QStringLiteral("timeoutSec"), 600}
    };
    if (java) {
        buildConfig.insert(QStringLiteral("prebuiltJarPath"), m_prebuiltJarPath->path());
    }
    if (frontend && !m_artifactRename->text().trimmed().isEmpty()) {
        buildConfig.insert(QStringLiteral("artifactRename"), m_artifactRename->text().trimmed());
    }

    QJsonObject deployConfig{
        {QStringLiteral("serverId"), m_serverId->currentData().toString()},
        {QStringLiteral("remoteBaseDir"), m_remoteBaseDir->text().trimmed()},
        {QStringLiteral("failureStrategy"), m_failureStrategy->currentData().toString()}
    };

    if (!frontend) {
        const QString mode = m_restartMode->currentData().toString();
        const bool serviceMode = mode == QStringLiteral("service-command");
        deployConfig.insert(QStringLiteral("restartMode"), mode);
        if (java) {
            deployConfig.insert(QStringLiteral("backupPolicy"), m_backupPolicy->currentData().toString());
        } else {
            deployConfig.insert(QStringLiteral("backupPolicy"), QStringLiteral("replace"));
        }
        if (serviceMode) {
            const QList<QPair<QString, QString>> serviceFields{
                {QStringLiteral("serviceMatch"), m_serviceMatch->text().trimmed()},
                {QStringLiteral("startCommand"), m_startCommand->text().trimmed()},
                {QStringLiteral("stopCommand"), m_stopCommand->text().trimmed()},
                {QStringLiteral("restartCommand"), m_restartCommand->text().trimmed()},
                {QStringLiteral("statusCommand"), m_statusCommand->text().trimmed()},
            };
            for (const auto &field : serviceFields) {
                if (!field.second.isEmpty()) {
                    deployConfig.insert(field.first, field.second);
                }
            }
        } else {
            deployConfig.insert(QStringLiteral("restartScript"), m_restartScript->path());
        }
        const QList<QPair<QString, QString>> sharedFields{
            {QStringLiteral("logDir"), m_logDir->text().trimmed()},
            {QStringLiteral("targetJarPath"), java ? m_targetJarPath->text().trimmed() : QString()},
            {QStringLiteral("backupDir"), java ? m_backupDir->text().trimmed() : QString()}
        };
        for (const auto &field : sharedFields) {
            if (!field.second.isEmpty()) {
                deployConfig.insert(field.first, field.second);
            }
        }
    } else {
        if (!m_logDir->text().trimmed().isEmpty()) {
            deployConfig.insert(QStringLiteral("logDir"), m_logDir->text().trimmed());
        }
    }

    return QJsonObject{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), m_id->text().trimmed()},
        {QStringLiteral("name"), m_name->text().trimmed()},
        {QStringLiteral("group"), m_group->text().trimmed()},
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
        {QStringLiteral("build"), buildConfig},
        {QStringLiteral("deploy"), deployConfig}
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
    const bool java = isJavaMaven();
    const bool prebuilt = java && m_buildMode != nullptr
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
    setFormRowVisible(m_prebuiltJarPathLabel, m_prebuiltJarPath, prebuilt);

    if (java && m_backupPolicy != nullptr && m_backupDirLabel != nullptr) {
        const bool backup = m_backupPolicy->currentData().toString() == QStringLiteral("backup");
        setFormRowVisible(m_backupDirLabel, m_backupDir, backup);
    }
}

bool ProjectDialog::isServiceCommandMode() const
{
    return !isFrontendStatic()
        && m_restartMode != nullptr
        && m_restartMode->currentData().toString() == QStringLiteral("service-command");
}

void ProjectDialog::syncRestartModePanel()
{
    if (m_restartModeStack == nullptr || m_restartMode == nullptr) {
        return;
    }
    m_restartModeStack->setCurrentIndex(m_restartMode->currentIndex());
    if (QWidget *page = m_restartModeStack->currentWidget()) {
        const int height = page->sizeHint().height();
        m_restartModeStack->setMinimumHeight(height);
        m_restartModeStack->setMaximumHeight(height);
    }
}

void ProjectDialog::onAccept()
{
    if (m_serverId->count() == 0) {
        QMessageBox::warning(this, QStringLiteral("无法保存"), QStringLiteral("请先新增至少一台服务器。"));
        return;
    }
    if (!isFrontendStatic() && !isServiceCommandMode() && m_restartScript->path().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("配置无效"), QStringLiteral("重启脚本模式下请填写脚本路径。"));
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
