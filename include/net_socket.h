#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <string>
#include <memory>
#include <vector>
#include <random>
#include <stdexcept>
#include <netinet/ip.h>

namespace network_socket {

/// An exception thrown when data does not arrive before a timeout value, if one is set.
class timeout_exception : public std::runtime_error {
public:
	timeout_exception() : runtime_error("TIMEOUT!") {};
};

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

	bool is_ipv4() const;
	bool is_ipv6() const;
	// Returns in host byte order
	in_port_t get_port() const;
	// Argument in host byte order
	void set_port(in_port_t);
	std::string get_address() const;
	void set_address(const std::string&);
	struct sockaddr_storage get_sockaddr() const {return addr;}

	friend bool operator==(const address&, const address&);
private:
	struct sockaddr_storage addr;
};
bool operator!=(const address&, const address&);

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

	// Socket address (name) information.
	// Exception thrown for local if socket is not connected and not passively opened.
	address get_local_address() const;
	// Exception thrown for remote if socket is not connected.
	address get_remote_address() const;

	ssize_t send(const void *data, size_t max_size) const;
	template<typename T> ssize_t send(const std::vector<T> &data, size_t max_size = 0) const;
	ssize_t send(const std::string &data, size_t max_size) const;
	ssize_t send(const std::string &data) const; // Also sends NULL
	ssize_t packet_error_send(const void *data, size_t max_size) const;
	template<typename T> ssize_t packet_error_send(const std::vector<T> &data, size_t max_size = 0) const;
	ssize_t packet_error_send(const std::string &data, size_t max_size) const;
	ssize_t packet_error_send(const std::string &data) const;
	ssize_t send_all(const void *data, size_t exact_size) const;
	template<typename T> ssize_t send_all(const std::vector<T> &data) const;
	ssize_t send_all(const std::string &data, size_t max_size) const;
	ssize_t send_all(const std::string &data) const; // Also sends NULL
	ssize_t recv(void *data, size_t max_size, int flags = 0);
	template<typename T> ssize_t recv(std::vector<T> &data, size_t max_size = 0);
	ssize_t recv(std::string &data, size_t max_size);
	ssize_t recv(std::string &data); // Attempt to recv untill NULL
	ssize_t recv_all(void *data, size_t exact_size);
	ssize_t recv_all(std::string &data, size_t exact_size);
	ssize_t recv_all(std::string &data); // recv untill NULL
	template<typename T> ssize_t recv_all(std::vector<T> &data, size_t exact_size = 0);

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
	const unsigned short _drop_rate{15}; // % chance to drop a packet for packet_error_send
	std::unique_ptr<std::default_random_engine> _rng;

	void copy(const net_socket *other = nullptr);
	void move(net_socket *other);
	int get_af() const;
	int get_socktype() const;
};

// Helper output operators
std::ostream& operator<<(std::ostream&, const address&);

//
// send/recv template definitions
//
template<typename T>
ssize_t net_socket::send(const std::vector<T> &data, size_t max_size) const {
	if( (max_size == 0) || (max_size > data.size()*sizeof(T)) ) {
		max_size = data.size()*sizeof(T);
	}

	return send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::packet_error_send(const std::vector<T> &data, size_t max_size) const {
	if( (max_size == 0) || (max_size > data.size()*sizeof(T)) ) {
		max_size = data.size()*sizeof(T);
	}

	return packet_error_send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::send_all(const std::vector<T> &data) const {
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
		data.resize(ss);
	}

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

	return ss;
}

} // namespace network_socket

#endif
