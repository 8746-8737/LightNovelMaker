#include <windows.h>
#include <string>
#include <iostream>
#include <locale>
#include <clocale>
#include <wchar.h>
#include <sstream>
#include <fstream>
#include <commdlg.h>
#include <vector>

#pragma warning(suppress : 28251)

using namespace std;

struct WindowSize {
    int width;
    int height;
};

struct Page {
    int width = 558;
    int height = 798;
    int maxCharPerLine;
    int maxLineCount;

    Page(int maxChars = 50, int maxLines = 10)
        : maxCharPerLine(maxChars), maxLineCount(maxLines) {}
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
wstring PaginateText(const wstring& content, const Page& page);
string readTextFile(const wstring& filePath);

HWND selectTextFileButton;
HWND lightNovelText;

DWORD savedCaretStart = 0;
DWORD savedCaretEnd = 0;

#define ID_SELECT_TEXT_FILE_BUTTON 1001
#define WM_FILE_UPDATED (WM_USER + 1)

WindowSize getWindowSize(HWND hwnd) {
    RECT rect;
    WindowSize size = { 0, 0 };
    if (GetClientRect(hwnd, &rect)) {
        size.width = rect.right - rect.left;
        size.height = rect.bottom - rect.top;
    }
    return size;
}

string convertWStringToUTF_8(const wstring& wideStr) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return "";
    }

    std::string utf8Str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], sizeNeeded, nullptr, nullptr);
    return utf8Str;
}

wstring convertUTF_8ToWString(const string& utf8Str) {
    int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideCharSize == 0) {
        MessageBox(NULL, L"UTF-8からワイド文字列への変換に失敗しました。", L"エラー", MB_OK);
        return L"";
    }
    wstring wideStr(wideCharSize, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharSize);

    OutputDebugString(wideStr.c_str());
    return wideStr;
}

string readTextFile(const wstring& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        MessageBox(NULL, L"ファイルを開くことができませんでした", L"エラー", MB_OK | MB_ICONERROR);
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

wstring PaginateText(const wstring& content, const Page& page) {
    wstring result;
    int currentLine = 0;
    int currentChar = 0;

    const wstring pageBreak = L"\r\n---------\r\n";
    wstring cleanedContent = content;

    size_t pos = 0;
    while ((pos = cleanedContent.find(pageBreak, pos)) != wstring::npos) {
        cleanedContent.erase(pos, pageBreak.length());
    }

    for (size_t i = 0; i < cleanedContent.length(); ++i) {
        wchar_t c = cleanedContent[i];

        if (c == L'\n') {
            result += L"\r\n";
            currentLine++;
            currentChar = 0;
        }
        else {
            if (currentChar >= page.maxCharPerLine) {
                result += L"\r\n";
                currentLine++;
                currentChar = 0;
            }

            result += c;
            currentChar++;
        }

        if (currentLine >= page.maxLineCount) {
            result += L"\r\n---------\r\n";
            currentLine = 0;
            currentChar = 0;
        }
    }

    if (!result.empty() && result.compare(result.length() - pageBreak.length(), pageBreak.length(), pageBreak) == 0) {
        result.erase(result.length() - pageBreak.length());
    }

    return result;
}

void SetTextBoxFont(HWND hWnd, const wchar_t* fontName, int fontSize) {
    HFONT hFont = CreateFont(
        fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, fontName);

    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void formatLightNovelText(const wstring& filePath) {
    string utf8Content = readTextFile(filePath);
    wstring content = convertUTF_8ToWString(utf8Content);
    wstring paginatedText = PaginateText(content, Page(50, 10));

    SetWindowText(lightNovelText, paginatedText.c_str());
}


void drawLayout(HWND m_hwnd) {
    WindowSize size = getWindowSize(m_hwnd);

    const int buttonWidth = 100;
    const int buttonHeight = 50;

    const int LN_text_Width = 500;
    const int LN_text_Height = 800;

    int xPosBTN = (size.width - buttonWidth);
    int yPosBTN = (size.height - buttonHeight);
    int xPosTXT = (size.width - LN_text_Width);
    int yPosTXT = (size.height - LN_text_Height);

    lightNovelText = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
        xPosTXT / 10,
        yPosTXT / 2,
        LN_text_Width,
        LN_text_Height,
        m_hwnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
        NULL
    );

    if (lightNovelText == NULL) {
        MessageBox(NULL, L"テキストボックスの作成に失敗しました。", L"エラー", MB_OK | MB_ICONERROR);
        OutputDebugString(L"テキストボックスの作成に失敗しました。\n");
    }

    selectTextFileButton = CreateWindow(
        L"BUTTON",
        L"ラノベを選ぶ",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        xPosBTN / 2,
        yPosBTN - 100,
        buttonWidth,
        buttonHeight,
        m_hwnd,
        (HMENU)ID_SELECT_TEXT_FILE_BUTTON,
        (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
        NULL
    );
}

HANDLE hDirectory = NULL;
wstring monitoredFilePath;
bool isMonitoringFile = false;

DWORD currentSelStart = 0;
DWORD currentSelEnd = 0;

DWORD WINAPI MonitorFile(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    wstring dirPath = monitoredFilePath.substr(0, monitoredFilePath.find_last_of(L"\\/"));

    HANDLE hDir = CreateFile(
        dirPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        return 1;
    }

    char buffer[1024];
    DWORD bytesReturned;

    while (true) {
        BOOL success = ReadDirectoryChangesW(
            hDir,
            &buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL
        );

        if (success) {
            PostMessage(hwnd, WM_FILE_UPDATED, 0, 0);
        }
    }

    CloseHandle(hDir);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        if (isMonitoringFile) {
            CloseHandle(hDirectory);
        }
        PostQuitMessage(0);
        return 0;

    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SELECT_TEXT_FILE_BUTTON) {
            OPENFILENAME ofn;
            wchar_t szFile[260] = { 0 };

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = L"テキストファイルを選択";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                monitoredFilePath = ofn.lpstrFile;
                formatLightNovelText(monitoredFilePath);
                isMonitoringFile = true;
                hDirectory = CreateThread(NULL, 0, MonitorFile, (LPVOID)hwnd, 0, NULL);
            }
        }
        return 0;

    case WM_FILE_UPDATED:
    {
        formatLightNovelText(monitoredFilePath);
        return 0;
    }

    case WM_SIZE:
        drawLayout(hwnd);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    setlocale(LC_ALL, "");
    const wchar_t CLASS_NAME[] = L"LightNovelApp";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Light Novel Reader",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    savedCaretStart = 0;
    savedCaretEnd = 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
