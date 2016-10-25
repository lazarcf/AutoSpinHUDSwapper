#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMutex>
#include <QMenu>
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
#include <QSettings>
#include "utils.h"

namespace Ui {
class MainWindow;
}

class switch_worker_t : public QObject {
    Q_OBJECT

public:
    switch_worker_t(HWND parent_window, int hud_index):
        parent_window(parent_window),
        hud_index(hud_index)
    {}
    //~switch_worker_t();

public slots:
    void work();
    void switch_hud();

signals:
    void finished();
    void remove_pending(HWND window);
    void log(QString);

private:
    HWND parent_window;
    int hud_index;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void changeEvent (QEvent *);

public slots:
    void log(QString);

private slots:
    void on_pushButton_clicked();
    void fs_check();
    //void directory_changed(const QString &path);
    //void file_changed(const QString &path);

    void set_hh_path_slot();
    void set_hero_name_slot();
    void set_hud_index_slot();
    void exit_slot();
    void show_hide_window();

    void tray_activated_slot(QSystemTrayIcon::ActivationReason);

    void on_hero_name_edit_editingFinished();

    void on_hud_index_edit_editingFinished();

    void on_hh_path_button_clicked();

    void switch_pending();

    void remove_pending_switch(HWND);

private:
    void switch_hud(HWND parent_window);
    //bool exact_icon_search(QPixmap src, QPixmap icon, int x, int y);
    //void region_icon_search(QPixmap src, QPixmap icon, int in_x, int in_y, int in_w, int in_h, int & out_x, int & out_y);
    //void send_click (int x, int y, HWND window = NULL, bool translate=true);
    void test_switch();

    //DWORD get_pid(QString proc_name);
    //QVector<cWindow> get_windows (DWORD hud_pid);

    QMap<HWND, QThread*> m_pending_switches;

    bool search_hh_file (QString path);

    Ui::MainWindow *ui;

    QSettings m_settings;

    QSystemTrayIcon m_systray;
    QMenu m_systray_menu;

    QStringList m_old_files;
    QString m_username;

    QTimer *m_fs_timer;
    QPoint m_icon_pos;

    QString m_hh_path;
    QMutex m_pending_switches_mutes;
};

#endif // MAINWINDOW_H
