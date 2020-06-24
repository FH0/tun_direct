allow_ip="0.0.0.0/8,100.64.0.0/10,127.0.0.0/8,169.254.0.0/16,192.0.0.0/24,192.0.2.0/24,192.88.99.0/24,198.18.0.0/15,198.51.100.0/24,203.0.113.0/24,172.16.0.0/12,192.168.0.0/16,10.0.0.0/8,224.0.0.0/3"

ip route add default dev tun3 table 1112
ip rule add fwmark 0x1112 lookup 1112

iptables -t mangle -A OUTPUT -p tcp -m owner --uid-owner nobody -j MARK --set-xmark 0x1112
iptables -t mangle -A OUTPUT -p udp -m owner --uid-owner nobody -j MARK --set-xmark 0x1112

iptables -t mangle -A OUTPUT -p tcp -m owner --uid-owner 10152 -j MARK --set-xmark 0x1112
iptables -t mangle -A OUTPUT -p udp -m owner --uid-owner 10152 -j MARK --set-xmark 0x1112

ip -6 route add default dev tun3 table 1112
ip -6 rule add fwmark 0x1112 lookup 1112

ip6tables -t mangle -A OUTPUT -p tcp -m owner --uid-owner nobody -j MARK --set-xmark 0x1112
ip6tables -t mangle -A OUTPUT -p udp -m owner --uid-owner nobody -j MARK --set-xmark 0x1112

ip6tables -t mangle -A OUTPUT -p tcp -m owner --uid-owner 10152 -j MARK --set-xmark 0x1112
ip6tables -t mangle -A OUTPUT -p udp -m owner --uid-owner 10152 -j MARK --set-xmark 0x1112

ip6tables -t mangle -A PREROUTING -p tcp -d $allow_ip -j ACCEPT
ip6tables -t mangle -A PREROUTING -p tcp -j MARK --set-xmark 0x1112
ip6tables -t mangle -A PREROUTING -p udp -d $allow_ip -j ACCEPT
ip6tables -t mangle -A PREROUTING -p udp -j MARK --set-xmark 0x1112
