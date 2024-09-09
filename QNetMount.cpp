#include "QNetMount.h"
#include "ui_QNetMount.h"

QNetMount::QNetMount(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::QNetMount),
      m_confgPath(QApplication::applicationFilePath() + ".json")
{
    ui->setupUi(this);
    // 最先开始需要初始化的资源 : 字符串、QAction
    initStrings(); // 1.初始化字符串资源
    initAcitons(); // 2.初始化设置 QAction 及绑定
    // 其他设置
    loadConfig(); // 解析 json 配置并应用
    //
    ui->toolButtonSettingCtrlSave->hide(); // 已经自动保存，这里暂时用不到
    ui->tabWidgetCentral->setCurrentWidget(ui->Index);
    ui->tabWidgetStore->setCurrentWidget(ui->Manager);
    emit ui->tabWidgetCentral->currentChanged(0);                 // 用户触发自动调整窗口大小
    setWindowIcon(style()->standardIcon(QStyle::SP_DesktopIcon)); // 添加程序图标
    initTrayIcon();                                               // 添加托盘图标
    initStatusBar();                                              // 状态栏添加 Alist 运行指示器
    updateSettingLibsTitleInfo();                                 // 更新关于信息中 alist 版本号
    checkAlistStatus();                                           // 更新 alist 运行指示器

    // realtime update config file
    connect(ui->checkBoxSettingCtrlStartHide, &QCheckBox::checkStateChanged, this, &QNetMount::updateToCofigFile);
    connect(ui->checkBoxSettingCtrlAlist, &QCheckBox::checkStateChanged, this, &QNetMount::updateToCofigFile);
    connect(ui->lineEditAlistExecPath, &QLineEdit::textChanged, this, &QNetMount::updateToCofigFile);
    connect(ui->lineEditAlistConfigPath, &QLineEdit::textChanged, this, &QNetMount::updateToCofigFile);
}

QNetMount::~QNetMount()
{
    stopAlistServer();
    delete ui;
    while (actionsMap().size())
    {
        auto ite = actionsMap().begin();
        delete ite.value();
        actionsMap().remove(ite.key());
    }
}

QString QNetMount::parentDir(const QString &path, bool *ok)
{
    // 获取父路径  // 不完美，但先凑合用吧
    // 1. 去除尾部目录标志  // rfind not \/
    QRegularExpression reSuffix(R"([\|\\|/]*$)");
    auto matchSuffix = reSuffix.match(path);
    QString pathSuffix = matchSuffix.hasMatch() ? matchSuffix.captured(0) : QString();
    QString pathNoSuffix = path.left(path.length() - pathSuffix.length());
    // 2. 截取
    QRegularExpression regularExpression(R"(^.*[\|\\|/])");
    auto matchPath = regularExpression.match(pathNoSuffix);
    QString dir = matchPath.hasMatch() ? matchPath.captured(0) : QString();
    if (dir.isEmpty())
    {
        // 未匹配到时返回传入的路径，防止空内容
        dir = path;
    }
    if (ok)
    {
        *ok = matchPath.hasMatch();
    }
    return dir;
}
QString QNetMount::configPath() { return m_confgPath; }
QString QNetMount::alistExecPath() { return ui->lineEditAlistExecPath->text(); }
QString QNetMount::alistConfigDir() { return ui->lineEditAlistConfigPath->text(); }
QString QNetMount::alistPidPath() { return QDir::toNativeSeparators(parentDir(alistExecPath()) + "daemon/pid"); }

void QNetMount::setString(const QString &key, const QString &value) { stringsMap()[key] = value; }
void QNetMount::setAction(const QString &key, QAction *value) { actionsMap()[key] = value; }
bool QNetMount::hasString(const QString &key) { return stringsMap().contains(key); }
bool QNetMount::hasAction(const QString &key) { return actionsMap().contains(key); }
QMap<QString, QString> &QNetMount::stringsMap() { return m_strings; }
QMap<QString, QAction *> &QNetMount::actionsMap() { return m_actions; }

QString QNetMount::getString(const QString &key)
{
    // 确保字符串资源已经初始化/确保取的到
    // 1. 未初始化则进行初始化  // 默认需要添加空值
    // 2. 找不到返回空值的字符串
    // 3. 返回找到的字符串
    if (stringsMap().empty())
    {
        initStrings();
    }
    auto ite = stringsMap().find(key);
    if (ite == stringsMap().end())
    {
        return stringsMap()[""];
    }
    return ite.value();
}

