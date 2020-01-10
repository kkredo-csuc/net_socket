#include <gtest/gtest.h>
#include "net_socket.h"

using network_socket::net_socket;

TEST( NetSocket, ConstructorTests ) {
	// Default constructor
	net_socket s;
	EXPECT_EQ(s.get_socket_descriptor(), -1);
	EXPECT_EQ(s.get_network_protocol(), net_socket::network_protocol::ANY);
	EXPECT_EQ(s.get_transport_protocol(), net_socket::transport_protocol::TCP);

	// IPv4, TCP
	net_socket s0(net_socket::network_protocol::IPv4, net_socket::transport_protocol::TCP);
	EXPECT_EQ(s0.get_network_protocol(), net_socket::network_protocol::IPv4);
	EXPECT_EQ(s0.get_transport_protocol(), net_socket::transport_protocol::TCP);

	// IPv4, UDP
	EXPECT_THROW(net_socket s1(net_socket::network_protocol::IPv4, net_socket::transport_protocol::UDP), std::invalid_argument);
	/* Once UDP implemented
	net_socket s1(net_socket::network_protocol::IPv4, net_socket::transport_protocol::UDP);
	EXPECT_EQ(s1.get_network_protocol(), net_socket::network_protocol::IPv4);
	EXPECT_EQ(s1.get_transport_protocol(), net_socket::transport_protocol::TCP);
	*/

	// IPv6, TCP
	net_socket s2(net_socket::network_protocol::IPv6, net_socket::transport_protocol::TCP);
	EXPECT_EQ(s2.get_network_protocol(), net_socket::network_protocol::IPv6);
	EXPECT_EQ(s2.get_transport_protocol(), net_socket::transport_protocol::TCP);

	// IPV6, UDP
	EXPECT_THROW(net_socket s3(net_socket::network_protocol::IPv6, net_socket::transport_protocol::UDP), std::invalid_argument);
	/* Once UDP implemented
	net_socket s3(net_socket::network_protocol::IPv6, net_socket::transport_protocol::UDP);
	EXPECT_EQ(s3.get_network_protocol(), net_socket::network_protocol::IPv6);
	EXPECT_EQ(s3.get_transport_protocol(), net_socket::transport_protocol::TCP);
	*/
}

TEST(NetSocket, GetterAndSetterTests ) {
	net_socket s;

	// Network protocol
	net_socket::network_protocol nproto = net_socket::network_protocol::ANY;
	s.set_network_protocol(nproto);
	EXPECT_EQ(s.get_network_protocol(), nproto);
	nproto = net_socket::network_protocol::IPv4;
	s.set_network_protocol(nproto);
	EXPECT_EQ(s.get_network_protocol(), nproto);
	nproto = net_socket::network_protocol::IPv6;
	s.set_network_protocol(nproto);
	EXPECT_EQ(s.get_network_protocol(), nproto);

	// Transport protocol
	net_socket::transport_protocol tproto = net_socket::transport_protocol::UDP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	tproto = net_socket::transport_protocol::TCP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
}
