#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H

namespace network_socket {

class net_socket{
public:

	enum network_protocol {ANY, IPv4, IPv6};
	enum transport_protocol {UDP, TCP};

	net_socket();
	net_socket(const network_protocol net, const transport_protocol tran);

	// Getter and setter members
	int get_socket_descriptor() const {return _sock_desc;}
	network_protocol get_network_protocol() const {return _net_proto;}
	void set_network_protocol(network_protocol np);
	transport_protocol get_transport_protocol() const {return _trans_proto;}
	void set_transport_protocol(transport_protocol tp);

private:
	int _sock_desc;
	network_protocol _net_proto;
	transport_protocol _trans_proto;

	void init();
};

} // namespace network_socket

#endif
