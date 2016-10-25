#include "utils.h"
#include <QDebug>
#include "psapi.h"
//#include "ShlObj.h"
//#include "TlHelp32.h"

bool exact_icon_search(QPixmap src, QPixmap icon, int x, int y) {
    QImage src_image = src.toImage();
    QImage icon_image = icon.toImage();

    if (x + icon.width() > src.width())
        return false;

    if (y + icon.height() > src.height())
        return false;

    if (x < 0) x = 0;
    if (y < 0) y = 0;

    for (int i = 0; i < icon.width(); i++) {
        for (int j = 0; j < icon.height(); j++) {
            if (src_image.pixel(x + i, y + j) != icon_image.pixel(i, j))
                return false;
        }
    }

    return true;
}

void region_icon_search(QPixmap src, QPixmap icon, int in_x, int in_y, int in_w, int in_h, int & out_x, int & out_y)
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

void send_click (int x, int y, HWND window, bool translate)
{
    POINT pt;
    pt.x = x;
    pt.y = y;

    if (window == NULL) {
        window = WindowFromPoint (pt);
        qDebug() << "win from point : " << QString::number(quintptr(window), 16);
    }

    if (translate)
        ScreenToClient(window, &pt);

    LPARAM coordinates = MAKELPARAM (pt.x, pt.y);

    qDebug() << "Sending click to " << QString::number (quintptr(window), 16) << " at " << pt.x << ", " << pt.y << " | original coords : " << x << ", " << y;
    PostMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, coordinates);
    PostMessageA(window, WM_LBUTTONUP, 0, coordinates);
}

DWORD get_pid(QString proc_name)
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

QVector<cWindow> get_windows (DWORD pid) {
    QVector<cWindow> windows;

    if (pid == 0)
        return windows;

    QPair<DWORD, QVector<cWindow>*> pair (pid, &windows);

    EnumWindows (&EnumWindowsProc, (LPARAM)&pair);

    return windows;
}