QAction *QNetMount::getAction(const QString &key)
{
    // 确保 QAction 资源已经初始化/确保取的到
    // 1. 未初始化则进行初始化 // 默认需要添加空值
    // 2. 找不到返回空值的 QAction
    // 3. 返回找到的 QAction
    if (actionsMap().empty())
    {
        initAcitons();
    }
    auto ite = actionsMap().find(key);
    if (ite == actionsMap().end())
    {
        return actionsMap()[""];
    }
    return ite.value();
}

void QNetMount::initStrings()
{
    // 防止重复初始化导致内存泄露
    if (stringsMap().size())
    {
        return;
    }
    // 防止找不到
    stringsMap()[""] = "";
    // 正常初始化
    stringsMap()["about_QNetMount_title"] = "关于 QNetMount";
    stringsMap()["about_QNetMount_info"] =
        "<a href='https://github.com/guoiaoang/QNetMount'>QNetMount</a>"
        "<br>模仿 NetMount 做的 GUI 界面，目前仅作为 Alist 的启动器。"
        "<br>支持托盘图标；"
        "<br>支持从当前程序修改 AList 管理员密码。"
        "<br>支持自定义 Alist 路径"
        "<br>更新时间 : 2024-09-10 01:03";
    stringsMap()["about_NetMount_title"] = "关于 NetMount";
    stringsMap()["about_NetMount_info"] =
        "<a style='color: lightblue;' href='https://www.netmount.cn'>NetMount</a>"
        "<br>统一管理和挂载云存储设施"
        "<br>GitHub : <a style='color: lightblue;' href='https://github.com/VirtualHotBar/NetMount/'>NetMount</a>";
    stringsMap()["about_Alist_title"] = "关于 Alist";
    stringsMap()["about_Alist_info"] =
        "<a style='color: lightblue;' href='https://alist.nn.ci/'>AList</a>"
        "<br>一个支持多存储的文件列表/WebDAV程序"
        "<br>GitHub : <a style='color: lightblue;' href='https://github.com/alist-org/alist'>AList</a>";
    stringsMap()["about_RcClone_title"] = "关于 RcClone";
    stringsMap()["about_RcClone_info"] =
        "<a style='color: lightblue;' href='https://rclone.org/'>RcClone</a>"
        "<br>rsync for cloud storage"
        "<br>GitHub : <a style='color: lightblue;' href='https://github.com/rclone/rclone'>RcClone</a>";
    stringsMap()["about_Qt_title"] = "关于 Qt";
    stringsMap()["show"] = "显示";
    stringsMap()["hide"] = "隐藏";
    stringsMap()["exit"] = "退出";
    stringsMap()["select_alist_exec_path"] = R"(选择 "alist.exe" 所在位置)";
    stringsMap()["select_alist_config_dir"] = R"(选择 "alist.exe" 所在位置)";
    stringsMap()["set_alist_admin_password"] = "设置 Alist 管理员密码";
    stringsMap()["alist_not_run"] = "alist 服务未启动";
    stringsMap()["alist_has_run"] = "alist 服务已启动";
    stringsMap()["read_alist_config_error"] = "读取配置文件错误！";
    stringsMap()["init_alist_config_then_exit"] = "是否初始化设置后退出程序？";
    stringsMap()["check_alist_exec_error_title"] = "Alist 可执行程序错误";
    stringsMap()["check_alist_exec_error_info"] = "路径：%1";

    stringsMap()["html_body_begin"] = "<html><body>";
    stringsMap()["html_body_end"] = "</body></html>";
    stringsMap()["rclone_github"] = "https://github.com/rclone/rclone";
    stringsMap()["alist_github"] = "https://github.com/alist-org/alist";
    stringsMap()["rclone_version"] = "v1.66.0";
    stringsMap()["alist_version"] = "v3.35.0";
    stringsMap()["rclone_info"] = "<a href='${rclone_github}'>Rclone</a>(<a href='www'>日志</a>):${rclone_version}";
    stringsMap()["alist_info"] = "<a href='${alist_github}'>Alist</a>(<a href='www'>日志</a>):${alist_version}";
}

