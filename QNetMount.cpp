#include "QNetMount.h"
#include "ui_QNetMount.h"
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QRegExp>
#include <QStandardPaths>
#include <QTimer>

QNetMount::QNetMount(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::QNetMount),
    m_confgPath(QApplication::applicationFilePath() + ".json") {
    ui->setupUi(this);
    ui->toolButtonSettingCtrlSave->hide(); // 已经自动保存，这里暂时用不到
    ui->tabWidgetCentral->setCurrentWidget(ui->index);
    ui->tabWidgetStore->setCurrentWidget(ui->manager);
    initAcitons();
    initTrayIcon();
    readConfig();

    // realtime update config file
    connect(ui->checkBoxSettingCtrlStartHide, &QCheckBox::checkStateChanged, this,
            &QNetMount::updateToCofigFile);
    connect(ui->checkBoxSettingCtrlAlist, &QCheckBox::checkStateChanged, this,
            &QNetMount::updateToCofigFile);
    connect(ui->lineEditAlistExecPath, &QLineEdit::textChanged, this,
            &QNetMount::updateToCofigFile);
    connect(ui->lineEditAlistConfigPath, &QLineEdit::textChanged, this,
            &QNetMount::updateToCofigFile);

#ifdef _DEBUG
    setWindowIcon(style()->standardIcon(QStyle::SP_TitleBarMenuButton));
#else
    setWindowIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
#endif // _DEBUG
}

QNetMount::~QNetMount() {
    stopAlist();
    delete ui;
    while (m_actions.size()) {
        auto ite = m_actions.begin();
        delete ite.value();
        m_actions.remove(ite.key());
    }
}

void QNetMount::on_tabWidgetCentral_currentChanged(int index) {
    Q_UNUSED(index);
    if (this->isMaximized() || this->isFullScreen() || this->isMinimized()) {
        return;
    }
    // const int minSize[7][2] = {
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 530 },
    //     { 366, 281 },
    // };
    // int w = minSize[index][0];
    // int h = minSize[index][1];
    bool spec = ui->tabWidgetCentral->currentWidget() == ui->setting;
    int w = 366;
    int h = spec ? 530 : 281;
    this->setMinimumSize(w, h);
    this->resize(w, h);
}

