# net_socket #

# Introduction

A C++ class that provides a basic socket interface similar to the socket API.
Each instance corresponds to a single socket endpoint and can perform the
traditional socket operations (i.e., connect, accept, send, recv) but uses C++
data types `std::string` and `std::vector` in addition to C data types.

Sockets may be manually `close`d or the destructor will close the socket, if
left open.

Only TCP sockets are supported at this time.

## Attributes

There are several attributes of the socket that you can control, including the:

* network-layer protocol
* transport-layer protocol
* number of pending connections to backlog (only for server applications)
* timeout value, which causes recv to give up after the specified time
* default receive size to use when a recv call does not explicitly or
implicitly specify it

Additionally, you can query the socket status for:

* is it connected?
* is the socket opened passively (listen was called)?
* the socket file descriptor, which is -1 if the socket is closed

## `send`ing and `recv`ing

`send` and `recv` functions return the amount of bytes actually sent or
received. The \_all version of send and recv will repeatedly perform the
operation until all the data requested has been processed. The standard version
will make only one attempt.

The amount of data to send or receive is determined by the:
 -# second argument when present and when not zero
 -# size of the string or vector, as determined by size(), when it's not empty
 -# receive size set in the socket

If you pass a vector or string into recv or recv_all with a small size, only
that many bytes will be received. If you want to use the socket's receive size,
use clear() first or specify the size.

packet_error_send functions emulate packet losses in the network by randomly
failing to send the requested data, but returning a non-error return value.

# Examples

A client application (using strings) might look like:

```
net_socket client;
string msg("here's your data");
client.connect("hostname", 9000); // Connect to a host with name "hostname"
                                  // on port 9000.
client.send(msg);                 // Send the string to the server.
msg.clear();                      // Clear the string to receive a large
                                  // chunk (see below).
client.recv(msg);                 // Receive some data from the server.
```

A server application (using vectors) might look like:

```
net_socket server;
vector<char> data;
server.listen("", 9000);          // Listen on port 9000 on any interface.
unique_ptr<net_socket> worker =
    server.accept();              // Get a new socket connected to a client.
worker->recv(data);               // Receive some data from the client.
data.insert(data.begin(), 'K');   // Prepend some data
data.insert(data.begin(), 'O');
worker->send(data);               // Send a message to the client
```
