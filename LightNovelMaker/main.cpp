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

//namespace
using namespace std;

//プロトタイプ宣言

//struct
struct WindowSize {
	int width;
	int height;
};

//ボタン
HWND selectTextFileButton;

//static
HWND lightNovelText;

// ボタンのIDを定義
#define ID_SELECT_TEXT_FILE_BUTTON 1001  // ボタンID

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
		return L"";
	}

	wstring wideStr(wideCharSize, L'\0');

	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharSize);

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


void formatLightNovelText(const wstring& filePath) {
	string utf8Content = readTextFile(filePath);
	wstring content = convertUTF_8ToWString(utf8Content);

	SetWindowText(lightNovelText, L"");

	vector<wstring> rows;

	size_t start = 0;
	size_t end = content.find(L'\n');

	while (end != wstring::npos) {
		rows.push_back(content.substr(start, end - start));
		start = end + 1;
		end = content.find(L'\n', start);
	}

	if (start < content.length()) {
		rows.push_back(content.substr(start));
	}

	std::wstring verticalText;

	for (const auto& row : rows) {
		for (const auto& ch : row) {
			verticalText += ch;
			verticalText += L'\n';
		}
	}

	SetWindowText(lightNovelText, verticalText.c_str());
}


void drawLayout(HWND m_hwnd) {

	WindowSize size = getWindowSize(m_hwnd);

	const int buttonWidth = 100;
	const int buttonHeight = 50;

	const int LN_text_Width = 558;
	const int LN_text_Height = 793;

	int xPosBTN = (size.width - buttonWidth);
	int yPosBTN = (size.height - buttonHeight);
	int xPosTXT = (size.width - LN_text_Width);
	int yPosTXT = (size.height - LN_text_Height);

	lightNovelText = CreateWindow(
		L"STATIC",
		L"placeholder text",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD,
		xPosTXT / 2,
		25,
		LN_text_Width,
		LN_text_Height,
		m_hwnd,
		NULL,
		(HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
		NULL
	);

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
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
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = L"テキストファイル\0*.TXT\0全てのファイル\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn)) {
				formatLightNovelText(ofn.lpstrFile);

			} else {
				MessageBox(hwnd, L"ファイル選択がキャンセルされました", L"情報", MB_OK);
			}
		}

		InvalidateRect(hwnd, NULL, TRUE);  // ウィンドウの再描画要求
		return 0;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		
		EndPaint(hwnd, &ps);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	setlocale(LC_CTYPE, "ja_JP.UTF-8");

	const wchar_t CLASS_NAME[] = L"LightNovelMaker";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"ライト・ノベル・メイカー",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL) {
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	drawLayout(hwnd);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}