void QNetMount::initAcitons() {
    if (!m_actions.contains("AboutQNetMount")) {
        QAction *action = new QAction;
        action->setText("About QNetMount");
        connect(action, &QAction::triggered, this, [this]() {
            QMessageBox::about(
                this, "About QNetMount",
                "模仿 NetMount 做的 GUI 界面，目前仅作为 Alist 的启动器。"
                "<br>支持托盘图标；"
                "<br>支持从当前程序修改 AList 管理员密码。"
                "<br>支持自定义 Alist 路径"
                "<br>Coding Time : 2024-09-06 11:00");
        });
        m_actions.insert("AboutQNetMount", action);
    }
    if (!m_actions.contains("AboutNetMount")) {
        QAction *action = new QAction(this);
        action->setText("About NetMount");
        connect(action, &QAction::triggered, this, [this]() {
            QMessageBox::about(
                this, "About NetMount",
                "<a style='color: lightblue;' "
                "href='https://www.netmount.cn'>NetMount</a>"
                "<br>统一管理和挂载云存储设施"
                "<br>GitHub : <a style='color: lightblue;' "
                "href='https://github.com/VirtualHotBar/NetMount/'>NetMount</a>");
        });
        m_actions.insert("AboutNetMount", action);
    }
    if (!m_actions.contains("AboutAlist")) {
        QAction *action = new QAction(this);
        action->setText("About Alist");
        connect(action, &QAction::triggered, this, [this]() {
            QMessageBox::about(
                this, "About Alist",
                "<a style='color: lightblue;' href='https://alist.nn.ci/'>AList</a>"
                "<br>一个支持多存储的文件列表/WebDAV程序"
                "<br>GitHub : <a style='color: lightblue;' "
                "href='https://github.com/alist-org/alist'>AList</a>");
        });
        m_actions.insert("AboutAlist", action);
    }
    if (!m_actions.contains("AboutRcClone")) {
        QAction *action = new QAction(this);
        action->setText("About RcClone");
        connect(action, &QAction::triggered, this, [this]() {
            QMessageBox::about(
                this, "About RcClone",
                "<a style='color: lightblue;' href='https://rclone.org/'>RcClone</a>"
                "<br>rsync for cloud storage"
                "<br>GitHub : <a style='color: lightblue;' "
                "href='https://github.com/rclone/rclone'>RcClone</a>");
        });
        m_actions.insert("AboutRcClone", action);
    }
    if (!m_actions.contains("AboutQt")) {
        QAction *actionAboutQt = new QAction(this);
        actionAboutQt->setText("AboutQt");
        connect(actionAboutQt, &QAction::triggered, this,
                []() { QApplication::aboutQt(); });
        m_actions.insert("AboutQt", actionAboutQt);
    }
    if (!m_actions.contains("Show")) {
        QAction *actionShow = new QAction(this);
        actionShow->setText("Hide");
        connect(actionShow, &QAction::triggered, this, [this, actionShow]() {
            if (this->isHidden()) {
                this->show();
                actionShow->setText("Hide");
                this->activateWindow();
            } else if (this->isVisible()) {
                this->hide();
                actionShow->setText("Show");
            }
        });
        m_actions.insert("Show", actionShow);
    }
    if (!m_actions.contains("Exit")) {
        QAction *actionExit = new QAction(this);
        actionExit->setText("Exit");
        connect(actionExit, &QAction::triggered, this,
                []() { QApplication::exit(); });
        m_actions.insert("Exit", actionExit);
    }
    // QToolbutton
    if (!m_actions.contains("ChooseAlistExecPath")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonChooseAlistExecPath->text());
        ui->toolButtonChooseAlistExecPath->setDefaultAction(action);
        connect(action, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getOpenFileName(
                this, "select alist.exe", ui->lineEditAlistExecPath->text(), "*.exe");
            if (!path.isEmpty()) {
                ui->lineEditAlistExecPath->setText(path);
            }
        });
        m_actions.insert("ChooseAlistExecPath", action);
    }
    if (!m_actions.contains("ChooseAlistConfigPath")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonChooseAlistConfigPath->text());
        ui->toolButtonChooseAlistConfigPath->setDefaultAction(action);
        connect(action, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getExistingDirectory(
                this, "select alist config dir", ui->lineEditAlistConfigPath->text());
            if (!path.isEmpty()) {
                ui->lineEditAlistConfigPath->setText(path);
            }
        });
        m_actions.insert("ChooseAlistConfigPath", action);
    }
    if (!m_actions.contains("ExplorerAlistExecPath")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonExplorerAlistExecDir->text());
        ui->toolButtonExplorerAlistExecDir->setDefaultAction(action);
        connect(action, &QAction::triggered, this, [this]() {
            //  "explorer /select,D:\tmp\"
            QString path =
                QDir::toNativeSeparators(ui->lineEditAlistExecPath->text());
            QProcess::startDetached("explorer", QStringList() << "/select," + path);
        });
        m_actions.insert("ExplorerAlistExecPath", action);
    }
    if (!m_actions.contains("ExplorerAlistConfigPath")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonExplorerAlistConfigPath->text());
        ui->toolButtonExplorerAlistConfigPath->setDefaultAction(action);
        connect(action, &QAction::triggered, this, [this]() {
            QString path =
                QDir::toNativeSeparators(ui->lineEditAlistConfigPath->text());
            QProcess::startDetached("explorer", QStringList() << "/select," + path);
        });
        m_actions.insert("ExplorerAlistConfigPath", action);
    }
    if (!m_actions.contains("StartAlistServer")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonStartAlistServer->text());
        ui->toolButtonStartAlistServer->setDefaultAction(action);
        connect(action, &QAction::triggered, this, &QNetMount::startAlist);
        m_actions.insert("StartAlistServer", action);
    }
    if (!m_actions.contains("StopAlistServer")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonStopAlistServer->text());
        ui->toolButtonStopAlistServer->setDefaultAction(action);
        connect(action, &QAction::triggered, this, &QNetMount::stopAlist);
        m_actions.insert("StopAlistServer", action);
    }
    if (!m_actions.contains("AlistSetPassword")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonAlistPassWord->text());
        ui->toolButtonAlistPassWord->setDefaultAction(action);
        connect(action, &QAction::triggered, this, [this]() {
            // alist admin set ${PASSWORD} --data ${DATA_DIIR}
            if (!checkAlistExecuable()) {
                return;
            }
            QProcess process;
            process.setProgram(AlistExecPath());
            process.setArguments({"admin", "set", ui->lineEditAlistPassWord->text(),
                                  "--data", AlistConfigDir()});
            connect(&process, &QProcess::readyReadStandardError, this,
                    [&process, this]() {
                QString str = process.readAllStandardError();
                QMessageBox::information(this, "set password info", str);
            });
            process.start();
            process.waitForFinished();
        });
        m_actions.insert("AlistSetPassword", action);
    }
    if (!m_actions.contains("IndexToStoreManaget")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexStroageManager->text());
        ui->toolButtonIndexStroageManager->setDefaultAction(action);
        connect(action, &QAction::triggered, this,
                [this]() { ui->tabWidgetCentral->setCurrentIndex(1); });
        m_actions.insert("IndexToStoreManaget", action);
    }
    if (!m_actions.contains("IndexToMountManaget")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexMountManager->text());
        ui->toolButtonIndexMountManager->setDefaultAction(action);
        connect(action, &QAction::triggered, this,
                [this]() { ui->tabWidgetCentral->setCurrentIndex(2); });
        m_actions.insert("IndexToMountManaget", action);
    }
    if (!m_actions.contains("IndexToTaskManaget")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexTaskManager->text());
        ui->toolButtonIndexTaskManager->setDefaultAction(action);
        connect(action, &QAction::triggered, this,
                [this]() { ui->tabWidgetCentral->setCurrentIndex(4); });
        m_actions.insert("IndexToTaskManaget", action);
    }
    if (!m_actions.contains("IndexToTransManaget")) {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexTransDetail->text());
        ui->toolButtonIndexTransDetail->setDefaultAction(action);
        connect(action, &QAction::triggered, this,
                [this]() { ui->tabWidgetCentral->setCurrentIndex(3); });
        m_actions.insert("IndexToTransManaget", action);
    }
}

