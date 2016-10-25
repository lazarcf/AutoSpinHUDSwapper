#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <functional>
#include <QThread>

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
    m_settings("GTO Software", "AutoSpinHUDSwapper"),
    m_icon_pos(-1, -1)
{
    ui->setupUi(this);

    if (!m_settings.contains("hud_index"))
        m_settings.setValue("hud_index", 7);

    ui->hud_index_edit->setText(m_settings.value("hud_index").toString());

    if (m_settings.contains("hh_path")) {
        ui->hh_path_edit->setText(m_settings.value("hh_path").toString());
        m_hh_path = m_settings.value("hh_path").toString();
    }

    if (m_settings.contains("hero")) {
        ui->hero_name_edit->setText(m_settings.value("hero").toString());
        m_username = ui->hero_name_edit->text();
    }

    // Setup tray icon
    QIcon * icon = new QIcon(":/resources/swapper_icon.png");
    this->setWindowIcon(*icon);
    m_systray.setIcon(*icon);
    m_systray.show();

    QAction *set_hh_action = new QAction("Set Hand History path", &m_systray_menu);
    QAction *set_hero_name_action = new QAction("Set Hero name", &m_systray_menu);
    QAction *set_hud_index_action = new QAction("Set HU HUD index", &m_systray_menu);

    QAction *show_action = new QAction("Show / Hide", &m_systray_menu);
    QAction *quit_action = new QAction("Exit", &m_systray_menu);

    m_systray_menu.addAction(set_hh_action);
    m_systray_menu.addAction(set_hero_name_action);
    m_systray_menu.addAction(set_hud_index_action);

    m_systray_menu.addSeparator();

    m_systray_menu.addAction(show_action);
    m_systray_menu.addAction(quit_action);

    connect(set_hh_action, SIGNAL(triggered(bool)), this, SLOT(set_hh_path_slot()));
    connect(set_hero_name_action, SIGNAL(triggered(bool)), this, SLOT(set_hero_name_slot()));
    connect(set_hud_index_action, SIGNAL(triggered(bool)), this, SLOT(set_hud_index_slot()));

    connect(show_action, SIGNAL(triggered(bool)), this, SLOT(show_hide_window()));
    connect(quit_action, SIGNAL(triggered(bool)), this, SLOT (exit_slot()));

    connect (&m_systray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT (tray_activated_slot(QSystemTrayIcon::ActivationReason)));

    m_systray.setContextMenu(&m_systray_menu);
    m_systray.show();


    QString warning;

    if (!m_settings.contains("hero"))
        warning = "You must set the hero name";

    if (!m_settings.contains("hh_path")) {
        if (warning.length() == 0)
            warning = "You must set the hand history path";
        else
            warning += " and hand history path";
    }

    if (warning.length() > 0)
        m_systray.showMessage("Warning", warning, QSystemTrayIcon::Warning);

    m_fs_timer = new QTimer (this);
    QObject::connect(m_fs_timer, SIGNAL (timeout()), this, SLOT (fs_check()));
    m_fs_timer->start(1000);

    QTimer::singleShot(900, this, SLOT(switch_pending()));
    log("version 1.0.0");
}

void MainWindow::show_hide_window() {
    if (this->isHidden()) {
        this->show();
        ShowWindow ((HWND)this->effectiveWinId(), SW_SHOWNORMAL);
    }
    else
        this->hide();
}

void MainWindow::tray_activated_slot(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick)
        show_hide_window();
}

void MainWindow::changeEvent (QEvent *) {
    if (this->windowState() & Qt::WindowMinimized) {
        this->hide();
    }
}

void MainWindow::exit_slot() {
    m_systray.hide();
    exit(0);
}

void MainWindow::set_hud_index_slot() {
    bool ok;
    int index = QInputDialog::getInt(0, "Set HUD index", "HUD index : ", m_settings.value("hud_index").toInt(), 1, 15, 1, &ok);

    if (ok) {
        m_settings.setValue("hud_index", index);

        QMessageBox::information(0, "Info", "Successfully set hud index to " + QString::number(index));
    }
}

void MainWindow::set_hero_name_slot() {
    QString hero;
    if (m_settings.contains("hero"))
        hero = m_settings.value("hero").toString();

    bool ok;
    hero = QInputDialog::getText(0, "Set Hero name", "Hero name : ", QLineEdit::Normal, hero, &ok);

    if (ok && hero != "") {
        m_settings.setValue("hero", hero);

        ui->hero_name_edit->setText(hero);
        m_username = ui->hero_name_edit->text();
        QMessageBox::information(0, "Info", "Successfully set hero name to '" + hero + "'");
    }
}

