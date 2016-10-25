#ifndef UTILS_H
#define UTILS_H

#include <QImage>
#include <QPixmap>
#include <windows.h>

struct cWindow{
    cWindow() {
        hwnd = 0;
    }

    HWND hwnd;
    QString class_name;
    QString title;
    QRect rect;
};

bool exact_icon_search(QPixmap src, QPixmap icon, int x, int y);

void region_icon_search(QPixmap src, QPixmap icon, int in_x, int in_y, int in_w, int in_h, int & out_x, int & out_y);

void send_click (int x, int y, HWND window, bool translate=true);

DWORD get_pid(QString proc_name);

QVector<cWindow> get_windows (DWORD pid);

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

#endif // UTILS_H
