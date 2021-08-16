#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <string>
#include <memory>
#include <vector>
#include <random>
#include <stdexcept>
#include <netinet/ip.h>

namespace network_socket {

/// \brief An exception thrown when data does not arrive before a timeout
/// value.
///
/// The exception is thrown when attempting to recv data and does not arrive
/// before the timeout expires and if the timeout is set. If some data is
/// received before the timeout, the number of bytes is stored; this is used by
/// the recv_all functions.

class timeout_exception : public std::runtime_error {
public:
	timeout_exception() : runtime_error("TIMEOUT!") {};
	timeout_exception(ssize_t s) : runtime_error("TIMEOUT!"),
		_partial_data_size(s) {};
	ssize_t get_partial_data_size() const {return _partial_data_size;}
	void set_partial_data_size(ssize_t s) {_partial_data_size = s;}
private:
	ssize_t _partial_data_size {0};
};

/// \brief A class that abstracts various sockaddr structures.
///
/// It provides easy conversion from sockaddr structures for both IPv4 and IPv6
/// families. The class also stores the port number and provides an output
/// operator for use with std::ostreams.

class address {
public:
	address();
	address(const struct sockaddr&);
	address(const struct sockaddr_storage&);
	address(const struct sockaddr_in&);
	address(const struct sockaddr_in6&);
	address(const address&);
	address& operator=(const address&);
	address& operator=(const struct sockaddr&);
	address& operator=(const struct sockaddr_storage&);
	address& operator=(const struct sockaddr_in&);
	address& operator=(const struct sockaddr_in6&);

	/// \brief Test if the address is an IPv4 address.
	/// \retval True: the address is a valid IPv4 address.
	/// \retval False: the address is not a valid IPv4 address.
	bool is_ipv4() const;

	/// \brief Test if the address is an IPv6 address.
	/// \retval True: the address is a valid IPv6 address.
	/// \retval False: the address is not a valid IPv6 address.
	bool is_ipv6() const;

	/// \brief Retrieve the port number in *host* byte order.
	/// \return The port number in *host* byte order.
	in_port_t get_port() const;

	/// \brief Assign the port number.
	/// \param p Port number in *host* byte order.
	void set_port(in_port_t p);

	/// \brief Retrieve a string representation of the address.
	///
	/// The string has the following format for each family.
	///
	/// Family|Format|Example
	///  ---|---|---
	/// IPv4|address:port|192.168.1.25:22
	/// IPv6|[address]:port|[ffe0::1]:22
	///
	std::string str() const;

	/// Get the address represented as string.
	std::string get_address() const;

	/// \brief Assign the address from a string.
	///
	/// Convert from a string representation instead of a sockaddr. The string
	/// must have a format accepted by inet_pton. The port remains unchanged;
	/// all other fields are assigned zero.
	void set_address(const std::string&);

	/// \brief Retrieve the sockadder structure for the address.
	/// \return A sockaddr_storage structure to ensure it can contain an IPv6
	/// address.
	struct sockaddr_storage get_sockaddr() const {return addr;}

	friend bool operator==(const address&, const address&);
private:
	struct sockaddr_storage addr;
};
bool operator!=(const address&, const address&);

/// \brief C++ network socket class that mimics the socket API with support for
/// some STL classes.
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
	/// Assigning from or to a net_socket that is open (is_connected() or
	/// is_passively_opened() are true) causes an exception.
	net_socket(const net_socket &other);

	net_socket(net_socket&& other) noexcept;

	/// Assignment operator
	///
	/// Assigning from or to a net_socket that is open (is_connected() or
	/// is_passively_opened() are true) causes an exception.
	net_socket& operator=(const net_socket& rhs);

	net_socket& operator=(net_socket&& rhs) noexcept;

	/// Destructor
	///
	/// The destructor closes the socket and releases resources to the OS.
	~net_socket() noexcept;

	// Getter and setter members
	/// \brief Get the socket descriptor (file descriptor).
	/// \return -1 if the socket is closed.
	int get_socket_descriptor() const {return _sock_desc;}
	network_protocol get_network_protocol() const {return _net_proto;}
	void set_network_protocol(network_protocol np);
	transport_protocol get_transport_protocol() const {return _trans_proto;}
	void set_transport_protocol(transport_protocol tp);
	bool is_passively_opened() const {return _passive;}
	int get_backlog() const {return _backlog;}
	/// \brief Set the pending connection backlog.
	///
	/// \param backlog Must be non-negative.
	void set_backlog(int backlog);
	bool is_connected() const {return _connected;}
	bool timeout_is_set() const {return _do_timeout;}
	/// \brief Get the current timeout interval.
	/// \return 0 if timeouts are disabled.
	double get_timeout() const;
	/// \brief Set the timeout interval.
	///
	/// Setting the timeout to 0 disables timeout operation similar to
	/// clear_timeout().
	void set_timeout(double s);
	/// Disable timeout operation.
	void clear_timeout() {_do_timeout = false;}
	size_t get_default_recv_size() const {return _recv_size;}
	/// \brief Set the number of bytes to receive when not otherwise specified.
	///
	/// Receive functions that specify a size of zero (the default) will use
	/// this size to determine how many bytes to receive.
	void set_default_recv_size(size_t s) {_recv_size = s;}

