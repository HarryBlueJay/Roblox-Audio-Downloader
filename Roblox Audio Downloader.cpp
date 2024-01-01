#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <curl\curl.h>
#include <curl\easy.h>
#include <filesystem>
#include <thread>

std::string rootDir = "";
std::vector<int> aliasesize = { 2332449,2379343 };
std::vector<std::string> aliases = {"cake.mp3","energetic.mp3"};

char* lap = nullptr;
std::string localappdata;
size_t sz = 0;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
HWND consoleWindow = GetConsoleWindow();

//Tray icon code from stackoverflow and various other internet sources, I was unsure on how it worked.
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LPCWSTR lpszClass = L"__hidden__";

bool start = false;

using std::string;

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
        memcpy_s(nid.szTip, sizeof(nid.szTip), L"RAD <3\nClick to toggle console", sizeof(wchar_t[31]));
        Shell_NotifyIcon(NIM_ADD, &nid);
        return 0;
    case WM_APP + 1:
        switch (lParam) {
        case WM_RBUTTONDOWN: // yoink https://stackoverflow.com/questions/68474486/creating-system-tray-right-click-menu-c/68488511#68488511
            POINT pt;
            GetCursorPos(&pt);
            HMENU hmenu = CreatePopupMenu();
            if (!start) {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING | MF_CHECKED, 1, L"Copy Audio");
            }
            else {
                InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING | MF_GRAYED | MF_DISABLED, 1, L"Copy Audio");
            }
            InsertMenuW(hmenu, -1, MF_BYPOSITION | MF_STRING, 2, L"Exit");
            SetForegroundWindow(hWnd);
            int cmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
            switch (cmd) {
            case 1:
                break;
            case 2:
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
    std::cin.get();
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
        std::filesystem::copy_file(std::string(argv[0]),rootDir+"\\Roblox Audio Downloader.exe", std::filesystem::copy_options::overwrite_existing);
        //CreateProcess code from https://stackoverflow.com/a/15440094

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        CreateProcessA((rootDir + "\\Roblox Audio Downloader.exe").c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 4;
    }
    string UWPRobloxVersionFolder = "";
    for (const auto& e : std::filesystem::directory_iterator(localappdata + string("\\Packages"))) {
        if (e.is_directory()) {
            if (e.path().string().find("ROBLOXCORPORATION.ROBLOX") != std::string::npos) {
                UWPRobloxVersionFolder = e.path().string();
                goto exitNest;
            }
        }
    }
    if (UWPRobloxVersionFolder == "") {
        std::cout << "Failed to locate UWP Roblox, unable to continue.\n";
        std::cin.get();
        return 3;
    }
    exitNest:
    bool debugForceError = false;
    if (error || debugForceError) {
        std::cout << "Press any key to continue...\n";
        char throwaway = _getch();
    }
    system("cls");
    /*std::cout << "\033[45;97mThis is going on the floor or something idk\n \033[0m";
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get the console handle.
    PCONSOLE_SCREEN_BUFFER_INFO lpScreenInfo = new CONSOLE_SCREEN_BUFFER_INFO(); // Create a pointer to the Screen Info pointing to a temporal screen info.
    GetConsoleScreenBufferInfo(hConsole, lpScreenInfo); // Saves the console screen info into the lpScreenInfo pointer.
    SMALL_RECT coord = lpScreenInfo->srWindow;
    int x = coord.Right + 1;*/
    thingy:
    std::cout << "just so i don't end up taking until next year to work on this, here's this extremely alpha version (tray icon doesn't work)\npress whatever (except q) to copy sounds\n";
    char throwaway = _getch();
    if (throwaway == 'q') {
        return 0;
    }
    std::filesystem::remove_all(localappdata + "\\Roblox Audio Downloader\\sounds");
    std::filesystem::copy(UWPRobloxVersionFolder+"\\LocalState\\sounds",localappdata+"\\Roblox Audio Downloader\\sounds");
    std::cout << "thingy hopefully worked\n";
    int audiocount = 1;
    for (const auto& e : std::filesystem::directory_iterator(localappdata + "\\Roblox Audio Downloader\\sounds")) {
        std::string audioPath = localappdata + "\\Roblox Audio Downloader\\sounds\\audio" + std::to_string(audiocount) + ".mp3";
        std::filesystem::rename(e.path().string(), audioPath);
        // i was originally gonna use hashes, i tried them and it got the test audio confused with other stuff, i switched to file size and it works flawlessly now
        std::cout << std::filesystem::file_size(audioPath) << ", " << "audio"+std::to_string(audiocount) << std::endl;
        int aliasIndex = 0; 
        for (int& v : aliasesize) {
            if (std::filesystem::file_size(audioPath) == v) {
                std::filesystem::rename(audioPath, localappdata + "\\Roblox Audio Downloader\\sounds\\"+aliases[aliasIndex]);
                break;
            }
            aliasIndex++;
        }
        aliasIndex = 0;
        std::cout << std::endl;
        //if (std::filesystem::file_size(localappdata + "\\Roblox Audio Downloader\\sounds\\audio" + std::to_string(audiocount) + ".mp3") == 2379343) {
        //    std::filesystem::rename(localappdata + "\\Roblox Audio Downloader\\sounds\\audio" + std::to_string(audiocount) + ".mp3", localappdata + "\\Roblox Audio Downloader\\sounds\\idk.mp3");
        //} 
        audiocount++;
    }
    system(std::string("explorer \"" + localappdata + "\\Roblox Audio Downloader\\sounds" + "\"").c_str());
    Sleep(500);
    throwaway = _getch(); // i don't think i want to question how that fixes it
    throwaway = ' ';
    //clear();
    goto thingy;
}