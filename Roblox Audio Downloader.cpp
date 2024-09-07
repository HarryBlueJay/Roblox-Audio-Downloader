#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <curl\curl.h>
#include <curl\easy.h>
#include <filesystem>
#include <thread>
#include <psapi.h>

std::string rootDir = "";
std::vector<int> aliassize = { 2332449,2379343 };
std::vector<std::string> aliases = { "cake.mp3","energetic.mp3" };
// the error code of "shut up it's fine"
std::error_code ec = std::error_code::error_code();

char* lap = nullptr;
std::string localappdata;
size_t sz = 0;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
HWND consoleWindow = GetConsoleWindow();

//Tray icon code from stackoverflow and various other internet sources, I was unsure on how it worked.
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LPCWSTR lpszClass = L"__hidden__";

bool loaded = false;
bool shown = true;

using std::string;
bool useNormalRobloxFolder = true;
string UWPRobloxFolder = "";
string robloxTempFolder = ""; 
bool robloxExists = false;
// if "userPresenceType": 2

void setRunningVersion() {
    // just quick checks if only one version exists
    if (robloxExists && UWPRobloxFolder == "") { useNormalRobloxFolder = true; return; }
    if (!robloxExists && UWPRobloxFolder != "") { useNormalRobloxFolder = false; return; }
    bool UWPRunning = false;
    bool robloxRunning = false;
    // thank you microsoft my brain was starting to implode reading the documentation
    //unsigned int i;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        std::cout << "Failed to get running roblox version, preferring the web client.\n";
        return;
    }
    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, aProcesses[i]);
            char filename[MAX_PATH] = {};
            GetModuleFileNameExA(process, NULL, filename, MAX_PATH);
            if (string(filename).find("Windows10Univeral.exe") != std::string::npos) {
                UWPRunning = true;
            }
            if (string(filename).find("RobloxPlayerBeta.exe") != std::string::npos) {
                robloxRunning = true;
            }
        }
    }
    if (UWPRunning) {
        useNormalRobloxFolder = false;
    }
    if (robloxRunning) {
        useNormalRobloxFolder = true;
    }
    if (UWPRunning && robloxRunning) {
        std::cout << "Multiple different versions of the roblox client are running. Sounds will be copied from the web client.\n";
    }
}

void copyAudios() {
    loaded = false;
    setRunningVersion();
    std::filesystem::remove_all(localappdata + "\\Roblox Audio Downloader\\sounds");
    std::filesystem::copy((useNormalRobloxFolder ? robloxTempFolder : UWPRobloxFolder + "\\LocalState") + "\\sounds", localappdata + "\\Roblox Audio Downloader\\sounds");
    int audiocount = 1;
    for (const auto& e : std::filesystem::directory_iterator(localappdata + "\\Roblox Audio Downloader\\sounds")) {
        std::string audioPath = localappdata + "\\Roblox Audio Downloader\\sounds\\audio" + std::to_string(audiocount) + ".mp3";
        std::filesystem::rename(e.path().string(), audioPath);
        // i was originally gonna use hashes, i tried them and it got the test audio confused with other stuff, i switched to file size and it works flawlessly now
        //std::cout << std::filesystem::file_size(audioPath) << ", " << "audio" + std::to_string(audiocount) << std::endl;
        int aliasIndex = 0;
        for (int& v : aliassize) {
            if (std::filesystem::file_size(audioPath) == v) {
                std::filesystem::rename(audioPath, localappdata + "\\Roblox Audio Downloader\\sounds\\" + aliases[aliasIndex]);
                break;
            }
            aliasIndex++;
        }
        aliasIndex = 0;
        audiocount++;
    }
    system(std::string("explorer %localappdata%\\Roblox Audio Downloader\\sounds").c_str()); // that works
    loaded = true;
}

