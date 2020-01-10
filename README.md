# net_socket
---

## Introduction
A C++ class that provides a basic socket interface similar to the socket API. Each instance corresponds to a single socket endpoint and can perform the traditional socket operations (i.e., connect, accept, send, recv) but use C++ data types in addition to C data types. `send` and `recv` operate on `std::vector<char>` and `std::string` in addition to the standard socket API data type (`void*`).

Sockets may be manually `close`d or the destructor will close the socket, if open.

Only TCP sockets are supported at this time.

### Simple Server
A server (passively) opens a socket with a call to `listen`, then `accept`s connections from clients, and calls `send` and `recv` based on application requirements.

### Simple Client
A client opens a socket by `connect`ing to a server, then calls `send` and `recv` based on application requirements.

## TODO
- UDP support
