#include <iostream>
#include <Windows.h>

// Функция для получения данных от пользователя
template <typename T>
void getUserInput(const char* prompt, T& value) {
    std::cout << prompt << ": ";
    std::cin >> value;
}

// Функция для получения пути к DLL
void getDLLpath(char* dll) {
    getUserInput("Please enter the path to DLL-file", dll);
}

// Функция для получения PID целевого процесса
void getPID(DWORD& PID) {
    getUserInput("Enter the PID of your target process", PID);
}

// Функция для открытия обработчика целевого процесса
HANDLE openProcess(DWORD pid) {
    HANDLE handleToProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    return handleToProc;
}

// Функция инъекции DLL в целевой процесс
bool injectDLL(DWORD PID, const char* dll) {
    // Открываем обработчик целевого процесса
    HANDLE handleToProc = openProcess(PID);
    if (!handleToProc) {
        std::cout << "Unable to open process.\n";
        return false;
    }

    // Получаем адрес функции LoadLibraryA из kernel32.dll
    LPVOID LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!LoadLibAddr) {
        std::cout << "Unable to get address of LoadLibraryA.\n";
        CloseHandle(handleToProc);
        return false;
    }

    // Получаем длину пути к DLL
    int dllLength = strlen(dll) + 1;
    // Выделяем виртуальную память в целевом процессе
    LPVOID baseAddr = VirtualAllocEx(handleToProc, NULL, dllLength, MEM_DECOMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!baseAddr) {
        std::cout << "Unable to allocate memory in the target process.\n";
        CloseHandle(handleToProc);
        return false;
    }

    // Записываем путь к DLL в выделенную виртуальную память
    if (!WriteProcessMemory(handleToProc, baseAddr, dll, dllLength, NULL)) {
        std::cout << "Unable to write to target process memory.\n";
        VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
        CloseHandle(handleToProc);
        return false;
    }

    // Создаем удаленный поток, вызывая LoadLibraryA с путем к DLL в качестве параметра
    HANDLE remThread = CreateRemoteThread(handleToProc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, baseAddr, 0, NULL);
    if (!remThread) {
        std::cout << "Unable to create a remote thread.\n";
        VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
        CloseHandle(handleToProc);
        return false;
    }

    // Ждем завершения удаленного потока
    WaitForSingleObject(remThread, INFINITE);

    // Освобождаем выделенную виртуальную память
    VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);
    CloseHandle(remThread);
    CloseHandle(handleToProc);
    return true;
}

int main() {
    // Устанавливаем заголовок консольного окна
    SetConsoleTitle("Injector - Release");

    DWORD PID = -1;
    char dll[255];

    // Получаем путь к DLL и PID от пользователя
    getDLLpath(dll);
    getPID(PID);

    // Выполняем инъекцию DLL
    if (injectDLL(PID, dll)) {
        std::cout << "DLL injected successfully.\n";
    }
    else {
        std::cout << "DLL injection failed.\n";
    }

    system("Pause");
    return 0;
}