void MainWindow::set_hh_path_slot() {
    QString start_dir = "C:/";
    if (m_settings.contains("hh_path"))
        start_dir = m_settings.value("hh_path").toString();

    QString path = QFileDialog::getExistingDirectory(0, "Open Directory", start_dir, QFileDialog::ShowDirsOnly);

    qDebug() << "Setting hh_path to : " << path;

    if (path.length() > 0) {
        m_settings.setValue("hh_path", path);

        m_hh_path = path;

        QMessageBox::information(0, "Info", "Successfully set hand history path to '" + path + "'");
    }
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

            if (!m_old_files.contains(path))
                log (username + " has left in 3rd place");

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
    //log (m_hh_path);
    QFileInfoList entries = QDir(m_hh_path).entryInfoList();

    auto ps_windows = get_windows (get_pid ("PokerStars.exe"));

    //log (QString("Searching %1 entries, found %2 pokerstars windows").arg(entries.length()).arg(ps_windows.length()));

    foreach (const QFileInfo & entry, entries) {
        //log (entry.filePath());
        if (entry.isFile() && entry.baseName().startsWith("HH") && !m_old_files.contains(entry.filePath())) {
            QString tourney_id = entry.baseName().split(" ")[1].mid(1);

            foreach (cWindow window, ps_windows) {
                if (window.title.contains(tourney_id) && search_hh_file (entry.filePath())) {
                    log ("Switching hud for window " + QString::number(quintptr(window.hwnd),16));

                    switch_hud(window.hwnd);

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
    /*auto ps_windows = get_windows (get_pid ("PokerTracker4.exe"));
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
    }*/
}


void MainWindow::log(QString str) {
    ui->plainTextEdit->appendPlainText(str);
    qDebug() << str;
}

void MainWindow::on_pushButton_clicked()
{
    int x = 647;
    int y = 24;

    DWORD            hud_pid        = get_pid("PokerTrackerHud4.exe");
    QVector<cWindow> hud_windows    = get_windows(hud_pid);

    foreach (auto window, hud_windows) {
        send_click (x, y, window.hwnd);
        qDebug() << "Sent click to " << QString::number ((quintptr)window.hwnd, 16) << " | " << window.rect;
        QMessageBox::information(this, "", "next?");
    }
}

void MainWindow::on_hero_name_edit_editingFinished()
{
    if (m_settings.contains("hero") && ui->hero_name_edit->text() == m_settings.value("hero").toString())
        return;

    m_settings.setValue("hero", ui->hero_name_edit->text());
    m_username = ui->hero_name_edit->text();

    QMessageBox::information(0, "Info", "Successfully set hero name to '" + ui->hero_name_edit->text() + "'");
}

void MainWindow::on_hud_index_edit_editingFinished()
{
    if (m_settings.contains("hud_index") && ui->hud_index_edit->text().trimmed().toInt() == m_settings.value("hud_index").toInt())
        return;

    bool ok = false;
    int index = ui->hud_index_edit->text().toInt(&ok);

    if (!ok || index < 1) {
        QMessageBox::warning(0, "Warning", "HUD index must be a number bigger than 1");
        ui->hud_index_edit->setText(m_settings.value("hud_index").toString());
        return;
    }

    m_settings.setValue("hud_index", index);

    QMessageBox::information(0, "Info", "Successfully set hud index to " + ui->hud_index_edit->text());
}

void MainWindow::on_hh_path_button_clicked()
{
    set_hh_path_slot();

    if (m_settings.contains("hh_path"))
        ui->hh_path_edit->setText(m_settings.value("hh_path").toString());
}

void MainWindow::switch_pending() {
    foreach(HWND win, m_pending_switches.keys()) {
        switch_hud(win);
    }

    QTimer::singleShot(900, this, SLOT(switch_pending()));
}

void MainWindow::switch_hud(HWND win) {
    if (m_pending_switches.contains(win)) {
        if (m_pending_switches[win]->isFinished())
            delete m_pending_switches[win];
        else
            return;
    }

    QThread * thread = new QThread;
    switch_worker_t *worker = new switch_worker_t(win, m_settings.value("hud_index", 7).toInt());

    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(work()));
    connect(worker, SIGNAL(log(QString)), this, SLOT(log(QString)));
    connect(worker, SIGNAL(remove_pending(HWND)), this, SLOT(remove_pending_switch(HWND)));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));

    thread->start();

    m_pending_switches_mutes.lock();

    m_pending_switches[win] = thread;

    m_pending_switches_mutes.unlock();
}
