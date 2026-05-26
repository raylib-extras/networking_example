#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include "open_dialog.h"

int ShowOpenDialog(char* inPath, int maxLen, const char* defaultName) {
    if (defaultName != NULL) {
        strncpy(inPath, defaultName, maxLen - 1);
    }

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Map Files (*.fyl)\0*.fyl\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = inPath;
    ofn.nMaxFile = maxLen;
    ofn.lpstrDefExt = "fyl";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    return GetOpenFileNameA(&ofn) ? 1 : 0;
}