void QNetMount::initAcitons()
{
    // 防止重复初始化导致内存泄露
    if (m_actions.size())
    {
        return;
    }
    // 防止找不到
    setAction("", new QAction);
    // 正常初始化
    if (!hasAction("AboutQNetMount"))
    {
        QAction *action = new QAction;
        action->setText(getString("about_QNetMount_title"));
        connect(action, &QAction::triggered, this, std::bind(&QMessageBox::about, this, getString("about_QNetMount_title"), getString("about_QNetMount_info")));
        setAction("AboutQNetMount", action);
    }
    if (!hasAction("AboutNetMount"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("about_NetMount_title"));
        connect(action, &QAction::triggered, this, std::bind(&QMessageBox::about, this, getString("about_NetMount_title"), getString("about_NetMount_info")));
        setAction("AboutNetMount", action);
    }
    if (!hasAction("AboutAlist"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("about_Alist_title"));
        connect(action, &QAction::triggered, this, std::bind(&QMessageBox::about, this, getString("about_Alist_title"), getString("about_Alist_info")));
        setAction("AboutAlist", action);
    }
    if (!hasAction("AboutRcClone"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("about_RcClone_title"));
        connect(action, &QAction::triggered, this, std::bind(&QMessageBox::about, this, getString("about_RcClone_title"), getString("about_RcClone_info")));
        setAction("AboutRcClone", action);
    }
    if (!hasAction("AboutQt"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("about_Qt_title"));
        connect(action, &QAction::triggered, this, &QApplication::aboutQt);
        setAction("AboutQt", action);
    }
    if (!hasAction("Show"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("hide"));
        auto func = [this, action]()
        {
            if (this->isHidden())
            {
                this->show();
                action->setText(getString("hide"));
                this->activateWindow();
            }
            else if (this->isVisible())
            {
                this->hide();
                action->setText(getString("show"));
            }
        };
        connect(action, &QAction::triggered, this, func);
        setAction("Show", action);
    }
    if (!hasAction("Exit"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("exit"));
        connect(action, &QAction::triggered, this, &QApplication::exit);
        setAction("Exit", action);
    }
    // QToolbutton
    if (!hasAction("ChooseAlistExecPath"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonChooseAlistExecPath->text());
        ui->toolButtonChooseAlistExecPath->setDefaultAction(action);
        auto func = [this]()
        {
            QString path = QFileDialog::getOpenFileName(this, getString("select_alist_exec_path"), alistExecPath(), "*.exe");
            if (!path.isEmpty())
            {
                ui->lineEditAlistExecPath->setText(path);
            }
        };
        connect(action, &QAction::triggered, this, func);
        setAction("ChooseAlistExecPath", action);
    }
    if (!hasAction("ChooseAlistConfigPath"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonChooseAlistConfigPath->text());
        ui->toolButtonChooseAlistConfigPath->setDefaultAction(action);
        auto func = [this]()
        {
            QString path = QFileDialog::getExistingDirectory(this, getString("select_alist_config_dir"), alistConfigDir());
            if (!path.isEmpty())
            {
                ui->lineEditAlistConfigPath->setText(path);
            }
        };
        connect(action, &QAction::triggered, this, func);
        setAction("ChooseAlistConfigPath", action);
    }
    if (!hasAction("ExplorerAlistExecPath"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonExplorerAlistExecDir->text());
        ui->toolButtonExplorerAlistExecDir->setDefaultAction(action);
        auto func = [this]()
        {
            QString locate = QDir::toNativeSeparators(ui->lineEditAlistExecPath->text());
            if (!checkAlistExecuable())
            {
                locate = parentDir(locate); // 父目录
                // 无目录时先创建目录
                if (QFile::exists(locate))
                {
                    QDir().mkpath(parentDir(locate));
                }
            }
            //  "explorer /select,D:\tmp\"
            QProcess::startDetached("explorer", QStringList() << "/select," + locate);
        };
        connect(action, &QAction::triggered, this, func);
        setAction("ExplorerAlistExecPath", action);
    }
    if (!hasAction("ExplorerAlistConfigPath"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonExplorerAlistConfigPath->text());
        ui->toolButtonExplorerAlistConfigPath->setDefaultAction(action);
        auto func = [this]()
        {
            QString dir = QDir::toNativeSeparators(ui->lineEditAlistConfigPath->text());
            // 无目录时先创建目录
            if (!QFileInfo(dir).exists())
            {
                QDir().mkpath(dir);
            }
            QProcess::startDetached("explorer", QStringList() << "/select," + dir);
        };
        connect(action, &QAction::triggered, this, func);
        setAction("ExplorerAlistConfigPath", action);
    }
    if (!hasAction("StartAlistServer"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonStartAlistServer->text());
        ui->toolButtonStartAlistServer->setDefaultAction(action);
        connect(action, &QAction::triggered, this, &QNetMount::startAlistServer);
        m_actions.insert("StartAlistServer", action);
    }
    if (!hasAction("StopAlistServer"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonStopAlistServer->text());
        ui->toolButtonStopAlistServer->setDefaultAction(action);
        connect(action, &QAction::triggered, this, &QNetMount::stopAlistServer);
        setAction("StopAlistServer", action);
    }
    if (!hasAction("AlistSetPassword"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonAlistPassWord->text());
        ui->toolButtonAlistPassWord->setDefaultAction(action);
        auto func = [this]()
        {
            // alist admin set ${PASSWORD} --data ${DATA_DIIR}
            if (!checkAlistExecuable())
            {
                return;
            }
            QProcess process;
            process.setProgram(alistExecPath());
            process.setArguments({"admin", "set", ui->lineEditAlistPassWord->text(), "--data", alistConfigDir()});
            auto func = [&process, this]()
            {
                QMessageBox::information(this, "set password info", process.readAllStandardError());
            };
            connect(&process, &QProcess::readyReadStandardError, this, func);
            process.start();
            process.waitForFinished();
        };
        connect(action, &QAction::triggered, this, func);
        setAction("AlistSetPassword", action);
    }
    if (!hasAction("IndexToStoreManaget"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexStroageManager->text());
        ui->toolButtonIndexStroageManager->setDefaultAction(action);
        connect(action, &QAction::triggered, this, std::bind(&QTabWidget::setCurrentIndex, ui->tabWidgetCentral, 1));
        setAction("IndexToStoreManaget", action);
    }
    if (!hasAction("IndexToMountManaget"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexMountManager->text());
        ui->toolButtonIndexMountManager->setDefaultAction(action);
        connect(action, &QAction::triggered, std::bind(&QTabWidget::setCurrentIndex, ui->tabWidgetCentral, 2));
        setAction("IndexToMountManaget", action);
    }
    if (!hasAction("IndexToTransManaget"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexTransDetail->text());
        ui->toolButtonIndexTransDetail->setDefaultAction(action);
        connect(action, &QAction::triggered, this, std::bind(&QTabWidget::setCurrentIndex, ui->tabWidgetCentral, 3));
        setAction("IndexToTransManaget", action);
    }
    if (!hasAction("IndexToTaskManaget"))
    {
        QAction *action = new QAction(this);
        action->setText(ui->toolButtonIndexTaskManager->text());
        ui->toolButtonIndexTaskManager->setDefaultAction(action);
        connect(action, &QAction::triggered, this, std::bind(&QTabWidget::setCurrentIndex, ui->tabWidgetCentral, 4));
        setAction("IndexToTaskManaget", action);
    }
    if (!hasAction("ShowAlistHost"))
    {
        QAction *action = new QAction(this);
        action->setText(getString("alist_not_run"));
        connect(action, &QAction::triggered, this, &QNetMount::showAlistHost);
        setAction("ShowAlistHost", action);
    }
}

