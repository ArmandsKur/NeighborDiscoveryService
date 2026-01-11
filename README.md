# NeighborDiscoveryService
A nighbor discovery service that uses custom L2 protocol to detect and track neighbors across multiple separate local networks.

## Overview
**NeigborDiscoveryService** is background service that broadcasts presence information and listens for broadcasts from other neighbors across local network. It supports multiple network intefracs, both IPv4/IPv6 addresses or interfaces without IP address, and provides real-time updates as interface state changes.

**NeighborCli** is command-line client that queries the service and displays all neighbors seen within last 30 seconds.
## Features
- Multi-interface support 

## Architecture
### Components
1. EventPoll: Main event loop using poll() to orchestrate all socket I/O.
2. InterfaceManager: Netlink-based interface monitoring.
3. NeighborManage: Custom Ethernet protocol implementation for neighbor discovery.
4. ClientManager: Unix domain socket server for CLI communication.

### Protocol design
The service uses a custom Ethernet protocol (EtherType 0x88B5 Local experimental) which gets broadcasted each 5 seconds:
```
EthernetFrame:
[EthernetHeader: 14 bytes]
[Neighbor Payload: ]
 - client_id: 16 bytes (random, generated using getentropy)
 - mac_addr: 6 bytes
 - ip_family: 1 byte (AF_INET/AF_INET6/0)
 - union of in_addr and in6_addr: 16 bytes
```
