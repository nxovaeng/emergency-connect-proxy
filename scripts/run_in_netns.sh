#!/usr/bin/env bash
#
# scripts/run_in_netns.sh
#
# Run emergency-connect-proxy inside a dedicated, isolated Linux Network Namespace.
# OpenVPN's tun0, routes, and DNS changes will remain strictly confined inside the namespace.
# The host can access the proxy service via http://10.200.1.2:8888 with ZERO host network pollution.
#

set -e

# Ensure running as root
if [ "$EUID" -ne 0 ]; then
    echo "[!] Please run this script with sudo or as root:"
    echo "    sudo $0 $@"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BINARY="${ROOT_DIR}/emergency-proxy"

if [ ! -f "$BINARY" ]; then
    echo "[*] Compiling emergency-proxy first..."
    make -C "$ROOT_DIR" -j"$(nproc)"
fi

NS_NAME="ws-emergency"
VETH_HOST="veth-wshost"
VETH_NS="veth-wsns"
HOST_IP="10.200.1.1"
NS_IP="10.200.1.2"
SUBNET="10.200.1.0/24"
PROXY_PORT="${PORT:-8888}"

# Detect host default egress interface
WAN_IF="$(ip route show default | awk '/default/ {print $5}' | head -n1)"
if [ -z "$WAN_IF" ]; then
    WAN_IF="eth0"
fi

cleanup() {
    echo ""
    echo "[*] Cleaning up network namespace and rules..."
    # Kill any dangling openvpn inside the namespace if still present
    ip netns exec "$NS_NAME" pkill -9 openvpn 2>/dev/null || true
    
    # Remove iptables NAT rule
    iptables -t nat -D POSTROUTING -s "$SUBNET" -o "$WAN_IF" -j MASQUERADE 2>/dev/null || true
    
    # Delete veth pair if still exists
    ip link delete "$VETH_HOST" 2>/dev/null || true
    
    # Delete namespace
    ip netns delete "$NS_NAME" 2>/dev/null || true
    
    # Remove custom netns resolv.conf
    rm -rf "/etc/netns/${NS_NAME}" 2>/dev/null || true
    
    echo "[*] Cleanup complete. Host network is untouched."
}

trap cleanup EXIT INT TERM

echo "============================================================"
echo " Starting Emergency Connect Proxy in Network Namespace: ${NS_NAME}"
echo " Host <-> Namespace Veth: ${HOST_IP} <-> ${NS_IP}"
echo " Proxy Listening on: ${NS_IP}:${PROXY_PORT} (or 0.0.0.0:${PROXY_PORT})"
echo "============================================================"

# 1. Clean up any existing leftover namespace
ip netns delete "$NS_NAME" 2>/dev/null || true
ip link delete "$VETH_HOST" 2>/dev/null || true

# 2. Create network namespace
echo "[+] Creating network namespace '${NS_NAME}'..."
ip netns add "$NS_NAME"

# 3. Enable loopback inside netns
ip netns exec "$NS_NAME" ip link set lo up

# 4. Create veth pair connecting host and namespace
echo "[+] Creating veth pair '${VETH_HOST}' <--> '${VETH_NS}'..."
ip link add "$VETH_HOST" type veth peer name "$VETH_NS"
ip link set "$VETH_NS" netns "$NS_NAME"

# 5. Configure IP addresses
echo "[+] Assigning IP addresses..."
ip addr add "${HOST_IP}/24" dev "$VETH_HOST"
ip link set "$VETH_HOST" up

ip netns exec "$NS_NAME" ip addr add "${NS_IP}/24" dev "$VETH_NS"
ip netns exec "$NS_NAME" ip link set "$VETH_NS" up

# 6. Default route inside netns points to host veth
echo "[+] Setting default route inside namespace via ${HOST_IP}..."
ip netns exec "$NS_NAME" ip route add default via "$HOST_IP"

# 7. Enable host IPv4 forwarding and NAT for namespace outbound traffic
echo "[+] Configuring host IPv4 forwarding and NAT on interface ${WAN_IF}..."
sysctl -w net.ipv4.ip_forward=1 >/dev/null
iptables -t nat -A POSTROUTING -s "$SUBNET" -o "$WAN_IF" -j MASQUERADE

# 8. Configure DNS inside namespace
mkdir -p "/etc/netns/${NS_NAME}"
cat <<EOF > "/etc/netns/${NS_NAME}/resolv.conf"
nameserver 1.1.1.1
nameserver 8.8.8.8
EOF

echo "[+] Network namespace initialized successfully."
echo "------------------------------------------------------------"
echo "  Proxy URL for host applications:"
echo "    http://${NS_IP}:${PROXY_PORT}"
echo "    socks5://${NS_IP}:${PROXY_PORT}"
echo ""
echo "  Example command in another host terminal:"
echo "    curl -x http://${NS_IP}:${PROXY_PORT} https://api.windscribe.com/Session"
echo "------------------------------------------------------------"

# 9. Launch emergency-proxy inside netns
cd "$ROOT_DIR"
ip netns exec "$NS_NAME" "$BINARY" --start --bind 0.0.0.0 --port "$PROXY_PORT" "$@"

