#ifndef QNETMOUNT_H
#define QNETMOUNT_H

#include <QAction>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui {
class QNetMount;
}
QT_END_NAMESPACE

class QNetMount : public QMainWindow {
    Q_OBJECT

public:
    QNetMount(QWidget *parent = nullptr);
    ~QNetMount();

    QString AlistExecPath();
    QString AlistConfigDir();

private:
    void initTrayIcon();
    void initAcitons();
    bool checkAlistExecuable();
    void disableStartAlistServerButtons(bool disable = true);
    void startAlist();
    void stopAlist();
    QString AlistToken();
    void initConfigFile(bool rewrite = false);  // rewrite 强制覆盖
    void readConfig();
    void updateToCofigFile();
    void resetAlistConfigDir();

private slots:
    void on_tabWidgetCentral_currentChanged(int index);

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    Ui::QNetMount *ui;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMap<QString, QAction *> m_actions;
    QProcess *m_processAlist = nullptr;
    QString m_confgPath;
    QString m_alist_address;
    int m_alist_httpPort = -1;
    qint64 m_alist_pid = -1;
};
#endif // QNETMOUNT_H
