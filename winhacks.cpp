#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMutex>
#include <QScreen>
#include <QTime>
#include "utils.h"

void switch_worker_t::work() {
    switch_hud();
    emit finished();
}

void switch_worker_t::switch_hud ()
{
    if (!IsWindow(parent_window)) {
        emit remove_pending(parent_window);
        return;
    }

    DWORD            hud_pid        = get_pid("PokerTrackerHud4.exe");
    QVector<cWindow> hud_windows    = get_windows(hud_pid);
    RECT             pwr;
    cWindow          hud_win;
    HWND             command_window = 0;

    if(hud_windows.size() == 0)
        return;

    // Search for HUD window which overlays parent_window

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

    if  (hud_win.hwnd == 0) {
        log ("Failed to find hud window");
        return;
    }

    if (command_window == 0) {
        log ("Failed to find command window");
        return;
    }

    QString rect_info = QString (" | location = (%1, %2) | size = %3x%4").arg(hud_win.rect.x()).arg(hud_win.rect.y()).arg(hud_win.rect.width()).arg(hud_win.rect.height());
    log ("Found hud window " + QString::number(quintptr(hud_win.hwnd),16) + rect_info);

    // Search for icon in the top section of the overlay window
    QPixmap icon   = QPixmap(":/resources/pt4_icon.png");
    QPixmap screenshot;
    QPoint screen_offset(0, 0);

    if (qApp->screens().size() == 1) {
        screenshot = qApp->screens().first()->grabWindow(0);
        screen_offset = qApp->screens().first()->geometry().topLeft();
    }
    else {
        log ("Found multiple screens");

        foreach (auto screen, qApp->screens()) {
            if (screen->geometry().contains(pwr.left, pwr.top, false)) {
                screenshot = screen->grabWindow(0,
                                                screen->geometry().x(), screen->geometry().y(),
                                                screen->geometry().width(), screen->geometry().height());
                screen_offset = screen->geometry().topLeft();
                log ("Found PS window inside screen " + screen->name());
                break;
            }
            else
                log (QString ("Did not find PS window (%1, %2) inside screen " + screen->name() + " , geometry = %3, %4").
                     arg(pwr.left).arg(pwr.top).arg(screen->geometry().x()).arg(screen->geometry().y()));
        }
    }

    //screenshot.save("C:/Users/Claudiu/Desktop/ss.png");

    int icon_x = -1, icon_y = -1;

    int region_height = 200;
    if (pwr.bottom - pwr.top < 200)
        region_height = pwr.bottom - pwr.top;

    region_icon_search (screenshot, icon,
                        pwr.left - screen_offset.x(), pwr.top - screen_offset.y(),
                        pwr.right - pwr.left, region_height,
                        icon_x, icon_y);

    if (icon_x == -1 || icon_y == -1) {
        log ("Failed to find icon");
        return;
    }

    log(QString("Found icon at %1x%2").arg(icon_x).arg(icon_y));

    icon_x += screen_offset.x();
    icon_y += screen_offset.y();

    // Don't click until all HUD context menus are destroyed
    QTime timeout;
    timeout.start();

    while (true) {
        hud_windows = get_windows (hud_pid);

        if (hud_windows.size() == 0)
            return;

        bool good_to_go = true;

        for (int i = 0; i < hud_windows.length(); i++) {
            const cWindow & window = hud_windows.at(i);

            if (window.class_name == "#32768") {
                if (timeout.elapsed() > 3000)
                    PostMessage (window.hwnd, WM_CLOSE, 0, 0);

                good_to_go = false;
                break;
            }
        }
        if (good_to_go)
            break;

        Sleep (100);

        if (timeout.elapsed() > 5000)
            return;
    }

    // Click icon
    send_click (icon_x, icon_y, hud_win.hwnd);
    Sleep (200);

    // Initiate menu clicking sequence
    bool finished = false;
    timeout.restart();

    while (!finished) {
        hud_windows = get_windows (hud_pid);

        if(hud_windows.size() == 0)
            return;

        for (int i = 0; i < hud_windows.length(); i++) {
            const cWindow & window = hud_windows.at(i);

            if (window.class_name == "#32768" && window.rect.x() != 0) {
                log(QString("HWND = %1 | x = %2 | y = %3")
                    .arg(QString::number(quintptr(window.hwnd)))
                    .arg(window.rect.x())
                    .arg(window.rect.y()));

                PostMessage (command_window, WM_COMMAND, 6011 + hud_index, 0);
                PostMessage (window.hwnd, WM_CLOSE, 0, 0);

                finished = true;
            }
        }

        Sleep (10);

        if (!finished && timeout.elapsed() > 1500)
            return;
    }

    emit remove_pending(parent_window);
}

void MainWindow::remove_pending_switch(HWND win) {
    m_pending_switches_mutex.lock();

    log ("Deleting thread " + QString::number(quintptr(m_pending_switches[win]), 16));
    QThread *thread = m_pending_switches[win];

    if (!thread->isFinished())
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    else
        thread->deleteLater();

    m_pending_switches.remove(win);

    m_pending_switches_mutex.unlock();
}

//MainWindow * g_mw;
//QVector<HHOOK>   hooks;

//LRESULT WINAPI CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);
//void MainWindow::test_hook()  {
//    DWORD            hud_pid        = get_pid("PokerTrackerHud4.exe");
//    HANDLE           hThreadSnap    = INVALID_HANDLE_VALUE;
//    THREADENTRY32    te32;
//    QVector<DWORD>   hud_threads;

//    hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, hud_pid);

//    if (hThreadSnap == INVALID_HANDLE_VALUE) {
//        log ("Failed to create thread snapshot");
//        return;
//    }

//    te32.dwSize = sizeof(THREADENTRY32);

//    if (!Thread32First( hThreadSnap, &te32 ) )
//    {
//        log("Thread32First failed" );
//        CloseHandle( hThreadSnap );
//        return;
//    }

//    do {
//        if (te32.th32OwnerProcessID == hud_pid) {
//            //log ("Thread id = " + QString::number(te32.th32ThreadID));
//            //log ("Base priority = " + QString::number(te32.tpBasePri));
//            hud_threads.append(te32.th32OwnerProcessID);
//        }
//    }
//    while (Thread32Next(hThreadSnap, &te32));

//    CloseHandle( hThreadSnap );

//    foreach (auto thread_id, hud_threads) {
//        HHOOK hook = SetWindowsHookExA(WH_CALLWNDPROC, CallWndProc, HINSTANCE (NULL), thread_id);
//        hooks.append (hook);
//    }

//    g_mw = this;
//    g_mw->log("test");
//}


//LRESULT WINAPI CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)  {
//    CHAR szCWPBuf[256];
//    CHAR szMsg[16];
//    HDC hdc;
//    static int c = 0;
//    size_t cch;
//    HRESULT hResult;

//    if (nCode < 0)  // do not process message
//        return CallNextHookEx(myhookdata[IDM_CALLWNDPROC].hhook, nCode, wParam, lParam);

//}
