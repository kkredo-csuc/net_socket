#include "net_socket.h"
#include <stdexcept>

namespace network_socket {

net_socket::net_socket() {
	init();
}

net_socket::net_socket(const network_protocol net, const transport_protocol tran) {
	init();
	_net_proto = net;
	_trans_proto = tran;

	if( tran != transport_protocol::TCP )
		throw std::invalid_argument("Only TCP sockets supported at this time.");

	if( (net != network_protocol::ANY) && (net != network_protocol::IPv4) && (net != network_protocol::IPv6) )
		throw std::invalid_argument("Unsupported network protocol.");
}

void net_socket::set_network_protocol(network_protocol np) {
	switch(np) {
		case network_protocol::ANY:
		case network_protocol::IPv4:
		case network_protocol::IPv6:
			_net_proto = np;
			break;
		default:
			throw std::invalid_argument("Unsupported network protocol");
			break;
	}
}

void net_socket::set_transport_protocol(transport_protocol tp) {
	switch(tp) {
		case transport_protocol::UDP:
		case transport_protocol::TCP:
			_trans_proto = tp;
			break;
		default:
			throw std::invalid_argument("Unsupported transport protocol");
			break;
	}
}

// Private members
void net_socket::init() {
	_sock_desc = -1;
	_net_proto = network_protocol::ANY;
	_trans_proto = transport_protocol::TCP;
}

} // namespace network_socket
