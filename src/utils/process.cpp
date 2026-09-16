#include "process.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <signal.h>
#endif

bool ProcessManager::startProcess(const std::string &executable, const std::vector<std::string> &args, int &pid) {
#ifdef _WIN32
    // Windows 进程启动
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    
    si.cb = sizeof(si);
    
    std::string cmdLine = executable;
    for (const auto &arg : args) {
        cmdLine += " " + arg;
    }
    
    if (CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        pid = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
#else
    // Unix/Linux 进程启动
    pid = fork();
    if (pid == 0) {
        // 子进程
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto &arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execvp(executable.c_str(), argv.data());
        exit(1);
    } else if (pid > 0) {
        return true;
    }
    return false;
#endif
}

bool ProcessManager::terminateProcess(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == nullptr) return false;
    
    bool result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
#else
    return kill(pid, SIGTERM) == 0;
#endif
}

bool ProcessManager::isProcessRunning(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess == nullptr) return false;
    
    DWORD exitCode;
    bool result = GetExitCodeProcess(hProcess, &exitCode) && (exitCode == STILL_ACTIVE);
    CloseHandle(hProcess);
    return result;
#else
    return kill(pid, 0) == 0;
#endif
}