	/// \brief Listen for connections on the specified interface and port or service
	/// name.
	void listen(const std::string &host, const std::string &service);
	/// Listen for connections on the specified interface and port.
	void listen(const std::string &host, unsigned short port);
	/// \brief Listen for connections on any interface on specified port or service
	/// name.
	void listen(const std::string &service);
	/// Listen for connections on any interface on specified port.
	void listen(unsigned short port);
	/// Connect to the specified host and port or service name.
	void connect(const std::string &host, const std::string &service);
	/// Connect to the specified host and port.
	void connect(const std::string &host, unsigned short port);
	/// Connect to the specified address.
	void connect(const address &addr);
	/// Close any active connections.
	void close();

	/// \brief Accept a new connection.
	///
	/// A new, connected socket is returned. The original socket remains
	/// passively opened for subsequent calls to accept. The returned socket is
	/// connected and ready to use.
	std::unique_ptr<net_socket> accept();

	/// \brief Get the local socket address (name) information.
	///
	/// An exception is thrown if the socket is not connected and not passively
	/// opened.
	address get_local_address() const;
	/// \brief Get the remote socket address (name) information.
	///
	/// An exception is thrown if the socket is not connected.
	address get_remote_address() const;

	/// \brief Send `max_size` bytes of data starting at memory location `data`.
	///
	/// A wrapper around the socket API `send` function. Throws an exception if
	/// the net_socket is not connected or upon error.
	ssize_t send(const void *data, size_t max_size) const;
	/// \brief Sends data from the vector.
	///
	/// Sends data in *network* byte order after conversion. Original object
	/// remains unchanged.
	template<typename T>
		ssize_t send(const std::vector<T> data, size_t max_size = 0) const;
	/// \brief Send the string data *and* a NULL.
	///
	/// The function always sends a NULL character, even if the string is
	/// empty (i.e., you can send the empty string). If `max_size` is zero
	/// (the default) then data.length()+1 bytes are sent. If `max_size` is
	/// larger than data.length(), then data.length()+1 bytes are sent.
	/// \return The actual number of bytes sent, including the NULL.
	ssize_t send(const std::string &data, size_t max_size = 0) const;

	/// \brief Sends the requested data, but sometimes doesn't.
	///
	/// `packet_error_send` randomly returns a non-zero value (indicating
	/// success, but doesn't actually send anything. The "drop" probability is
	/// determined by the _drop_rate parameter at compile time.
	ssize_t packet_error_send(const void *data, size_t max_size) const;
	/// \details See `send(std::vector)` and `packet_error_send(void*)`.
	template<typename T>
		ssize_t packet_error_send(const std::vector<T> data,
			size_t max_size = 0) const;
	/// \details See `send(std::string)` and `packet_error_send(void*)`.
	ssize_t packet_error_send(const std::string &data, size_t max_size = 0)
		const;

	/// \brief Attempt to send all the requested data.
	///
	/// Multiple calls to `send` may take place, if required.
	/// \return The actual number of bytes sent.
	ssize_t send_all(const void *data, size_t exact_size) const;
	/// \details See `send_all(void*)` and `send(std::vector)`.
	template<typename T>
		ssize_t send_all(const std::vector<T> data) const;
	/// \details See `send_all(void*)` and `send(std::string)`.
	ssize_t send_all(const std::string &data, size_t max_size = 0) const;

	/// \brief Attempt to receive `max_size` bytes of data.
	///
	/// `recv` only makes one attempt to retrieve the data. By default, `recv`
	/// waits indefinitely for data. Use a timeout value (`set_timeout`) if the
	/// net_socket should only wait for a definite time interval. If a timeout
	/// occurs, a `timeout_exception` is thrown. Exceptions also thrown if the
	/// net_socket is not connected or upon error.
	ssize_t recv(void *data, size_t max_size, int flags = 0);
	/// \brief Receive data into a vector.
	///
	/// Receives data in *network* byte order and converts elements to *host*
	/// byte order before returning.
	template<typename T>
		ssize_t recv(std::vector<T> &data, size_t max_size = 0);
	/// \brief Receive a string.
	///
	/// If `max_size` equals zero (the default), then `recv(std::string)`
	/// expects a NULL in the network byte stream. It will continue to receive
	/// bytes until it finds a NULL or until the default receive size is
	/// reached. If `max_size` is non-zero, the function will receive bytes
	/// until it finds a NULL or until it receives the first `max_size` bytes
	/// into the string. Any remaining characters after `max_size`, including
	/// NULL, will remain in the OS buffer. WARNING: If recv finds neither a
	/// NULL nor `max_size` bytes, then it will loop indefinitely.
	ssize_t recv(std::string &data, size_t max_size = 0);