void QNetMount::initTrayIcon() {
    // new
    if (!m_trayIcon) {
        m_trayIcon = new QSystemTrayIcon;
    } else {
        return;
    }
    // style
#ifdef _DEBUG
    m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_TitleBarMenuButton));
#else
    m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
#endif // _DEBUG
    m_trayIcon->setObjectName(QApplication::applicationName());
    m_trayIcon->setToolTip(QApplication::applicationName());
    // menu
    QMenu *trayMenu = new QMenu;
    trayMenu->addAction(m_actions.value("AboutQNetMount"));
    trayMenu->addAction(m_actions.value("AboutNetMount"));
    trayMenu->addAction(m_actions.value("AboutAlist"));
    trayMenu->addAction(m_actions.value("AboutRcClone"));
    // 只显示托盘图标时，再显示 "AboutQt" ，点击弹窗的 "OK" 按钮会导致程序退出
    // trayMenu->addAction(m_actions.value("AboutQt"));
    trayMenu->addAction(m_actions.value("Show"));
    trayMenu->addSeparator();
    trayMenu->addAction(m_actions.value("Exit"));
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->show();
    m_trayIcon->setToolTip(QApplication::applicationName() + "(" +
                           QApplication::applicationFilePath() + ")");
    // connect
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                switch (reason) {
        case QSystemTrayIcon::Context:
                    m_trayIcon->contextMenu()->popup(QCursor::pos());
            break;
        case QSystemTrayIcon::DoubleClick:
            m_actions.value("Show")->triggered();
            break;
        case QSystemTrayIcon::Trigger:
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:
            break;
        }
    });
}

void QNetMount::closeEvent(QCloseEvent *event) {
    if (m_trayIcon) {
        this->hide();    // 默认动作
        event->ignore(); // 必须调用该语句
        return;          // ignore() 后也不能交给父类处理
    }
    QWidget::closeEvent(event);
}

void QNetMount::initConfigFile(bool rewrite) {
    // rewrite : 是否强制覆盖
    // 不存在或强制写入时进行初始化
    if (!QFileInfo::exists(m_confgPath) || rewrite) {
        QJsonObject jsonRoot;
        jsonRoot["StartHide"] = true;
        jsonRoot["AlistStart"] = true;
        jsonRoot["AlistExec"] =
            QApplication::applicationDirPath() + "/core/alist/alist.exe";
        jsonRoot["AlistConfig"] =
            QApplication::applicationDirPath() + "/core/alist/data";
        QJsonDocument doc;
        doc.setObject(jsonRoot);
        QByteArray byteArray = doc.toJson(QJsonDocument::Indented);
        QFile file(m_confgPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(byteArray);
            file.close();
        }
    }
}

