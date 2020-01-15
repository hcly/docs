# android5.1 4G拔号路由问题
系统启动后有显示4G图标,同时ppp0拔号成功,但无法上网
```java
root@icoolv3:/ # netcfg                                                        
lo       UP                                   127.0.0.1/8   0x00000049 00:00:00:00:00:00
sit0     DOWN                                   0.0.0.0/0   0x00000080 00:00:00:00:00:00
eth0     DOWN                                   0.0.0.0/0   0x00001002 00:00:00:00:00:00
ppp0     UP                               10.64.219.137/32  0x000010d1 00:00:00:00:00:00
ip6tnl0  DOWN                                   0.0.0.0/0   0x00000080 00:00:00:00:00:00
```
通过指令端点可以ping通外网  
ping -I ppp0 www.baidu.com
```java
root@icoolv3:/ # ping -I ppp0 www.baidu.com
PING www.a.shifen.com (183.232.231.174) from 10.64.219.137 ppp0: 56(84) bytes of data.
64 bytes from 183.232.231.174: icmp_seq=1 ttl=56 time=41.2 ms
64 bytes from 183.232.231.174: icmp_seq=2 ttl=56 time=24.8 ms
64 bytes from 183.232.231.174: icmp_seq=3 ttl=56 time=37.0 ms
64 bytes from 183.232.231.174: icmp_seq=4 ttl=56 time=22.6 ms
```
从而可以判断应该是路由出了问题  
从logcat中可以看到一个错误提示:
```java
D/pppd    ( 1423): Script /etc/ppp/ip-up finished (pid 1508), status = 0x0
D/ConnectivityService(  397): registerNetworkAgent NetworkAgentInfo{ ni{[type: MOBILE[LTE], state: CONNECTED/CONNECTED, reason: connected, extra: cmnet, roaming: false, failover: false, isAvailable: true, isConnectedToProvisioningNetwork: false]}  network{null}  lp{{InterfaceName: ppp0 LinkAddresses: [10.6.182.2/32,]  Routes: [0.0.0.0/0 -> 10.64.64.64 ppp0,] DnsAddresses: [221.179.38.7,120.196.165.7,] Domains: null MTU: 1500 TcpBufferSizes: 524288,1048576,2097152,262144,524288,1048576}}  nc{[ Transports: CELLULAR Capabilities: SUPL&INTERNET&NOT_RESTRICTED&TRUSTED&NOT_VPN LinkUpBandwidth>=51200Kbps LinkDnBandwidth>=102400Kbps Specifier: <1>]}  Score{10}  everValidated{false}  lastValidated{false}  created{false}  explicitlySelected{false} }
D/ConnectivityService(  397): NetworkAgentInfo [MOBILE (LTE) - 100] EVENT_NETWORK_INFO_CHANGED, going from null to CONNECTED
D/ConnectivityService(  397): Adding iface ppp0 to network 100
D/ConnectivityService(  397): Setting MTU size: ppp0, 1500
D/ConnectivityService(  397): Adding Route [0.0.0.0/0 -> 10.64.64.64 ppp0] to network 100
E/Netd    (  111): netlink response contains error (No such process)
E/ConnectivityService(  397): Exception in addRoute for gateway: java.lang.IllegalStateException: command '17 network route add 100 ppp0 0.0.0.0/0 10.64.64.64' failed with '400 17 addRoute() failed (No such process)'
D/ConnectivityService(  397): Setting Dns servers for network 100 to [/221.179.38.7, /120.196.165.7]
```
对比wifi连接,这中间少了一个操作
```java
D/ConnectivityService(  392): Adding iface wlan0 to network 100
E/WifiStateMachine(  392): Did not find remoteAddress {192.168.2.1} in /proc/net/arp
E/ConnectivityService(  392): Unexpected mtu value: 0, wlan0
D/ConnectivityService(  392): Adding Route [fe80::/64 -> :: wlan0] to network 100
E/WifiStateMachine(  392): WifiStateMachine CMD_START_SCAN source 1000 txSuccessRate=1.12 rxSuccessRate=0.00 targetRoamBSSID=34:ce:00:08:92:72 RSSI=-51
D/ConnectivityService(  392): Adding Route [192.168.2.0/24 -> 0.0.0.0 wlan0] to network 100
D/ConnectivityService(  392): Adding Route [0.0.0.0/0 -> 192.168.2.1 wlan0] to network 100
D/ConnectivityService(  392): Setting Dns servers for network 100 to [/192.168.2.1]
D/ConnectivityService(  392): notifyType IP_CHANGED for NetworkAgentInfo [WIFI () - 100]
D/ConnectivityService(  392): notifyType PRECHECK for NetworkAgentInfo [WIFI () - 100]
D/ConnectivityService(  392): rematching NetworkAgentInfo [WIFI () - 100]
```
手动添加路由相关设置
```java
ip route add 10.64.64.64 dev ppp0 table ppp0
ip route add default via 10.64.64.64 dev ppp0 table ppp0
```
执行上面两条指令后就能正常上网
参考相关说明修改frameworks/base/services/core/java/com/android/server/ConnectivityService.java
```java
    private boolean updateRoutes(LinkProperties newLp, LinkProperties oldLp, int netId) {
        CompareResult<RouteInfo> routeDiff = new CompareResult<RouteInfo>();
        if (oldLp != null) {
            routeDiff = oldLp.compareAllRoutes(newLp);
        } else if (newLp != null) {
            routeDiff.added = newLp.getAllRoutes();
        }

        // add routes before removing old in case it helps with continuous connectivity

        // do this twice, adding non-nexthop routes first, then routes they are dependent on
        for (RouteInfo route : routeDiff.added) {
            if (route.hasGateway()) continue;
            if (DBG) log("Adding Route [" + route + "] to network " + netId);
            try {
                mNetd.addRoute(netId, route);
            } catch (Exception e) {
                if ((route.getDestination().getAddress() instanceof Inet4Address) || VDBG) {
                    loge("Exception in addRoute for non-gateway: " + e);
                }
            }
        }
        for (RouteInfo route : routeDiff.added) {
            if (route.hasGateway() == false) continue;
            if (DBG) log("Adding Route [" + route + "] to network " + netId);
            try {
				//add by hclydao
				if(route.getInterface().equals("ppp0")) {
					RouteInfo xroute = RouteInfo.makeHostRoute(route.getGateway(), route.getInterface());//mkae host route for nexthop
					mNetd.addRoute(netId, xroute);//add nexthop(getGateway()) for table ppp0
					if (DBG) log("Adding Route [" + xroute + "] to network " + netId + " for ppp0");
				}
				//add end
                mNetd.addRoute(netId, route);
            } catch (Exception e) {
                if ((route.getGateway() instanceof Inet4Address) || VDBG) {
                    loge("Exception in addRoute for gateway: " + e);
                }
            }
        }

        for (RouteInfo route : routeDiff.removed) {
            if (DBG) log("Removing Route [" + route + "] from network " + netId);
            try {
                mNetd.removeRoute(netId, route);
            } catch (Exception e) {
                loge("Exception in removeRoute: " + e);
            }
        }
        return !routeDiff.added.isEmpty() || !routeDiff.removed.isEmpty();
    }
```
参考链接[https://blog.csdn.net/u012246195/article/details/53427111](https://blog.csdn.net/u012246195/article/details/53427111)
