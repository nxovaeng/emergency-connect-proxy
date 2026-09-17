# Emergency Connect Proxy for Windscribe

一个独立的 HTTP/SOCKS5 代理工具，帮助在速率限制环境下登录 Windscribe 客户端。

## 🎯 功能特性

- ✅ 基于 Windscribe 紧急连接技术与 WSNet 网络子系统
- ✅ 自动获取与解析远端端点信息 (模拟 Windscribe Desktop App 尝试策略)
- ✅ 建立 OpenVPN 隧道代理
- ✅ 跨平台支持 (Windows/macOS/Linux)
- ✅ HTTP 和 SOCKS5 协议支持
- ✅ 自动端点故障转移


## 📋 使用场景

当你在某些限制网络环境中：
1. Windscribe 账户触发速率限制 (rate limited)
2. 无法直接登录客户端

**解决方案：**
1. 启动此代理工具 → 建立紧急 VPN 隧道
2. 配置系统代理或应用代理
3. 通过新 IP 登录 Windscribe 客户端
4. 登录成功后关闭代理

## 🚀 快速开始

### Linux/macOS
```bash
# 编译
mkdir build && cd build
cmake ..
make

# 启动代理 (需要 sudo)
sudo ./emergency-proxy --start --port 8888

# 在另一个终端配置系统代理
export http_proxy=http://127.0.0.1:8888
export https_proxy=http://127.0.0.1:8888

# 运行 Windscribe 客户端登录
./windscribe

# 登录完成后停止代理
sudo ./emergency-proxy --stop
```

### Windows
```bash
# 使用 Visual Studio 编译
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# 以管理员身份运行
emergency-proxy.exe --start --port 8888

# 在系统设置中配置代理
# 设置 → 网络和 Internet → 代理
# 代理地址: 127.0.0.1:8888

# 运行 Windscribe 登录
# 登录后停止代理
emergency-proxy.exe --stop
```

### 🤖 GitHub 自动构建与跨平台矩阵

本项目支持 GitHub Actions 自动交叉编译与打包，支持手动一键触发（`workflow_dispatch`）或推送标签触发：

| 平台 | 架构 | 构建工具链 | 输出包格式 |
| :--- | :--- | :--- | :--- |
| **Linux** | `x86_64` (amd64) | GCC / CMake (原生) | `.tar.gz` |
| **Linux** | `arm64` (aarch64) | GCC aarch64 交叉编译 | `.tar.gz` |
| **Windows** | `x86_64` (x64) | MSVC / CMake (原生) | `.zip` |
| **Windows** | `arm64` (ARM64) | MSVC ARM64 交叉编译 | `.zip` |

**手动触发方式**：
1. 打开 GitHub 仓库页面，点击 **Actions** 标签页；
2. 在左侧选择 **Build and Package** 工作流；
3. 点击右侧 **Run workflow** 下拉菜单，选择构建分支、类型（`Release`/`Debug`），即可一键构建 4 种架构的二进制产物并生成 SHA256 校验和。

## 📖 命令行用法

```bash
# 启动代理
./emergency-proxy --start [--port 8888] [--bind 127.0.0.1]

# 指定应急/VPN 认证账号密码 (或通过环境变量 EMERGENCY_USER / EMERGENCY_PASS 传入)
./emergency-proxy --start --username <username> --password <password>

# 独立拉取并查看远端紧急端点列表 (通过 wsnet)
./emergency-proxy --fetch-endpoints

# 运行单元测试
make test

# 停止代理
./emergency-proxy --stop

# 查看状态
./emergency-proxy --status

# 使用配置文件
./emergency-proxy --start --config config/default.conf

# 显示帮助
./emergency-proxy --help
```

### 🔒 Linux 网络命名空间隔离运行（推荐）

通过 Linux Network Namespace 可以实现 **零路由污染、零网卡侵入**：
OpenVPN 的 `tun0` 网卡及默认路由被严格隔离在独立的网络空间中，宿主机完全不受影响，只需通过本地虚拟端口连接代理：

```bash
# 一键隔离运行（自动配置 veth、NAT 与 DNS，退出时自动清理）
sudo ./scripts/run_in_netns.sh

# 宿主机使用代理访问 Windscribe API：
curl -x http://10.200.1.2:8888 https://api.windscribe.com/Session
```


## ⚙️ 配置文件

编辑 `config/default.conf`:

```json
{
  "proxy": {
    "port": 8888,
    "bindAddress": "127.0.0.1",
    "enableSocks5": true,
    "enableHttp": true
  },
  "openvpn": {
    "timeout": 30,
    "retries": 3
  },
  "endpoints": [
    {"ip": "1.1.1.1", "port": 1194, "protocol": "udp"},
    {"ip": "2.2.2.2", "port": 443, "protocol": "tcp"}
  ],
  "logging": {
    "level": "info",
    "file": "emergency-proxy.log"
  }
}
```

## 🔧 编译要求

### Ubuntu/Debian
```bash
sudo apt-get install -y \
  build-essential \
  cmake \
  libssl-dev \
  openvpn
```

### macOS
```bash
brew install cmake openssl openvpn
```

### Windows
- Visual Studio 2019+ 或 MinGW
- CMake 3.12+
- OpenSSL (vcpkg 或预编译)
- OpenVPN 客户端

## 📁 项目结构

```
emergency-connect-proxy/
├── src/
│   ├── main.cpp                    # CLI 入口
│   ├── emergency_proxy.h/cpp       # 代理核心
│   ├── openvpn_tunnel.h/cpp        # OpenVPN 隧道
│   ├── proxy_server.h/cpp          # HTTP/SOCKS5 代理服务器
│   └── utils/
│       ├── config.h/cpp            # 配置文件解析
│       ├── logger.h/cpp            # 日志系统
│       ├── process.h/cpp           # 进程管理
│       ├── network.h/cpp           # 网络工具
│       └── string_utils.h/cpp      # 字符串工具
├── config/
│   ├── default.conf                # 默认配置
│   └── endpoints.json              # 端点列表
├── CMakeLists.txt                  # 编译配置
├── README.md
├── BUILD.md                        # 编译指南
└── LICENSE                         # MIT License
```

## 🔐 安全说明

- 代理进程需要管理员/root 权限 (配置 OpenVPN)
- 所有流量通过加密的 OpenVPN 隧道
- 凭证不存储在日志中
- 配置文件权限: 600 (仅 owner 可读)

## 🐛 故障排查

### 代理启动失败
```bash
# 检查端口是否被占用
sudo netstat -tlnp | grep 8888

# 使用其他端口
./emergency-proxy --start --port 9999
```

### OpenVPN 连接失败
```bash
# 查看详细日志
./emergency-proxy --start --log-level debug

# 验证 OpenVPN 是否安装
openvpn --version
```

### DNS 不工作
- 确保 DNS 通过 VPN 隧道转发
- 检查系统代理配置

## 📝 日志位置

- Linux/macOS: `/tmp/emergency-proxy.log`
- Windows: `%TEMP%\emergency-proxy.log`

使用 `--log-file` 指定自定义位置

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

MIT License - 见 LICENSE 文件

## ⚠️ 免责声明

本工具仅供学习和个人使用。用户需自行承担使用本工具的法律责任。

---

**相关链接：**
- [Windscribe 官网](https://windscribe.com)
- [OpenVPN 项目](https://openvpn.net)