void QNetMount::readConfig() {
    initConfigFile();
    if (QFileInfo::exists(m_confgPath)) {
        QFile file(m_confgPath);
        file.open(QFile::ReadOnly);
        auto jsonStr = file.readAll();
        file.close();
        try {
            auto jsonRoot = QJsonDocument::fromJson(jsonStr);
            auto startHide = jsonRoot["StartHide"];
            auto alistStart = jsonRoot["AlistStart"];
            auto alistExec = jsonRoot["AlistExec"];
            auto alistConfig = jsonRoot["AlistConfig"];
            if (startHide.isBool() && startHide.toBool(false)) {
                ui->checkBoxSettingCtrlStartHide->setChecked(true);
                QTimer::singleShot(0, this, [this]() { this->hide(); });
                // this->hide();  // 没有隐藏
            }
            if (alistExec.isString()) {
                auto exec = alistExec.toString();
                ui->lineEditAlistExecPath->setText(exec);
            }
            if (alistConfig.isString()) {
                auto conf = alistConfig.toString();
                ui->lineEditAlistConfigPath->setText(conf);
            }
            if (alistStart.isBool() && alistStart.toBool(false)) {
                ui->checkBoxSettingCtrlAlist->setChecked(true);
                startAlist();
            } else {
                disableStartAlistServerButtons(false);
            }
        } catch (...) {
            auto sb = QMessageBox::warning(this, "读取配置文件错误！",
                                           "是否初始化设置后退出程序？",
                                           QMessageBox::Yes | QMessageBox::Abort);
            if (sb == QMessageBox::Yes) {
                initConfigFile(true);
            }
            QApplication::exit();
        }
    }
}

void QNetMount::updateToCofigFile() {
    QJsonObject jsonRoot;
    jsonRoot["StartHide"] = ui->checkBoxSettingCtrlStartHide->isChecked();
    jsonRoot["AlistStart"] = ui->checkBoxSettingCtrlAlist->isChecked();
    jsonRoot["AlistExec"] = ui->lineEditAlistExecPath->text();
    jsonRoot["AlistConfig"] = ui->lineEditAlistConfigPath->text();
    QJsonDocument doc;
    doc.setObject(jsonRoot);
    QByteArray byteArray = doc.toJson(QJsonDocument::Indented);
    QFile file(m_confgPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(byteArray);
        file.close();
    }
}

void QNetMount::resetAlistConfigDir() {
    // 配置文件中有几个固定的目录地址，需要更新为当前数据目录，
    // 否则会导致从 NetMount
    // 复制过来的配置依然使用原先的数据库，导致管理员密码被重置 每次调用 alist
    // 命令前都需要进行检查

    QString configPath;
    QString bleveDir, tempDir, dbFile, logName;     // path should be
    QString bleve_dir, temp_dir, db_file, log_name; // path form json
    configPath = QDir::toNativeSeparators(AlistConfigDir() + "/config.json");
    bleveDir = QDir::toNativeSeparators(AlistConfigDir() + "/bleve");
    tempDir = QDir::toNativeSeparators(AlistConfigDir() + "/temp");
    dbFile = QDir::toNativeSeparators(AlistConfigDir() + "/data.db");
    logName = QDir::toNativeSeparators(AlistConfigDir() + "/log/log.log");
    if (!QFileInfo::exists(configPath)) {
        // 没有配置文件，不必更新
        return;
    }

    // read alist config json
    QByteArray byteArrayRead;
    {
        QFile file(configPath);
        file.open(QFile::ReadOnly);
        byteArrayRead = file.readAll();
        file.close();
    }
    // parse json
    QJsonDocument jsonRoot = QJsonDocument::fromJson(byteArrayRead);
    QJsonObject jsonObject = jsonRoot.object();
    bleve_dir = jsonRoot["bleve_dir"].toString();
    temp_dir = jsonRoot["temp_dir"].toString();
    db_file = jsonRoot["database"]["db_file"].toString();
    log_name = jsonRoot["log"]["name"].toString();
    m_alist_address = jsonObject["scheme"].toObject()["address"].toString();
    m_alist_httpPort = jsonObject["scheme"].toObject()["http_port"].toInt();

    if (bleveDir == bleve_dir && tempDir == temp_dir && dbFile == db_file &&
        logName == log_name) {
        // 路径相同，不必更新
        return;
    }

    // 路径不同，需要更新
    jsonObject["bleve_dir"] = bleveDir;
    jsonObject["temp_dir"] = tempDir;
    auto database = jsonObject["database"].toObject(); // 注意层级
    database["db_file"] = dbFile;
    jsonObject["database"] = database;             // 设置回去
    auto log = jsonObject["log"].toObject(); // 注意层级
    log["name"] = logName;
    jsonObject["log"] = log; // 设置回去
    QJsonDocument doc(jsonObject);
    QByteArray byteArrayWrite = doc.toJson();
    {
        QFile file(configPath); // 原文覆盖
        file.open(QFile::WriteOnly);
        file.write(byteArrayWrite);
        file.close();
    }
}