void QNetMount::initTrayIcon()
{
    if (m_trayIcon)
    {
        return;
    }
    m_trayIcon = new QSystemTrayIcon;
    // style
    m_trayIcon->setIcon(windowIcon());
    m_trayIcon->setObjectName(QApplication::applicationName());
    m_trayIcon->setToolTip(QApplication::applicationName());
    // menu
    QMenu *trayMenu = new QMenu(this);
    trayMenu->addAction(getAction("AboutQNetMount"));
    trayMenu->addAction(getAction("AboutNetMount"));
    trayMenu->addAction(getAction("AboutAlist"));
    trayMenu->addAction(getAction("AboutRcClone"));
    // 只显示托盘图标时，再显示 "AboutQt" ，点击弹窗的 "OK" 按钮会导致程序退出
    // trayMenu->addAction(getAction("AboutQt"));
    trayMenu->addAction(getAction("Show"));
    trayMenu->addSeparator();
    trayMenu->addAction(getAction("Exit"));
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->setToolTip(QApplication::applicationName() + "(" + QApplication::applicationFilePath() + ")");
    m_trayIcon->show();
    // connect
    auto func = [this](QSystemTrayIcon::ActivationReason reason)
    {
        switch (reason)
        {
        case QSystemTrayIcon::Context:
            m_trayIcon->contextMenu()->popup(QCursor::pos());
            break;
        case QSystemTrayIcon::DoubleClick:
            getAction("Show")->triggered();
            break;
        case QSystemTrayIcon::Trigger:
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:
            break;
        }
    };
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, func);
}

