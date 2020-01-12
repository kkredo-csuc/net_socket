#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <string>
#include <memory>
#include <vector>

namespace network_socket {

class timeout_exception : public std::runtime_error {
public:
	timeout_exception() : runtime_error("TIMEOUT!") {};
};

class net_socket{
public:

	enum network_protocol {ANY, IPv4, IPv6};
	enum transport_protocol {UDP, TCP};

	explicit net_socket(
		network_protocol net = network_protocol::ANY,
		transport_protocol tran = transport_protocol::TCP
		);
	net_socket(const net_socket &other);
	net_socket(net_socket&& other) noexcept;
	net_socket& operator=(const net_socket& rhs);
	net_socket& operator=(net_socket&& rhs) noexcept;
	~net_socket() noexcept;

	// Getter and setter members
	int get_socket_descriptor() const {return _sock_desc;}
	network_protocol get_network_protocol() const {return _net_proto;}
	void set_network_protocol(network_protocol np);
	transport_protocol get_transport_protocol() const {return _trans_proto;}
	void set_transport_protocol(transport_protocol tp);
	bool is_passively_opened() const {return _passive;}
	int get_backlog() const {return _backlog;}
	void set_backlog(int backlog);
	bool is_connected() const {return _connected;}
	bool timeout_is_set() const {return _do_timeout;}
	double get_timeout() const;
	void set_timeout(double s);
	void clear_timeout() {_do_timeout = false;}
	size_t get_recv_size() const {return _recv_size;}
	void set_recv_size(size_t s) {_recv_size = s;}

	void listen(const std::string &host, const std::string &service);
	void listen(const std::string &host, unsigned short port);
	void listen(const std::string &service);
	void listen(unsigned short port);
	void connect(const std::string &host, const std::string &service);
	void connect(const std::string &host, unsigned short port);
	std::unique_ptr<net_socket> accept();
	void close();

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
