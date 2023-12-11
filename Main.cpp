#include <iostream>
#include <Windows.h>

// ������� ��� ��������� ������ �� ������������
template <typename T>
void getUserInput(const char* prompt, T& value) {
    std::cout << prompt << ": ";
    std::cin >> value;
}

// ������� ��� ��������� ���� � DLL
void getDLLpath(char* dll) {
    getUserInput("Please enter the path to DLL-file", dll);
}

// ������� ��� ��������� PID �������� ��������
void getPID(DWORD& PID) {
    getUserInput("Enter the PID of your target process", PID);
}

// ������� ��� �������� ����������� �������� ��������
HANDLE openProcess(DWORD pid) {
    HANDLE handleToProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    return handleToProc;
}

// ������� �������� DLL � ������� �������
bool injectDLL(DWORD PID, const char* dll) {
    // ��������� ���������� �������� ��������
    HANDLE handleToProc = openProcess(PID);
    if (!handleToProc) {
        std::cout << "Unable to open process.\n";
        return false;
    }

    // �������� ����� ������� LoadLibraryA �� kernel32.dll
    LPVOID LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!LoadLibAddr) {
        std::cout << "Unable to get address of LoadLibraryA.\n";
        CloseHandle(handleToProc);
        return false;
    }

    // �������� ����� ���� � DLL
    int dllLength = strlen(dll) + 1;
    // �������� ����������� ������ � ������� ��������
    LPVOID baseAddr = VirtualAllocEx(handleToProc, NULL, dllLength, MEM_DECOMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!baseAddr) {
        std::cout << "Unable to allocate memory in the target process.\n";
        CloseHandle(handleToProc);
        return false;
    }

    // ���������� ���� � DLL � ���������� ����������� ������
    if (!WriteProcessMemory(handleToProc, baseAddr, dll, dllLength, NULL)) {
        std::cout << "Unable to write to target process memory.\n";
        VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
        CloseHandle(handleToProc);
        return false;
    }

    // ������� ��������� �����, ������� LoadLibraryA � ����� � DLL � �������� ���������
    HANDLE remThread = CreateRemoteThread(handleToProc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, baseAddr, 0, NULL);
    if (!remThread) {
        std::cout << "Unable to create a remote thread.\n";
        VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
        CloseHandle(handleToProc);
        return false;
    }

    // ���� ���������� ���������� ������
    WaitForSingleObject(remThread, INFINITE);

    // ����������� ���������� ����������� ������
    VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
    CloseHandle(remThread);
    CloseHandle(handleToProc);
    return true;
}

int main() {
    // ������������� ��������� ����������� ����
    SetConsoleTitle("Injector - Release");

    DWORD PID = -1;
    char dll[255];

    // �������� ���� � DLL � PID �� ������������
    getDLLpath(dll);
    getPID(PID);

    // ��������� �������� DLL
    if (injectDLL(PID, dll)) {
        std::cout << "DLL injected successfully.\n";
    }
    else {
        std::cout << "DLL injection failed.\n";
    }

    system("Pause");
    return 0;
}