void QNetMount::initStatusBar()
{
    if (m_alist_status)
    {
        return;
    }
    m_alist_status = new QToolButton;
    m_alist_status->setDefaultAction(getAction("ShowAlistHost"));
    ui->statusbar->addPermanentWidget(m_alist_status);
}

void QNetMount::updateSettingLibsTitleInfo()
{
    stringsMap()["alist_version"] = alistVersion();
    QStringList infos;
    infos << getString("html_body_begin")
          << getString("rclone_info")
                 .replace("${rclone_github}", getString("rclone_github"))
                 .replace("${rclone_version}", getString("rclone_version"))
          << "<br>"
          << getString("alist_info")
                 .replace("${alist_github}", getString("alist_github"))
                 .replace("${alist_version}", getString("alist_version"))
          << getString("html_body_end");
    ui->labelSettingLibsTitleInfo->setText(infos.join(""));
}

void QNetMount::checkAlistStatus()
{
    // 更新界面 alist 服务运行状态
    // m_processAlist 由监控线程进行维护
    bool running = bool(m_processAlist);
    getAction("ShowAlistHost")->setText(getString(running ? "alist_has_run" : "alist_not_run"));
    ui->toolButtonStartAlistServer->setDisabled(running);
    ui->lineEditAlistExecPath->setReadOnly(running);
    ui->lineEditAlistConfigPath->setReadOnly(running);
    ui->toolButtonChooseAlistExecPath->setDisabled(running);
    ui->toolButtonChooseAlistConfigPath->setDisabled(running);
    ui->toolButtonStopAlistServer->setDisabled(!running);
    // 写进程ID到文件
    QString alistPidFile = alistPidPath();
    if (checkAlistExecuable() && running)
    {
        // 先创建目录再创建文件才会成功
        auto pidDir = parentDir(alistPidFile);
        if (!QFileInfo::exists(pidDir))
        {
            QDir().mkpath(pidDir);
        }
        QFile file(alistPidFile);
        if (file.open(QFile::ReadWrite))
        {
            file.write(QString::number(m_processAlist->processId()).toStdString().data());
            file.close();
        }
    }
    else if (QFileInfo::exists(alistPidFile))
    {
        // 即使不在这里手动删除，Alist 服务也会自动删除该文件
        QFile::remove(alistPidFile);
    }
}

void QNetMount::on_tabWidgetCentral_currentChanged(int index)
{
    Q_UNUSED(index);
    if (this->isMaximized() || this->isFullScreen() || this->isMinimized())
    {
        return;
    }
    // const int minSize[7][2] = {
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 281 },
    //     { 366, 543 },
    //     { 366, 281 },
    // };
    // int w = minSize[index][0];
    // int h = minSize[index][1];
    bool spec = ui->tabWidgetCentral->currentWidget() == ui->Setting;
    int w = 366;
    int h = spec ? 543 : 281;
    this->setMinimumSize(w, h);
    this->resize(w, h);
}

void QNetMount::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon)
    {
        this->hide();    // 默认动作
        event->ignore(); // 必须调用该语句
        return;          // ignore() 后也不能交给父类处理
    }
    QWidget::closeEvent(event);
}

void QNetMount::initConfigFile(bool rewrite)
{
    // rewrite : 是否强制覆盖
    // 不存在或强制写入时进行初始化
    if (!QFileInfo::exists(configPath()) || rewrite)
    {
        QJsonObject jsonRoot;
        jsonRoot["StartHide"] = true;
        jsonRoot["AlistStart"] = true;
        jsonRoot["AlistExec"] = QApplication::applicationDirPath() + "/core/alist/alist.exe";
        jsonRoot["AlistConfig"] = QApplication::applicationDirPath() + "/core/alist/data";
        QJsonDocument doc;
        doc.setObject(jsonRoot);
        QByteArray byteArray = doc.toJson(QJsonDocument::Indented);
        QFile file(m_confgPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.write(byteArray);
            file.close();
        }
    }
}

