#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QDebug>
#include <Windows.h>
#include <windowsx.h>
#include <QPointer>
#include <Psapi.h>
#include <Qpair>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>
#include <QDir>
#include <QTimer>

#define PROGRAM_NAME "AutoSpinSwap"

namespace Ui {
class MainWindow;
}

struct cWindow{
    HWND hwnd;
    QString class_name;
    QString title;
    QRect rect;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void log(QString);

private slots:
    void on_pushButton_clicked();
    void fs_check();
    //void directory_changed(const QString &path);
    //void file_changed(const QString &path);

private:
    //void test_hook();
    void switch_hud(HWND parent_window);
    bool exact_icon_search(QPixmap src, QPixmap icon, int x, int y);
    void region_icon_search(QPixmap src, QPixmap icon, int in_x, int in_y, int in_w, int in_h, int & out_x, int & out_y);
    void send_click (int x, int y, HWND window = NULL, bool translate=true);
    void test_switch();

    DWORD get_pid(QString proc_name);
    QVector<cWindow> get_windows (DWORD hud_pid);

    bool search_hh_file (QString path);

    QSystemTrayIcon m_systray;

    QStringList m_old_files;
    QString m_username;

    QTimer *m_fs_timer;
    Ui::MainWindow *ui;
    QPoint m_icon_pos;

    QString m_hh_path;
};

#endif // MAINWINDOW_H