QString QNetMount::AlistExecPath() { return ui->lineEditAlistExecPath->text(); }

QString QNetMount::AlistConfigDir() {
    return ui->lineEditAlistConfigPath->text();
}

bool QNetMount::checkAlistExecuable() {
    resetAlistConfigDir();
    if (QFileInfo(AlistExecPath()).isExecutable()) {
        return true;
    }
    QMessageBox::warning(
        this, "错误提示",
        QString("alist 可执行程序错误！\n路径：%1").arg(AlistExecPath()));
    return false;
}

void QNetMount::disableStartAlistServerButtons(bool disable) {
    ui->toolButtonStartAlistServer->setDisabled(disable);
    ui->lineEditAlistExecPath->setReadOnly(disable);
    ui->lineEditAlistConfigPath->setReadOnly(disable);
    ui->toolButtonChooseAlistExecPath->setDisabled(disable);
    ui->toolButtonChooseAlistConfigPath->setDisabled(disable);
    ui->toolButtonStopAlistServer->setDisabled(!disable);
}

void QNetMount::startAlist() {
    if (!checkAlistExecuable() || m_processAlist) {
        return;
    }
    disableStartAlistServerButtons();
    // 1. `alist start` 启动服务可使用 `alist stop` 停止服务
    // 2. `alist server` 启动时输出信息较全，但无法直接使用 `alist stop`
    // 停止服务，
    //    需要在 `data\daemon\pid` 文件中保存服务进程的PID
    // 因此建议使用 `alist start` 启动服务
    m_processAlist = new QProcess;
    m_processAlist->setProgram(AlistExecPath());
    m_processAlist->setArguments({"start", "--data", AlistConfigDir()});
    m_processAlist->startDetached(&m_alist_pid);
    if (m_processAlist->exitCode() && m_alist_pid <= 0) {
        disableStartAlistServerButtons(false);
    }
}

void QNetMount::stopAlist() {
    if (!m_processAlist) {
        return;
    }
    if (!checkAlistExecuable() || !m_processAlist) {
        return;
    }
    disableStartAlistServerButtons(false);
    QProcess process;
    process.setProgram(AlistExecPath());
    process.setArguments({"stop", "--data", AlistConfigDir()});
    process.start();
    process.waitForFinished();

    delete m_processAlist;
    m_processAlist = nullptr;
}

QString QNetMount::AlistToken() {
    if (!checkAlistExecuable()) {
        return QString();
    }
    // 输出到错误显示的内容为彩色，且可能为一行或多行
    QStringList tokens;
    QProcess process;
    process.setProgram(AlistExecPath());
    process.setArguments({"admin", "token"});
    connect(&process, &QProcess::readyReadStandardError, this,
            [&process, &tokens]() {
        QString str = process.readAllStandardError();
        QRegExp regExp("Admin token: (alist-\\S+)");
        int pos = 0;
        while ((pos = regExp.indexIn(str, pos)) != -1) {
            tokens << regExp.cap(1);
            pos += regExp.matchedLength();
        }
    });
    process.start();
    process.waitForFinished();
    if (process.exitCode()) {
        ;
    }
    if (QProcess::NormalExit != process.exitStatus()) {
        ;
    }
    return tokens.at(0);
}
