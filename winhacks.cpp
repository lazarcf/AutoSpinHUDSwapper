#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ShlObj.h"
#include "TlHelp32.h"

MainWindow * g_mw;
QVector<HHOOK>   hooks;

LRESULT WINAPI CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

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

void MainWindow::switch_hud (HWND parent_window)
{
    DWORD            hud_pid        = get_pid("PokerTrackerHud4.exe");
    QVector<cWindow> hud_windows    = get_windows(hud_pid);
    RECT             pwr;
    cWindow          hud_win;
    HWND             command_window;

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
    log ("Found hud window " + QString::number(quintptr(hud_win.hwnd),16));

    QPixmap icon   = QPixmap("C:\\Users\\Claudiu\\Desktop\\output\\icon.bmp");
    QPixmap screen = QPixmap::grabWindow(QApplication::desktop()->winId());
    int icon_x = -1, icon_y = -1;

    // Search for icon in the top section of the overlay window

    // Retry for 15 seconds, window might not be visible
    int search_retry_counter = 31;

    while (search_retry_counter--) {
        if (search_retry_counter < 30)
            Sleep (500);

        screen = QPixmap::grabWindow(QApplication::desktop()->winId());

        if (m_icon_pos.x() != -1) {
            if (exact_icon_search (screen, icon, hud_win.rect.x() + m_icon_pos.x(), hud_win.rect.y() + m_icon_pos.y()))
                break;
        }

        log (QString("searching for icon @ %1x%2 , %3x%4").arg(hud_win.rect.x()).arg(hud_win.rect.y()).arg(hud_win.rect.width()).arg(150));

        region_icon_search (screen, icon,
                            hud_win.rect.x(), hud_win.rect.y(),
                            hud_win.rect.width(), 150,
                            icon_x, icon_y);
        if (icon_x == -1)
            log ("Did not find icon, retrying");
        else {
            m_icon_pos = QPoint (icon_x - hud_win.rect.x(), icon_y - hud_win.rect.y());
            break;
        }
    }

    if (search_retry_counter <= 0) {
        log ("Error, could not find icon");
        return;
    }

    icon_x = hud_win.rect.x() + m_icon_pos.x();
    icon_y = hud_win.rect.y() + m_icon_pos.y();

    log(QString("Found icon at %1x%2").arg(m_icon_pos.x()).arg(m_icon_pos.y()));

    // Don't click until all HUD context menus are destroyed
    while (true) {
        hud_windows = get_windows (hud_pid);
        bool good_to_go = true;

        for (int i = 0; i < hud_windows.length(); i++) {
            const cWindow & window = hud_windows.at(i);

            if (window.class_name == "#32768") {
                good_to_go = false;
                break;
            }
        }
        if (good_to_go)
            break;

        Sleep (100);
    }

    // Click icon
    send_click (icon_x, icon_y, hud_win.hwnd);
    Sleep (200);

    // Initiate menu clicking sequence
    QPoint fdp(-1, -1);
    bool finished = false;

    while (!finished) {
        hud_windows = get_windows (hud_pid);

        for (int i = 0; i < hud_windows.length(); i++) {
            const cWindow & window = hud_windows.at(i);

            if (fdp.x() == -1 && window.class_name == "#32768" && window.rect.x() != 0) {
                log(QString("HWND = %1 | x = %2 | y = %3")
                    .arg(QString::number(quintptr(window.hwnd)))
                    .arg(window.rect.x())
                    .arg(window.rect.y()));

                fdp = QPoint(window.rect.x(), window.rect.y());

                log(QString("Sending first click at %1x%2").arg(pwr.left + 300).arg(pwr.top + 50));

                PostMessage (command_window, WM_COMMAND, 0x1782, 0);
                //send_click (fdp.x() + 100, fdp.y() + 185, window.hwnd, false);
                //DestroyWindow(window.hwnd);
                send_click (pwr.left + 300, pwr.top + 50);
                //BringWindowToTop(parent_window);
                //SetActiveWindow(parent_window);
                //SetFocus(parent_window);

                finished = true;
            }
        }
        Sleep (10);
    }
}

bool MainWindow::exact_icon_search(QPixmap src, QPixmap icon, int x, int y) {
    QImage src_image = src.toImage();
    QImage icon_image = icon.toImage();

    for (int i = 0; i < icon.width(); i++) {
        for (int j = 0; j < icon.height(); j++) {
            if (src_image.pixel(x + i, y + j) != icon_image.pixel(i, j))
                return false;
        }
    }

    return true;
}

void MainWindow::region_icon_search(QPixmap src, QPixmap icon, int in_x, int in_y, int in_w, int in_h, int & out_x, int & out_y)
{
    for (int x = in_x; x < in_x + in_w - icon.width(); x++) {
        for (int y = in_y; y < in_y + in_h - icon.height(); y++) {
            if (exact_icon_search(src, icon, x, y)) {
                out_x = x;
                out_y = y;
                return;
            }
        }
    }
}

void MainWindow::send_click (int x, int y, HWND window, bool translate)
{
    POINT pt;
    pt.x = x;
    pt.y = y;

    if (window == NULL)
        window = WindowFromPoint (pt);

    if (translate)
        ScreenToClient(window, &pt);

    LPARAM coordinates = MAKELPARAM (pt.x, pt.y);

    qDebug() << "Buttondown result = " << PostMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, coordinates);
    qDebug() << "Buttunup   result = " << PostMessageA(window, WM_LBUTTONUP, 0, coordinates);
}

DWORD MainWindow::get_pid(QString proc_name)
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return 0;

    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++ ) {
        CHAR szProcessName[MAX_PATH] = "<unknown>";

        HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                       PROCESS_VM_READ,
                                       FALSE, aProcesses[i] );
        if (NULL != hProcess )
        {
            HMODULE hMod;
            DWORD cbNeeded;

            if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod),
                                     &cbNeeded) )
            {
                GetModuleBaseNameA(hProcess, hMod, szProcessName, MAX_PATH);
            }
        }
        if (QString(szProcessName) == proc_name)
            return aProcesses [i];

    }

    return 0;
}


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    QPair<DWORD, QVector<cWindow>*> *params = (QPair<DWORD, QVector<cWindow>*>*)lParam;

    DWORD hud_pid = params->first;
    DWORD current_window_pid = 0;
    QVector <cWindow> * hud_windows = params->second;

    GetWindowThreadProcessId (hwnd, &current_window_pid);

    if (current_window_pid == hud_pid) {
        char class_name[300] = "unknown";
        char title[300] = "-";
        GetClassNameA (hwnd, class_name, 300);

        RECT wr;
        GetWindowRect (hwnd, &wr);
        GetWindowTextA(hwnd, title, 300);

        cWindow window;
        window.hwnd = hwnd;
        window.class_name = QString (class_name);
        window.rect = QRect(wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top);
        window.title = QString (title);

        hud_windows->push_back(window);
    }

    return TRUE;
}

QVector<cWindow> MainWindow::get_windows (DWORD pid) {
    QVector<cWindow> windows;

    QPair<DWORD, QVector<cWindow>*> pair (pid, &windows);

    EnumWindows (&EnumWindowsProc, (LPARAM)&pair);

    return windows;
}
