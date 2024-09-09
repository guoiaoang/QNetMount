#ifndef QNETMOUNT_H
#define QNETMOUNT_H

#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QProcess>
#include <QRegExp>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class QNetMount;
}
QT_END_NAMESPACE

class QNetMount : public QMainWindow
{
    Q_OBJECT

public:
    QNetMount(QWidget *parent = nullptr);
    ~QNetMount();

    QString configPath();
    QString alistExecPath();
    QString alistConfigDir();
    static QString parentDir(const QString&path, bool *ok = nullptr);

private:
    QString getString(const QString &key);
    QAction *getAction(const QString &key);
    void setString(const QString &key, const QString &value);
    void setAction(const QString &key, QAction *value);
    bool hasString(const QString &key);
    bool hasAction(const QString &key);
    QMap<QString, QString> &stringsMap();
    QMap<QString, QAction *> &actionsMap();
    void initStrings();
    void initTrayIcon();
    void initStatusBar();
    void initAcitons();
    void disableStartAlistServerButtons(bool disable = true);
    void startAlistServer();
    void stopAlistServer();
    void initConfigFile(bool rewrite = false); // rewrite 强制覆盖
    void loadConfig();
    void updateToCofigFile();
    QString alistToken();
    QString alistVersion();
    QString alistPidPath();
    const QVector<QHostAddress> &getIPs();
    bool checkAlistExecuable();
    void syncAlistConfigDir();
    void showAlistHost();
    void updateSettingLibsTitleInfo();

signals:

private slots:
    void on_tabWidgetCentral_currentChanged(int index);
    void checkAlistStatus();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    Ui::QNetMount *ui;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMap<QString, QString> m_strings; // QMap<id, text> 获取文本信息
    QMap<QString, QAction *> m_actions;
    QString m_confgPath;
    // Alist
    QProcess *m_processAlist = nullptr;  // 维护 Alist 运行状态
    QToolButton *m_alist_status = nullptr;
    QString m_alist_address;
    int m_alist_httpPort = -1;
};
#endif // QNETMOUNT_H
