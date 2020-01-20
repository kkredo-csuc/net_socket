#include "net_socket.h"
#include <stdexcept>
#include <netdb.h>
#include <unistd.h>
#include <chrono>

using std::string;
using std::unique_ptr;

namespace network_socket {

net_socket::net_socket(const network_protocol net, const transport_protocol tran) :
	_net_proto(net), _trans_proto(tran) {

	if( _trans_proto != transport_protocol::TCP ) {
		throw std::invalid_argument(
			"net_socket::net_socket(): Only TCP sockets supported at this time");
	}

	if( (_net_proto != network_protocol::ANY)
		&& (_net_proto != network_protocol::IPv4)
		&& (_net_proto != network_protocol::IPv6) ) {

		throw std::invalid_argument("net_socket::net_socket(): Unsupported network protocol");
	}

	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	_rng = unique_ptr<std::default_random_engine>(new std::default_random_engine(seed));
}

net_socket::net_socket(const net_socket& other) : net_socket() {
	copy(&other);
}

net_socket::net_socket(net_socket&& other) noexcept : net_socket() {
	move(&other);
}

net_socket& net_socket::operator=(const net_socket& rhs) {
	copy(&rhs);
	return *this;
}

net_socket& net_socket::operator=(net_socket&& rhs) noexcept {
	move(&rhs);
	return *this;
}

net_socket::~net_socket() noexcept {
	close();
}

void net_socket::set_network_protocol(network_protocol np) {
	if( _passive || _connected ) {
		throw std::runtime_error(
			"net_socket::set_network_protocol(): \
			Unable to change network protocol of an open socket");
	}

	switch(np) {
		case network_protocol::ANY:
		case network_protocol::IPv4:
		case network_protocol::IPv6:
			_net_proto = np;
			break;
		default:
			throw std::invalid_argument(
				"net_socket::set_network_protocol(): Unsupported network protocol");
			break;
	}
}

void net_socket::set_transport_protocol(transport_protocol tp) {
	if( _passive || _connected ) {
		throw std::runtime_error(
			"net_socket::set_transport_protocol(): \
			Unable to change transport protocol of an open socket");
	}

	switch(tp) {
		case transport_protocol::UDP:
			throw std::invalid_argument(
				"net_socket::set_transport_protocol(): Only TCP sockets supported at this time");
			break;
		case transport_protocol::TCP:
			_trans_proto = tp;
			break;
		default:
			throw std::invalid_argument(
				"net_socket::set_transport_protocol(): Unsupported transport protocol");
			break;
	}
}

void net_socket::set_backlog(int backlog) {
	if( backlog < 0 ) {
		throw std::invalid_argument(
			string("net_socket::set_backlog(): Negative backlog values (")
			+ std::to_string(backlog) + string(") not allowed"));
	}

	if( _passive ) {
		throw std::runtime_error(
			"net_socket::set_backlog(): Unable to change backlog of passively opened socket");
	}

	_backlog = backlog;
}

double net_socket::get_timeout() const {
	double ret = 0.0;
	if( _do_timeout ) {
		ret =  _timeout.tv_sec + static_cast<double>(_timeout.tv_usec)/1e6;
	}

	return ret;
}

void net_socket::set_timeout(double s) {
	if( s < 0.0 ) {
		throw std::invalid_argument(
			std::string("net_socket::set_timeout(): Negative timeout value (")
				+ std::to_string(s) + std::string(") provided"));
	}

	if( s == 0 ) {
		_do_timeout = false;
		_timeout.tv_sec = 0;
		_timeout.tv_usec = 0;
	}
	else {
		_do_timeout = true;
		_timeout.tv_sec = static_cast<long>(s);
		_timeout.tv_usec = static_cast<long>((s-_timeout.tv_sec)*1e6);
	}
}

void net_socket::listen(const std::string &host, const std::string &service) {
	if( _sock_desc != -1 ) {
		throw std::runtime_error("net_socket::listen(): Listen called on an open socket");
	}

	struct addrinfo hints{};
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
		h = nullptr;
	}
	else {
		h = host.c_str();
	}

	/* Get local address info */
	if ( ( s = getaddrinfo(h, service.c_str(), &hints, &result ) ) != 0 ) {
		throw std::runtime_error(string("net_socket::listen(): ") + string(gai_strerror(s)));
	}