void QNetMount::loadConfig()
{
    initConfigFile();
    if (QFileInfo::exists(configPath()))
    {
        QFile file(m_confgPath);
        file.open(QFile::ReadOnly);
        auto jsonStr = file.readAll();
        file.close();
        try
        {
            auto jsonRoot = QJsonDocument::fromJson(jsonStr);
            auto alistExec = jsonRoot["AlistExec"].toString();
            auto alistConfig = jsonRoot["AlistConfig"].toString();
            auto startHide = jsonRoot["StartHide"].toBool(false);
            auto alistStart = jsonRoot["AlistStart"].toBool(false);
            ui->lineEditAlistExecPath->setText(alistExec);
            ui->lineEditAlistConfigPath->setText(alistConfig);
            ui->checkBoxSettingCtrlStartHide->setChecked(startHide);
            ui->checkBoxSettingCtrlAlist->setChecked(alistStart);
            if (alistStart)
            {
                // this->hide();  // 直接调用可能会没有隐藏
                QTimer::singleShot(0, this, &QNetMount::hide);
            }
            if (alistStart)
            {
                startAlistServer();
            }
        }
        catch (...)
        {
            auto sb = QMessageBox::warning(this, getString("read_alist_config_error"), getString("init_alist_config_then_exit"), QMessageBox::Yes | QMessageBox::Abort);
            if (sb == QMessageBox::Yes)
            {
                initConfigFile(true);
            }
            QApplication::exit();
        }
    }
}

void QNetMount::updateToCofigFile()
{
    QJsonObject jsonRoot;
    jsonRoot["StartHide"] = ui->checkBoxSettingCtrlStartHide->isChecked();
    jsonRoot["AlistStart"] = ui->checkBoxSettingCtrlAlist->isChecked();
    jsonRoot["AlistExec"] = ui->lineEditAlistExecPath->text();
    jsonRoot["AlistConfig"] = ui->lineEditAlistConfigPath->text();
    QJsonDocument doc;
    doc.setObject(jsonRoot);
    QByteArray byteArray = doc.toJson(QJsonDocument::Indented);
    QFile file(m_confgPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.write(byteArray);
        file.close();
    }
}

void QNetMount::syncAlistConfigDir()
{
    // **每次调用 alist 命令前都需要进行检查**
    // 配置文件中有几个固定的目录地址，需要更新为当前数据目录，否则会导致从 NetMount 复制过来的配置依然使用原先的数据库，导致管理员密码被重置
    QString alistConfigPath;
    alistConfigPath = QDir::toNativeSeparators(alistConfigDir() + "/config.json");
    if (!QFileInfo::exists(alistConfigPath))
    {
        // 没有配置文件，不必更新
        return;
    }

    QString alistBleveDir, alistTempDir, alistDbFile, alistLogName;         // path should be
    QString alist_bleve_dir, alist_temp_dir, alist_db_file, alist_log_name; // path form json
    alistBleveDir = QDir::toNativeSeparators(alistConfigDir() + "/bleve");
    alistTempDir = QDir::toNativeSeparators(alistConfigDir() + "/temp");
    alistDbFile = QDir::toNativeSeparators(alistConfigDir() + "/data.db");
    alistLogName = QDir::toNativeSeparators(alistConfigDir() + "/log/log.log");

    // read alist config json
    QByteArray byteArrayRead;
    {
        QFile file(alistConfigPath);
        file.open(QFile::ReadOnly);
        byteArrayRead = file.readAll();
        file.close();
    }
    // parse json
    QJsonDocument jsonRoot = QJsonDocument::fromJson(byteArrayRead);
    QJsonObject jsonObject = jsonRoot.object();
    alist_bleve_dir = jsonRoot["bleve_dir"].toString();
    alist_temp_dir = jsonRoot["temp_dir"].toString();
    alist_db_file = jsonRoot["database"]["db_file"].toString();
    alist_log_name = jsonRoot["log"]["name"].toString();
    m_alist_address = jsonObject["scheme"].toObject()["address"].toString();
    m_alist_httpPort = jsonObject["scheme"].toObject()["http_port"].toInt();

    if (alistBleveDir == alist_bleve_dir && alistTempDir == alist_temp_dir && alistDbFile == alist_db_file && alistLogName == alist_log_name)
    {
        // 路径相同，不必更新
        return;
    }

    // 路径不同，需要更新
    jsonObject["bleve_dir"] = alistBleveDir;
    jsonObject["temp_dir"] = alistTempDir;
    auto database = jsonObject["database"].toObject(); // 注意层级
    database["db_file"] = alistDbFile;
    jsonObject["database"] = database;       // 设置回去
    auto log = jsonObject["log"].toObject(); // 注意层级
    log["name"] = alistLogName;
    jsonObject["log"] = log; // 设置回去
    QJsonDocument doc(jsonObject);

    // write alist config json
    QByteArray byteArrayWrite = doc.toJson();
    {
        QFile file(alistConfigPath); // 原文覆盖
        file.open(QFile::WriteOnly);
        file.write(byteArrayWrite);
        file.close();
    }
}