int traySystem() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc;
    HWND hWnd;
    MSG msg;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = nullptr;
    wc.hCursor = nullptr;
    wc.hIcon = nullptr;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = lpszClass;
    wc.lpszMenuName = nullptr;
    wc.style = 0;
    RegisterClass(&wc);

    hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    static NOTIFYICONDATA nid;

    switch (iMsg) {
    case WM_CREATE:
        std::memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = iMsg; // yoink https://stackoverflow.com/questions/68474486/creating-system-tray-right-click-menu-c/68488511#68488511
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
        nid.uCallbackMessage = WM_APP + 1;
        nid.hIcon = (HICON)LoadImageA(NULL, (rootDir + "\\rad.ico").c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);
        memcpy_s(nid.szTip, sizeof(nid.szTip), L"RAD <3\nDouble click to toggle console", sizeof(wchar_t[38]));
        Shell_NotifyIcon(NIM_ADD, &nid);
        return 0;
    case WM_APP + 1:
        switch (lParam) {
        case WM_LBUTTONDBLCLK:
            if (shown) {
                ShowWindow(consoleWindow, SW_HIDE);
            }
            else {
                ShowWindow(consoleWindow, SW_SHOW);
            }
            shown = !shown;
            break;
        case WM_RBUTTONDOWN: // yoink https://stackoverflow.com/questions/68474486/creating-system-tray-right-click-menu-c/68488511#68488511
            POINT pt;
            GetCursorPos(&pt);
            HMENU hmenu = CreatePopupMenu();
            if (loaded) {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 1, L"Copy Audio");
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 2, L"Remove External Audio");
            }
            else {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING | MF_GRAYED | MF_DISABLED, 1, L"Copy Audio");
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING | MF_GRAYED | MF_DISABLED, 2, L"Remove External Audio");
            }
            if (shown) {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 3, L"Hide Window");
            }
            else {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 3, L"Show Window");
            }
            InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 4, L"Exit");
            SetForegroundWindow(hWnd);
            int cmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
            switch (cmd) {
            case 1:
                copyAudios();
                break;
            case 2:
                std::filesystem::remove_all((useNormalRobloxFolder ? robloxTempFolder : UWPRobloxFolder + "\\LocalState") + "\\sounds",ec);
                break;
            case 3:
                if (shown) {
                    ShowWindow(consoleWindow, SW_HIDE);
                }
                else {
                    ShowWindow(consoleWindow, SW_SHOW);
                }
                shown = !shown;
                break;
            case 4:
                ExitProcess(0);
                break;
            }
            PostMessage(hWnd, WM_NULL, 0, 0);
            break;
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int downloadFile(FILE* file, std::string url) {
    int retryAttempts = 0;
    int http_code = 0;
    retryDownload:
    CURL* req = curl_easy_init();
    CURLcode res;
    curl_easy_setopt(req, CURLOPT_URL, url);
    curl_easy_setopt(req, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(req, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(req, CURLOPT_FOLLOWLOCATION);
    curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(req, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(req);
    curl_easy_getinfo(req, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(req);
    if (retryAttempts < 3 && http_code > 399) {
        retryAttempts++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        goto retryDownload;
    }
    if (http_code == 0)
        return 404;
    return http_code;
}

void clear() {
    COORD topLeft = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}
int main(int argc, char* argv[]) {
    SetConsoleTitleA("Roblox Audio Downloader");
    bool error = false;
    std::cout << "Setting up..." << std::endl;
    if (!(_dupenv_s(&lap, &sz, "localappdata") == 0 && lap != nullptr)) {
        std::cout << "Failed to locate localappdata, unable to continue.\n";
        std::cin.get();
        return 1;
    }
    localappdata = lap;
    rootDir = localappdata + "\\Roblox Audio Downloader";
    if (std::filesystem::exists(rootDir) == false) {
        try {
            std::filesystem::create_directory(rootDir);
        }
        catch (std::filesystem::filesystem_error) {
            std::cout << "Failed to create RAD folder, unable to continue.\n";
            std::cin.get();
            return 2;
        }
    }
    if (std::filesystem::exists(rootDir + "\\rad.ico") == false) {
        FILE* icon;
        int response = 0;
        if (fopen_s(&icon, (rootDir + "\\rad.ico").c_str(), "wb") != 0) {
            std::cout << "Failed to create icon, skipping download.\n";
            error = true;
            goto skipDownload;
        }
        response = downloadFile(icon, "https://raw.githubusercontent.com/HarryBlueJay/Roblox-Audio-Downloader/main/rad.ico");
        if (response > 399) {
            std::cout << "Failed to download icon, shell icon unavailible.\n";
            error = true;
            fclose(icon);
            std::error_code ec;
            std::filesystem::remove(rootDir + "\\rad.ico", ec);
            goto skipDownload;
        }
        fclose(icon);
    }
    skipDownload:
    if (std::string(argv[0]).find(rootDir) == std::string::npos && std::string(argv[0]).find("coding time\\C++\\Roblox Audio Downloader") == std::string::npos) {
        std::filesystem::copy_file(std::string(argv[0]), rootDir + "\\Roblox Audio Downloader.exe", std::filesystem::copy_options::overwrite_existing);
        //CreateProcess code from https://stackoverflow.com/a/15440094

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        CreateProcessA((rootDir + "\\Roblox Audio Downloader.exe").c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 3;
    }

    for (const auto& e : std::filesystem::directory_iterator(localappdata + string("\\Packages"))) {
        if (e.is_directory()) {
            if (e.path().string().find("ROBLOXCORPORATION.ROBLOX") != std::string::npos) {
                UWPRobloxFolder = e.path().string();
                break;
            }
        }
    }
    HKEY hKey;
    //char thingy[260]{};

    //this only has to check if this exists, if it exists, roblox must be installed, otherwise roblox would be broken
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ROBLOX Corporation\\Environments\\roblox-player", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        robloxExists = true;
        /* unsigned long size = 260;
         RegGetValueA(hKey, "", "InstallLocation", RRF_RT_REG_SZ, NULL, &thingy, &size);
         RegCloseKey(hKey);*/
    }
    if (!robloxExists && UWPRobloxFolder == "") {
        std::cout << "You might wanna install Roblox first... (If you have, open an issue)\n";
        std::cin.get();
        return 4;
    }
    if (robloxExists) {
        robloxTempFolder = localappdata + "\\temp\\roblox";
    }
    //Initialize the tray icon system
    std::thread t1(traySystem);
    bool debugForceError = true;
    if (error || debugForceError) {
        std::cout << "Press any key to continue...\n";
        char throwaway = _getch();
    }
    loaded = true;
    system("cls");
    std::cout << "the ui sucks, i'm aware, tray icon now works\npress whatever (except q) to copy sounds\n";
    thingy:
    char throwaway = _getch();
    if (throwaway == 'q') {
        ExitProcess(0);
    }
    setRunningVersion();
    copyAudios();
    Sleep(500);
    throwaway = _getch(); // i don't think i want to question how that fixes it
    throwaway = ' ';
    //clear();
    goto thingy;
}