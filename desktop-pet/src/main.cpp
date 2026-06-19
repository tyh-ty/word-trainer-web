#include "App.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    App app;
    int result = app.run(hInstance);
    CoUninitialize();
    return result;
}
