#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QMessageBox>

QString get_appdata_folder();

// Context Menu
// Set Hand History path

// Checker :
// Create DLL which installs itself into the threads of the HUD
// Hook WH_WNDPROC or smth and write all WM_COMMAND timestamps into a file
// file will be read by main app to check if hud was switched

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_icon_pos(-1, -1)
{
    ui->setupUi(this);

    m_fs_timer = new QTimer (this);
    QObject::connect(m_fs_timer, SIGNAL (timeout()), this, SLOT (fs_check()));
    m_fs_timer->start(1000);
}

bool MainWindow::search_hh_file(QString path)
{
    //log ("Searching file " + path);
    QFile file(path);

    if (!file.open(QFile::ReadOnly))
        return false;

    QTextStream stream(&file);
    QString     line;

    bool finished = false;

    while(stream.readLineInto(&line)) {
        if (line.contains("finished the tournament in 3rd place")) {
            QString username = line.left(line.indexOf("finished")-1);

            //log (username + " has left in 3rd place");

            if (username.trimmed().toLower() != m_username.trimmed().toLower()) {
                finished = true;
            }
            else
                m_old_files.append (path);
        }
        if (line.contains("PokerStars Hand") && finished) {
            m_old_files.append(path);
            log ("Adding " + path + " to old files");
            return true;
        }
    }

    return false;
}

void MainWindow::fs_check()
{
    QFileInfoList entries = QDir(m_hh_path).entryInfoList();

    auto ps_windows = get_windows (get_pid ("PokerStars.exe"));

    foreach (const QFileInfo & entry, entries) {
        if (entry.isFile() && entry.baseName().startsWith("HH") && !m_old_files.contains(entry.filePath())) {
            QString tourney_id = entry.baseName().split(" ")[1].mid(1);

            foreach (cWindow window, ps_windows) {
                if (window.title.contains(tourney_id) && search_hh_file (entry.filePath())) {
                    log ("Switching hud for window " + QString::number(quintptr(window.hwnd),16));
                    switch_hud (window.hwnd);
                    break;
                }
            }
        }
    }
}

/*void MainWindow::directory_changed(const QString &path)
{
    QFileInfoList entries = QDir(path).entryInfoList();
}

void MainWindow::file_changed(const QString &path)
{
    if(m_old_files.contains(path)) {
        m_fs_watcher.removePath(path);
        return;
    }

    if (search_hh_file(path)) {
        QString tourney_id = QFileInfo(path).baseName().split(" ")[1].mid(1);
        auto ps_windows = get_windows (get_pid ("PokerStars.exe"));

        foreach (cWindow window, ps_windows) {
            if (window.title.contains(tourney_id)) {
                log ("Switching hud for window " + QString::number(quintptr(window.hwnd),16));
                switch_hud (window.hwnd);
                break;
            }
        }
    }
}
*/

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::test_switch()
{
    auto ps_windows = get_windows (get_pid ("PokerTracker4.exe"));
    HWND parent_window = NULL;

    foreach (cWindow window, ps_windows) {
        if (window.title.contains(ui->lineEdit->text().trimmed())) {
            log ("Switching hud for window " + QString::number(quintptr(window.hwnd),16));
            parent_window = window.hwnd;
            //switch_hud (window.hwnd);

            break;
        }
    }

    DWORD            hud_pid        = get_pid("PokerTrackerHud4.exe");
    QVector<cWindow> hud_windows    = get_windows(hud_pid);

    RECT             pwr;
    cWindow          hud_win;
    hud_win.hwnd = NULL;
    HWND             command_window = NULL;

    GetWindowRect(parent_window, &pwr);

    for (int i = 0; i < hud_windows.length(); i++) {
        const cWindow & window = hud_windows.at(i);

        if (window.rect.left() <= pwr.left && window.rect.top() <= pwr.top &&
                window.rect.bottom() >= pwr.bottom && window.rect.right() >= pwr.right) {
            hud_win = window;
        }
        if (window.class_name == "wxWindowNR")
            command_window = window.hwnd;
    }

    if (!hud_win.hwnd)
    {
        log ("Did not find hud window");
        return;
    }

    log ("Found hud window " + QString::number(quintptr(hud_win.hwnd),16));

    QPixmap icon   = QPixmap("C:\\Users\\Claudiu\\Desktop\\output\\icon.bmp");
    QPixmap screen = QPixmap::grabWindow(QApplication::desktop()->winId());

    int icon_x = -1, icon_y = -1;

    log (QString("Searching for icon @ %1x%2").arg (hud_win.rect.x()).arg (hud_win.rect.y()));

    region_icon_search (screen, icon, hud_win.rect.x(), hud_win.rect.y(),
                        hud_win.rect.width(), 150,
                        icon_x, icon_y);

    if (icon_x == -1) {
        log ("Failed to find icon");
        return;
    }

    log (QString("Found icon @ %1x%2").arg(icon_x).arg(icon_y));

    send_click (icon_x, icon_y, hud_win.hwnd);
    Sleep (1000);

    QPoint fdp(-1, -1);
    bool finished = false;

    while (!finished) {
        hud_windows = get_windows (hud_pid);

        for (int i = 0; i < hud_windows.length(); i++) {
            const cWindow & window = hud_windows.at(i);

            if (fdp.x() == -1 && window.class_name == "#32768" && window.rect.x() != 0) {
                log(QString("HWND = %1 | x = %2 | y = %3")
                    .arg(QString::number(quintptr(window.hwnd), 16))
                    .arg(window.rect.x())
                    .arg(window.rect.y()));

                fdp = QPoint(window.rect.x(), window.rect.y());

                //send_click (fdp.x() + 100, fdp.y() + 185, window.hwnd);
                //log(QString("Sending first click at %1x%2 to %3").arg(fdp.x()+100).arg(fdp.y()+115).arg(QString::number(quintptr(window.hwnd), 16)));

                //send_click (fdp.x() + 100, fdp.y() + 119, window.hwnd);
                //SendMessage(window.hwnd, 0x92, 0, 0x19fb6c);
                log ("sending WM_COMMAND");
                PostMessage (command_window, WM_COMMAND, 0x1782, 0);
                //send_click (fdp.x() + 100, fdp.y() + 119, window.hwnd);
                //BringWindowToTop(parent_window);
                //SetActiveWindow(parent_window);
                //SetFocus(parent_window);

                //DestroyWindow(window.hwnd);
                return;

                break;
            }
            else if (QPoint (window.rect.x(), window.rect.y()) != fdp && fdp.x() != -1 && window.class_name == "#32768") {
                send_click (window.rect.x() + 129, window.rect.y() + 145, window.hwnd, false);
                log(QString("Sending second click at %1x%2").arg(window.rect.x()+129).arg(window.rect.y()+145));
                finished = true;
            }
        }
        Sleep (10);
    }
}


void MainWindow::log(QString str) {
    ui->plainTextEdit->appendPlainText(str);
    qDebug() << str;
}

void MainWindow::on_pushButton_clicked()
{
}
