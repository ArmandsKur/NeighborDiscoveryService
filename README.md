# NeighborDiscoveryService
A neighbor discovery service that uses custom L2 protocol to detect and track neighbors across multiple separate local networks.

## Build
```
# clone the repo
git clone https://github.com/ArmandsKur/NeighborDiscoveryService.git
# go to creadted repo dir
cd NeighborDiscoveryService
# create and open 'build' directory
mkdir build && cd build
# build the project.
# Note: running make will prompt your password to set CAP_NET_RAW capability
cmake ..
make
# run the NeighborDiscoveryService
./NeighborDiscoveryService
# in separate terminal run CLI client
./NeighborCli
```

## Overview
**NeighborDiscoveryService** is background service that broadcasts presence information and listens for broadcasts from other neighbors across local network. It supports multiple network interfaces, both IPv4/IPv6 addresses or interfaces without IP address, and provides real-time updates as interface state changes.

**NeighborCli** is command-line client that queries the service and displays all neighbors seen within last 30 seconds.

## Architecture
### Components
1. EventPoll: Main event loop using poll() to orchestrate all socket I/O.
2. InterfaceManager: Netlink-based interface monitoring.
3. NeighborManage: Custom Ethernet protocol implementation for neighbor discovery.
4. ClientManager: Unix domain socket server for CLI communication.

### Protocol design
The service uses a custom Ethernet protocol (using EtherType 0x88B5 Local experimental) which gets broadcasted each 5 seconds:
```
EthernetFrame:
[EthernetHeader: 14 bytes]
[Neighbor Payload: ]
 - client_id: 16 bytes (random, generated using getentropy)
 - mac_addr: 6 bytes
 - ip_family: 1 byte (AF_INET/AF_INET6/0)
 - union of in_addr and in6_addr: 16 bytes
```
### Key design decisions

## Testing
### Test environment
- VM Manager: UTM
- Setup: 3 VMs, 2 networks
  - VM1: Connected to both networks (using interfaces enp0s1 and enp0s2)
  - VM2: Connected to both networks (using interfaces enp0s1 and enp0s2)
  - VM3: Connected to one network only (using enp0s1)

### Tests performed
#### Test1:Standard operation
**Setup:** All 3 VMs running with all IP addresses on all interfaces.  
**Result:** CLI correctly shows neighbor connections on appropriate local interfaces.
#### Test2: IP address flush
**Setup:** Started with all IPs assigned, then flushed all IPs from enp0s2 on VM1.  
**Result:**
- Service does not get disrupted, broadcasts still sent and received using enp0s2.
- VM2 still sees this connection but IP is marked as None.
#### Test3: Interface UP/DOWN events
**Setup:** During standard service run disabled enp0s1 interface on VM2.  
**Result:**
- Neighbors continue to show VM2 for 30 seconds.
- After 30s VM1 shows only enp0s2 connection to VM2.
- VM3 loses track of VM2 entirely (it's only interface is enp0s1).
- Starting up the interface on VM2 immediately restores discovery.
#### Test: Starting service with disabled interface
**Setup:** Service starts with any interface disabled.  
**Result:** Netlink events trigger immediate broadcast sending/receiving on newly enabled interfaces
#### Test: IPv4 priority
**Setup:** VM2 starts with enp0s2 interface having both IPv4 and IPv6. Interface IPv4 gets disabled and enabled, right after each activity CLI is ran.  
**Result:** At start for enp0s2 IPv4 is displayed. After IPv4 is disabled IPv6 is displayed instead. When IPv4 is re-enabled, IPv4 again starts showing instead of IPv6.