	/* Iterate through the address list and try to perform passive open */
	for ( rp = result; rp != nullptr; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( bind( s, rp->ai_addr, rp->ai_addrlen ) == 0 ) {
			break;
		}

		::close( s );
	}
	freeaddrinfo( result );

	if ( rp == nullptr ) {
		throw std::runtime_error(string("net_socket::listen(): ") + string(strerror(errno)));
	}
	if ( ::listen( s, _backlog ) == -1 ) {
		::close( s );
		throw std::runtime_error(string("net_socket::listen(): ") + string(strerror(errno)));
	}

	_sock_desc = s;
	_passive = true;
}

void net_socket::listen(const std::string &host, const unsigned short port) {
	listen(host, std::to_string(port));
}

void net_socket::listen(const std::string &service) {
	listen("", service);
}

void net_socket::listen(const unsigned short port){
	listen("", std::to_string(port));
}

void net_socket::connect(const std::string &host, const std::string &service) {
	if( _passive ) {
		throw std::runtime_error(
			"net_socket::connect(): Unable to connect using a passively opened socket");
	}

	struct addrinfo hints{};
	struct addrinfo *rp, *result;
	int s;

	// Translate host name into peer's IP address
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = get_af();
	hints.ai_socktype = get_socktype();
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ( ( s = getaddrinfo( host.c_str(), service.c_str(), &hints, &result ) ) != 0 ) {
		throw std::runtime_error(string("net_socket::connect(): ") + string(strerror(errno)));
	}

	// Iterate through the address list and try to connect
	for ( rp = result; rp != nullptr; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( ::connect( s, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
			break;
		}

		::close( s );
	}
	freeaddrinfo( result );
	if ( rp == nullptr ) {
		throw std::runtime_error(string("net_socket::connect(): ") + string(strerror(errno)));
	}

	_sock_desc = s;
	_connected = true;
}

void net_socket::connect(const std::string &host, const unsigned short port) {
	connect(host, std::to_string(port));
}

unique_ptr<net_socket> net_socket::accept() {
	int new_s = ::accept(_sock_desc, nullptr, nullptr);
	if( new_s == -1 ){
		throw std::runtime_error(string("net_socket::accept(): ") + string(strerror(errno)));
	}

	unique_ptr<net_socket> ret(new net_socket(_net_proto, _trans_proto));
	ret->_sock_desc = new_s;
	ret->_connected = true;

	return ret;
}

void net_socket::close() {
	if( _sock_desc != -1 ){
		::close(_sock_desc);
		_sock_desc = -1;
		_passive = false;
		_connected = false;
	}
}

ssize_t net_socket::send(const void *data, size_t max_size) const {
	if( !_connected ) {
		throw std::runtime_error("net_socket::send(): Unable to send on unconnected socket");
	}

	ssize_t ret = ::send(_sock_desc, data, max_size, 0);
	if( ret == -1 ) {
		throw std::runtime_error(string("net_socket::send(): ")+string(strerror(errno)));
	}

	return ret;
}

ssize_t net_socket::send(const std::string &data, size_t max_size) const {
	if( (max_size == 0) || (max_size > data.length()) ) {
		max_size = data.length();
	}

	return send(data.data(), max_size);
}

ssize_t net_socket::packet_error_send(const void *data, size_t max_size) const {
	// Must check here in addition to send() in case the packet
	// is dropped and send() is never called.
	if( !_connected ) {
		throw std::runtime_error(
			"net_socket::packet_error_send(): Unable to send on unconnected socket");
	}

	std::uniform_int_distribution<unsigned short> dist(1,100);

	ssize_t ret;
	if( dist(*_rng) <= _drop_rate ) {
		ret = max_size;
	}
	else {
		ret = send(data, max_size);
	}

	return ret;
}

ssize_t net_socket::packet_error_send(const std::string &data, size_t max_size) const {
	if( (max_size == 0) || (max_size > data.length()) ) {
		max_size = data.length();
	}

	return packet_error_send(data.data(), max_size);
}

ssize_t net_socket::send_all(const void *data, size_t exact_size) const {
	if( !_connected ) {
		throw std::runtime_error(
			"net_socket::send_all(): Unable to send_all on unconnected socket");
	}

	auto d = static_cast<const char*>(data);
	ssize_t sent = 0;
	while( sent < exact_size ) {
		sent += send(d + sent, exact_size - sent);
	}

	return sent;
}

ssize_t net_socket::send_all(const std::string &data) const {
	return send_all(data.data(), data.length());
}

ssize_t net_socket::recv(void *data, size_t max_size) {
	if( !_connected ) {
		throw std::runtime_error("net_socket::recv(): Unable to recv on unconnected socket");
	}

	if( max_size == 0 ){
		return 0;
	}

	if( _do_timeout ){
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(_sock_desc, &fds);
		struct timeval tmp_tv = _timeout;
		int sret = select(_sock_desc+1, &fds, nullptr, nullptr, &tmp_tv);
		if( sret < 0 ) {
			throw std::runtime_error(string("net_socket::recv(): ")+string(strerror(errno)));
		}

		if( sret == 0 ) {
			throw timeout_exception();
		}
	}

	ssize_t ret = ::recv(_sock_desc, data, max_size, 0);
	if( ret == -1 ) {
		throw std::runtime_error(string("net_socket::recv(): ")+string(strerror(errno)));
	}

	if( ret == 0 ) {
		close();
	}

	return ret;
}

ssize_t net_socket::recv(std::string &data, size_t max_size) {
	if( max_size == 0 ) {
		if( data.empty() ) {
			max_size = _recv_size;
		}
		else {
			max_size = data.length();
		}
	}

	char tmp[max_size];
	ssize_t ss = recv(tmp, max_size);
	data.assign(tmp, tmp + ss);
	return ss;
}

ssize_t net_socket::recv_all(void *data, size_t exact_size) {
	if( !_connected ) {
		throw std::runtime_error("net_socket::recv_all(): Unable to recv on unconnected socket");
	}

	auto d = static_cast<char*>(data);
	size_t rcvd = 0, rs;
	while( rcvd < exact_size ) {
		rs = recv(d + rcvd, exact_size - rcvd);
		if( rs == 0 ){
			break;
		}
		rcvd += rs;
	}

	return rcvd;
}

ssize_t net_socket::recv_all(std::string &data, size_t exact_size) {
	if( exact_size == 0 ) {
		if( data.empty() ) {
			exact_size = _recv_size;
		}
		else {
			exact_size = data.length();
		}
	}

	char tmp[exact_size];
	ssize_t ss = recv_all(tmp, exact_size);
	data.assign(tmp, tmp + ss);
	return ss;
}

// Private members
void net_socket::copy(const net_socket *other) {
	if( other != nullptr ) {
		if( _passive || other->_passive || _connected || other->_connected ) {
			throw std::runtime_error(
				"net_socket internal error: Unable to assign to/from an open socket");
		}

		_net_proto = other->_net_proto;
		_trans_proto = other->_trans_proto;
		_backlog = other->_backlog;
		_do_timeout = other->_do_timeout;
		_timeout = other->_timeout;
		_recv_size = other->_recv_size;
	}
	else {
		_net_proto = network_protocol::ANY;
		_trans_proto = transport_protocol::TCP;
		_backlog = 5;
		_do_timeout = false;
		_timeout = {};
		_recv_size = 1400;
	}

	_sock_desc = -1;
	_passive = false;
	_connected = false;
}

void net_socket::move(net_socket *other) {
	_sock_desc = other->_sock_desc;
	_net_proto = other->_net_proto;
	_trans_proto = other->_trans_proto;
	_passive = other->_passive;
	_backlog = other->_backlog;
	_connected = other->_connected;
	_do_timeout = other->_do_timeout;
	_timeout = other->_timeout;
	_recv_size = other->_recv_size;
	other->copy();
}

int net_socket::get_af() const {
	int ret;
	switch(_net_proto){
		case IPv4: ret = AF_INET; break;
		case IPv6: ret = AF_INET6; break;
		case ANY: ret = AF_UNSPEC; break;
		default:
			throw std::runtime_error("net_socket internal error: Undefined network protocol ("
				+ std::to_string(_net_proto) + ")");
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
			throw std::runtime_error("net_socket internal error: Undefined transport protocol ("
				+ std::to_string(_trans_proto) + ")");
			break;
	}

	return ret;
}

} // namespace network_socket