bool QNetMount::checkAlistExecuable()
{
    syncAlistConfigDir();
    if (QFileInfo(alistExecPath()).isExecutable())
    {
        return true;
    }
    // QMessageBox::warning(this, getString("check_alist_exec_error_title"), getString("check_alist_exec_error_info").arg(AlistExecPath()));
    return false;
}

const QVector<QHostAddress> &QNetMount::getIPs()
{
    auto toSortStr = [](const QHostAddress &ip)
    {
        QString str;
        str += ip == QHostAddress::LocalHost ? "Localhost " : "NotLocalhost ";
        if (ip.toIPv4Address())
        {
            str += "IPv4 ";
            auto strs = ip.toString().split(".");
            str += QString("%1.%2.%3.%4")
                       .arg(strs.at(0), 3, '0')
                       .arg(strs.at(1), 3, '0')
                       .arg(strs.at(2), 3, '0')
                       .arg(strs.at(3), 3, '0');
        }
        else
        {
            str += "IPv6 ";
            auto strIP = ip.toString();
            // 补齐后面的网卡名称
            if (!strIP.contains("%"))
            {
                strIP += "%0000_0000";
            }
            // 补齐IP分割符
            auto strsIP = strIP.split("%");
            auto strOnlyIP = strsIP.at(0);
            auto strOnlyName = strsIP.at(1); // scopeId
            int splitCount = strOnlyIP.count(":");
            if (splitCount != 7)
            {
                if (strOnlyIP.contains("::"))
                {
                    strOnlyIP = strOnlyIP.replace("::", QString("%1").arg("::", 2 + 7 - splitCount, ':'));
                }
                else
                {
                    strOnlyIP = ":::::::0000_" + strOnlyIP; // ...
                }
            }
            auto strOnlyIPs = strOnlyIP.split(":");
            str += QString("%1:%2:%3:%4:%5:%6:%7:%8")
                       .arg(strOnlyIPs.at(0), 4, '0')
                       .arg(strOnlyIPs.at(1), 4, '0')
                       .arg(strOnlyIPs.at(2), 4, '0')
                       .arg(strOnlyIPs.at(3), 4, '0')
                       .arg(strOnlyIPs.at(4), 4, '0')
                       .arg(strOnlyIPs.at(5), 4, '0')
                       .arg(strOnlyIPs.at(6), 4, '0')
                       .arg(strOnlyIPs.at(7), 4, '0');
            str += "%" + strOnlyName;
        }
        // 再加上原始的名字
        str += " " + ip.toString();
        return str;
    };
    auto sortIPs = [toSortStr](const QHostAddress &left, const QHostAddress &right)
    {
        return toSortStr(left) <= toSortStr(right);
    };
    static QVector<QHostAddress> vecips;
    // 判断是否已经初始化
    if (vecips.empty())
    {
        const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (auto &item : ipAddressesList)
        {
            vecips.push_back(item);
        }
        std::sort(vecips.begin(), vecips.end(), sortIPs);
    }
    // for (const QHostAddress &entry : vecips) {
    //     qDebug() << QString(entry.toIPv4Address() ? "bind : %1:%2" : "bind : [%1]:%2")
    //                .arg(entry.toString())
    //                .arg(m_server->serverPort());
    // }
    return vecips;
}

void QNetMount::showAlistHost()
{
    static QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setWindowFlag(Qt::Dialog);
    textEdit->resize(380, 240);
    QString port = QString::number(m_alist_httpPort);
    auto alist_pid = m_processAlist ? m_processAlist->processId() : -1;
    QStringList infos;
    infos << "Alist Server " + QString(alist_pid <= 0 ? "not run." : "has run. pid " + QString::number(alist_pid) + ".");
    infos << "    http://localhost:" + port;
    if (m_alist_address == "0.0.0.0")
    {
        for (const QHostAddress &entry : getIPs())
        {
            infos << "    http://" + QString(entry.toIPv4Address() ? "%1:%2" : "[%1]:%2").arg(entry.toString()).arg(port);
        }
    }
    else
    {
        infos << "\thttp://" + m_alist_address + port;
    }
    // ip list / port
    textEdit->setText(infos.join("\n"));
    textEdit->show();
}

