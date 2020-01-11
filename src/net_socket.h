#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

#include <string>
#include <memory>

namespace network_socket {

class net_socket{
public:

	enum network_protocol {ANY, IPv4, IPv6};
	enum transport_protocol {UDP, TCP};

	net_socket();
	net_socket(const network_protocol net, const transport_protocol tran);
	net_socket(const net_socket& rhs);
	net_socket& operator=(const net_socket& rhs);

	// Getter and setter members
	int get_socket_descriptor() const {return _sock_desc;}
	network_protocol get_network_protocol() const {return _net_proto;}
	void set_network_protocol(network_protocol np);
	transport_protocol get_transport_protocol() const {return _trans_proto;}
	void set_transport_protocol(transport_protocol tp);
	bool is_passively_opened() const {return _passive;}
	int get_backlog() const {return _backlog;}
	void set_backlog(int i);
	bool is_connected() const {return _connected;}

	void listen(const std::string host, const std::string service);
	void listen(const std::string host, const unsigned short port);
	void listen(const std::string service);
	void listen(const unsigned short port);
	void connect(const std::string host, const std::string service);
	void connect(const std::string host, const unsigned short port);
	std::unique_ptr<net_socket> accept();
private:
	int _sock_desc;
	network_protocol _net_proto;
	transport_protocol _trans_proto;
	bool _passive;
	int _backlog;
	bool _connected;

	void init(const net_socket *other = nullptr);
	int get_af() const;
	int get_socktype() const;
};

} // namespace network_socket

#endif
