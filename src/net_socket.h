//--------------------------------------------------------------------------------------------------
/// \class network_socket::net_socket src/net_socket.h
/// \brief
/// C++ network socket class that mimics the socket API with support for some STL classes.
///
/// \section Attributes
/// There are several attributes of the socket that you can control, including the:
/// - network-layer protocol
/// - transport-layer protocol
/// - number of pending connections to backlog (only for server applications)
/// - timeout value, which causes recv to give up after the specified time
/// - default receive size to use when a recv call does not explicity or implicity specify it
///
/// Additionally, you can query the socket status for:
/// - is it connected?
/// - is the socket opened passively (listen was called)?
/// - the socket file descriptor, which is -1 if the socket is closed
///
/// \section Examples
/// A client application (using strings) might look like:
/// \code
/// net_socket client;
/// string msg("here's your data");
/// client.connect("hostname", 9000); // Connect to a host with name "hostname" on port 9000.
/// client.send(msg);                 // Send the string to the server.
/// msg.clear();                      // Clear the string to receive a large chunk (see below).
/// client.recv(msg);                 // Receive some data from the server.
/// \endcode
///
/// A server application (using vectors) might look like:
/// \code
/// net_socket server;
/// vector<char> data;
/// server.listen("", 9000);          // Listen on port 9000 on any interface.
/// unique_ptr<net_socket> worker =
///     server.accept();              // Get a new socket connected to a connection.
/// worker->recv(data);               // Receive some data from the client.
/// data.insert(data.begin(), 'K');   // Prepend some data
/// data.insert(data.begin(), 'O');
/// worker->send(data);               // Send a message to the client
/// \endcode
///
/// \section Sending and Receiving
/// Send and receive functions return the amount of bytes actually sent or received. The _all
/// version of send and recv will repeatedly perform the operation until all the data requested has
/// been processed. The standard version will make only one attempt.
///
/// The amount of data to send or receive is determined by the:
///  -# second argument when present and when not zero
///  -# size of the string or vector, as determined by size(), when it's not empty
///  -# receive size set in the socket
///
/// If you pass a vector or string into recv or recv_all with a small size, only that many bytes
/// will be received. If you want to use the socket's receive size, use clear() first or specify
/// the size.
///
//--------------------------------------------------------------------------------------------------

#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <string>
#include <memory>
#include <vector>

namespace network_socket {

/// An exception thrown when data does not arrive before a timeout value, if one is set.
class timeout_exception : public std::runtime_error {
public:
	timeout_exception() : runtime_error("TIMEOUT!") {};
};

class net_socket{
public:

	/// enum for possible network-layer protocols.
	enum network_protocol {ANY, IPv4, IPv6};
	/// enum for possible transport-layer protocols.
	enum transport_protocol {UDP, TCP};

	explicit net_socket(
		network_protocol net = network_protocol::ANY,
		transport_protocol tran = transport_protocol::TCP
		);
	/// Copy constructor
	///
	/// Assigning from or to a net_socket that is open (is_connected() or is_passively_opened() are
	/// true) causes an exception.
	net_socket(const net_socket &other);
	net_socket(net_socket&& other) noexcept;

	/// Assignment operator
	///
	/// Assigning from or to a net_socket that is open (is_connected() or is_passively_opened() are
	/// true) causes an exception.
	net_socket& operator=(const net_socket& rhs);
	net_socket& operator=(net_socket&& rhs) noexcept;

	/// Destructor
	///
	/// The destructor closes the socket and releases resources to the OS.
	~net_socket() noexcept;

	// Getter and setter members
	int get_socket_descriptor() const {return _sock_desc;}
	network_protocol get_network_protocol() const {return _net_proto;}
	void set_network_protocol(network_protocol np);
	transport_protocol get_transport_protocol() const {return _trans_proto;}
	void set_transport_protocol(transport_protocol tp);
	bool is_passively_opened() const {return _passive;}
	int get_backlog() const {return _backlog;}
	/// Set the pending connection backlog.
	///
	/// Argument must be non-negative.
	void set_backlog(int backlog);
	bool is_connected() const {return _connected;}
	bool timeout_is_set() const {return _do_timeout;}
	/// Get the current timeout interval.
	///
	/// Returns 0 if timeouts are disabled.
	double get_timeout() const;
	/// Set the timeout interval.
	///
	/// Setting the timeout to 0 disables timeout operation similar to clear_timeout() .
	void set_timeout(double s);
	/// Disable timeout operation.
	void clear_timeout() {_do_timeout = false;}
	size_t get_recv_size() const {return _recv_size;}
	void set_recv_size(size_t s) {_recv_size = s;}

	// Connection management functions.
	void listen(const std::string &host, const std::string &service);
	void listen(const std::string &host, unsigned short port);
	void listen(const std::string &service);
	void listen(unsigned short port);
	void connect(const std::string &host, const std::string &service);
	void connect(const std::string &host, unsigned short port);
	void close();

	/// Accept a new connection.
	///
	/// A new, connected socket is returned. The original socket remains passively opened for
	/// subsequent calls to accept. The returned socket is connected and ready to use.
	std::unique_ptr<net_socket> accept();

	ssize_t send(const void *data, size_t max_size) const;
	template<typename T> ssize_t send(const std::vector<T> &data, size_t max_size = 0) const;
	ssize_t send(const std::string &data, size_t max_size = 0) const;
	ssize_t send_all(const void *data, size_t exact_size) const;
	template<typename T> ssize_t send_all(const std::vector<T> &data) const;
	ssize_t send_all(const std::string &data) const;
	ssize_t recv(void *data, size_t max_size);
	template<typename T> ssize_t recv(std::vector<T> &data, size_t max_size = 0);
	ssize_t recv(std::string &data, size_t max_size = 0);
	ssize_t recv_all(void *data, size_t exact_size);
	template<typename T> ssize_t recv_all(std::vector<T> &data, size_t exact_size = 0);
	ssize_t recv_all(std::string &data, size_t exact_size = 0);

private:
	int _sock_desc{-1};
	network_protocol _net_proto{network_protocol::ANY};
	transport_protocol _trans_proto{transport_protocol::TCP};
	bool _passive{false};
	int _backlog{5};
	bool _connected{false};
	bool _do_timeout{false};
	struct timeval _timeout{};
	size_t _recv_size{1400};

	void copy(const net_socket *other = nullptr);
	void move(net_socket *other);
	int get_af() const;
	int get_socktype() const;
};

} // namespace network_socket

// send/recv template definitions
#include "net_socket_templates.h"

#endif