void QNetMount::startAlistServer()
{
    ui->toolButtonStartAlistServer->setDisabled(true); // 进入先禁止重复点击，解除禁用状态由其他条件设置
    if (!checkAlistExecuable() || m_processAlist)
    {
        // 1. 程序不可执行时之前属于未启动服务状态，应该允许修改设置并重新进行启动
        // 2. 进程句柄不为空，说明程序在运行中，此时可校验服务状态同步到界面
        checkAlistStatus();
        return;
    }

    // 1. `alist start` 启动后会调用 `alist server` 启动子程序，可使用 `alist stop` 停止子程序
    // 2. `alist server` 启动时输出信息较全，但无法直接使用 `alist stop` 停止服务。停止服务需要在 `alist\daemon\pid` 文件中保存 `alist server` 服务进程的 PID
    // 调用 `alist start` 启动的子程序自动退出，孙子程序依赖于 `conhost.exe`。但当前程序与 `server` 程序与依赖程序非树形结构，管理更加困难，因此不再建议使用 `alist start` 命令
    //
    // alist 启动时，如果端口已经被占用，使用 start 与 server 均不会报错，与端口未占用时输出一致，然后退出，但 `server` 退出可反映实际情况。
    auto monitor = [this]()
    {
        m_processAlist = new QProcess;
        m_processAlist->setProgram(alistExecPath());
        m_processAlist->setArguments({"server", "--data", alistConfigDir(), "--force-bin-dir"});

        auto started = [this]()
        {
            checkAlistStatus();
        };
        auto finished = [this](int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit)
        {
            Q_UNUSED(exitCode);
            Q_UNUSED(exitStatus);
            m_processAlist->deleteLater(); // `Obj::deleteLater()` or `delete obj` ??
            m_processAlist = nullptr;
            checkAlistStatus();
        };
        connect(m_processAlist, &QProcess::started, this, started);
        connect(m_processAlist, &QProcess::finished, this, finished);
        m_processAlist->start();
        m_processAlist->waitForStarted();
        m_processAlist->waitForFinished(-1);
    };
    QThread *thread = QThread::create(monitor);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void QNetMount::stopAlistServer()
{
    ui->toolButtonStopAlistServer->setDisabled(true);
    if (checkAlistExecuable())
    {
        QProcess process;
        process.setProgram(alistExecPath());
        process.setArguments({"stop", "--data", alistConfigDir()});
        process.start();
        process.waitForFinished();
        // 发送进程停止信号后，由监视线程监视alist服务状态并进行状态同步，不要在此处做任何设置
    }
}

QString QNetMount::alistVersion()
{
    if (!checkAlistExecuable())
    {
        return QString();
    }
    // 输出到错误显示的内容为彩色，且可能为一行或多行
    // cmd : `alist version`  // StandardOutput
    QStringList versions;
    QProcess process;
    process.setProgram(alistExecPath());
    process.setArguments({"version"});
    auto func = [&process, &versions]()
    {
        QString str = process.readAllStandardOutput();
        QRegExp regExp("\nVersion: (\\S+)\n");
        int pos = 0;
        while ((pos = regExp.indexIn(str, pos)) != -1)
        {
            versions << regExp.cap(1);
            pos += regExp.matchedLength();
        }
    };
    connect(&process, &QProcess::readyReadStandardOutput, this, func);
    process.start();
    process.waitForFinished();
    if (process.exitCode())
    {
        ;
    }
    if (QProcess::NormalExit != process.exitStatus())
    {
        ;
    }
    return versions.size() ? versions.at(0) : QString();
}

QString QNetMount::alistToken()
{
    if (!checkAlistExecuable())
    {
        return QString();
    }
    // 输出到错误显示的内容为彩色，且可能为一行或多行
    // cmd : `alist admin token`  // StandardError
    QStringList tokens;
    QProcess process;
    process.setProgram(alistExecPath());
    process.setArguments({"admin", "token"});
    auto func = [&process, &tokens]()
    {
        QString str = process.readAllStandardError();
        QRegExp regExp("Admin token: (alist-\\S+)");
        int pos = 0;
        while ((pos = regExp.indexIn(str, pos)) != -1)
        {
            tokens << regExp.cap(1);
            pos += regExp.matchedLength();
        }
    };
    connect(&process, &QProcess::readyReadStandardError, this, func);
    process.start();
    process.waitForFinished();
    if (process.exitCode())
    {
        ;
    }
    if (QProcess::NormalExit != process.exitStatus())
    {
        ;
    }
    return tokens.at(0);
}
