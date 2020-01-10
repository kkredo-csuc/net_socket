#include "net_socket.h"
#include <stdexcept>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

using std::string;

namespace network_socket {

net_socket::net_socket() {
	init();
}

net_socket::net_socket(const network_protocol net, const transport_protocol tran) {
	init();
	_net_proto = net;
	_trans_proto = tran;

	if( tran != transport_protocol::TCP )
		throw std::invalid_argument("Only TCP sockets supported at this time");

	if( (net != network_protocol::ANY) && (net != network_protocol::IPv4) && (net != network_protocol::IPv6) )
		throw std::invalid_argument("Unsupported network protocol");
}

net_socket::net_socket(const net_socket& other) {
	init(&other);
}

net_socket& net_socket::operator=(const net_socket& rhs) {
	init(&rhs);
	return *this;
}

void net_socket::set_network_protocol(network_protocol np) {
	if( _passive )
		throw std::runtime_error("Unable to change network protocol of an open socket");

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
	if( _passive )
		throw std::runtime_error("Unable to change transport protocol of an open socket");

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

void net_socket::set_backlog(int b) {
	if( b < 0 )
		throw std::invalid_argument("Negative backlog values not allowed");

	if( _passive )
		throw std::runtime_error("Unable to change backlog of passively opened socket");

	_backlog = b;
}

void net_socket::listen(const string host, const string service) {
	if( _sock_desc != -1 )
		throw std::runtime_error("Listen called on an open socket");

	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;
	const char *h;

	/* Build address data structure */
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = get_af();
	hints.ai_socktype = get_socktype();
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	if( host.empty() ){
		h = NULL;
	}
	else {
		h = host.c_str();
	}

	/* Get local address info */
	if ( ( s = getaddrinfo(h, service.c_str(), &hints, &result ) ) != 0 ) {
		throw std::runtime_error(gai_strerror(s));
	}

	/* Iterate through the address list and try to perform passive open */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( !bind( s, rp->ai_addr, rp->ai_addrlen ) ) {
			break;
		}

		::close( s );
	}
	freeaddrinfo( result );

	if ( rp == NULL ) {
		throw std::runtime_error(strerror(errno));
	}
	if ( ::listen( s, _backlog ) == -1 ) {
		::close( s );
		throw std::runtime_error(strerror(errno));
	}

	_sock_desc = s;
	_passive = true;
}

void net_socket::listen(const string host, const unsigned short port) {
	listen(host, std::to_string(port));
}

void net_socket::listen(const string service) {
	listen("", service);
}

void net_socket::listen(const unsigned short port){
	listen("", std::to_string(port));
}

// Private members
void net_socket::init(const net_socket *other) {
	if( other != nullptr ) {
		if( _passive || other->_passive )
			throw std::runtime_error("Unable to assign to/from an open socket");

		_net_proto = other->_net_proto;
		_trans_proto = other->_trans_proto;
		_backlog = other->_backlog;
	}
	else {
		_net_proto = network_protocol::ANY;
		_trans_proto = transport_protocol::TCP;
		_backlog = 5;
	}

	_sock_desc = -1;
	_passive = false;
}

int net_socket::get_af() const {
	int ret;
	switch(_net_proto){
		case IPv4: ret = AF_INET; break;
		case IPv6: ret = AF_INET6; break;
		case ANY: ret = AF_UNSPEC; break;
		default:
			throw std::runtime_error("Undefined network protocol (" + std::to_string(_net_proto) + ")");
			break;
	}

	return ret;
}

int net_socket::get_socktype() const {
	int ret;
	switch(_trans_proto){
		case TCP: ret = SOCK_STREAM; break;
		case UDP: ret = SOCK_DGRAM; break;
		default:
			throw std::runtime_error("Undefined transport protocol (" + std::to_string(_trans_proto) + ")");
			break;
	}

	return ret;
}

} // namespace network_socket
