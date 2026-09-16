#pragma once

#include <string>
#include <vector>

class ProcessManager {
public:
    // 启动进程
    static bool startProcess(const std::string &executable, const std::vector<std::string> &args, int &pid);
    
    // 终止进程
    static bool terminateProcess(int pid);
    
    // 检查进程是否运行
    static bool isProcessRunning(int pid);
};