	/// \brief Attempt to receive all the requested data.
	///
	/// Similar to `recv(void*)` except multiple attempts are made to receive
	/// `exact_size` bytes of data.
	/// \return The actual number of bytes received, which may be less than
	/// `exact_size`.
	ssize_t recv_all(void *data, size_t exact_size);
	/// \details See `recv_all(void*)` and `recv(std::vector)`. If `exact_size`
	/// equals zero (the default), then attempt to recive data.size() bytes. If
	/// both are zero, then attempt to receive the default receive size.
	template<typename T>
		ssize_t recv_all(std::vector<T> &data, size_t exact_size = 0);
	/// \details See `recv_all(void*)` and `recv(std::string)`.
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
	// % chance to drop a packet for packet_error_send
	const unsigned short _drop_rate{15};
	std::unique_ptr<std::default_random_engine> _rng;

	void copy(const net_socket *other = nullptr);
	void move(net_socket *other);
	int get_af() const;
	int get_socktype() const;
	template<typename T> void ntoh_swap(std::vector<T> &data) const;
	template<typename T> void hton_swap(std::vector<T> &data) const;
};

// Helper output operators
std::ostream& operator<<(std::ostream&, const address&);

//
// send/recv template definitions
//
template<typename T>
ssize_t net_socket::send(std::vector<T> data, size_t max_size) const {
	if( (max_size == 0) || (max_size > data.size()*sizeof(T)) ) {
		max_size = data.size()*sizeof(T);
	}

	hton_swap(data);

	return send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::packet_error_send(std::vector<T> data,
	size_t max_size) const {

	if( (max_size == 0) || (max_size > data.size()*sizeof(T)) ) {
		max_size = data.size()*sizeof(T);
	}
	hton_swap(data);

	return packet_error_send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::send_all(std::vector<T> data) const {
	hton_swap(data);
	return send_all(data.data(), data.size()*sizeof(T));
}

template<typename T>
ssize_t net_socket::recv(std::vector<T> &data, size_t max_size) {
	if( max_size == 0 ) {
		if( data.empty() ) {
			data.resize(_recv_size);
			max_size = _recv_size;
		}
		else {
			max_size = data.size()*sizeof(T);
		}
	}
	else {
		data.resize(max_size);
	}

	ssize_t ss = recv(data.data(), max_size);
	if( ss >= 0 ) {
		data.resize(ss/sizeof(T));
	}

	ntoh_swap(data);

	return ss;
}

template<typename T>
ssize_t net_socket::recv_all(std::vector<T> &data, size_t exact_size) {
	if( exact_size == 0 ) {
		if( data.empty() ) {
			data.resize(_recv_size);
			exact_size = _recv_size;
		}
		else {
			exact_size = data.size()*sizeof(T);
		}
	}
	else {
		data.resize(exact_size);
	}

	ssize_t ss = recv_all(data.data(), exact_size);
	if( ss >= 0 ) {
		data.resize(ss/sizeof(T));
	}

	ntoh_swap(data);

	return ss;
}

template<typename T>
void net_socket::hton_swap(std::vector<T> &data) const {
	if( sizeof(T) > 1 ) {
		for( auto itr : data ) {
			if( sizeof(T) == 2 ) {
				itr = htons(itr);
			}
			else if( sizeof(T) == 4 ) {
				itr = htonl(itr);
			}
			else {
				throw std::runtime_error("Unable to handle arrays with elements \
					larger than 4 bytes.");
			}
		}
	}
}

template<typename T>
void net_socket::ntoh_swap(std::vector<T> &data) const {
	if( sizeof(T) > 1 ) {
		for( auto itr : data ) {
			if( sizeof(T) == 2 ) {
				itr = ntohs(itr);
			}
			else if( sizeof(T) == 4 ) {
				itr = ntohl(itr);
			}
			else {
				throw std::runtime_error("Unable to handle arrays with elements \
					larger than 4 bytes.");
			}
		}
	}
}

} // namespace network_socket

#endif
