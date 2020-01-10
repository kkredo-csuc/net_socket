#include <gtest/gtest.h>
#include "net_socket.h"

using std::runtime_error;
using network_socket::net_socket;

TEST( NetSocket, ConstructorTests ) {
	// Default constructor
	net_socket s;
	EXPECT_EQ(s.get_socket_descriptor(), -1);
	EXPECT_EQ(s.get_network_protocol(), net_socket::network_protocol::ANY);
	EXPECT_EQ(s.get_transport_protocol(), net_socket::transport_protocol::TCP);
	EXPECT_FALSE(s.is_passively_opened());
	EXPECT_EQ(s.get_backlog(), 5);

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

TEST( NetSocket, GetterAndSetterTests ) {
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
	EXPECT_THROW(s.set_network_protocol(static_cast<net_socket::network_protocol>(99)), std::invalid_argument);

	// Transport protocol
	net_socket::transport_protocol tproto = net_socket::transport_protocol::UDP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	tproto = net_socket::transport_protocol::TCP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	EXPECT_THROW(s.set_transport_protocol(static_cast<net_socket::transport_protocol>(99)), std::invalid_argument);

	int bl = s.get_backlog();
	s.set_backlog(bl+10);
	EXPECT_EQ(s.get_backlog(), bl+10);
	ASSERT_THROW(s.set_backlog(-10), std::invalid_argument);
}

TEST( NetSocket, ListenTests ) {
	net_socket s1;
	s1.listen("localhost", "ums");
	EXPECT_TRUE(s1.is_passively_opened());

	net_socket s2;
	s2.listen("localhost", "9000");
	EXPECT_TRUE(s2.is_passively_opened());

	net_socket s3;
	s3.listen("localhost", 9001);
	EXPECT_TRUE(s3.is_passively_opened());

	net_socket s4;
	s4.listen("rfe");
	EXPECT_TRUE(s4.is_passively_opened());

	net_socket s5;
	s5.listen("9002");
	EXPECT_TRUE(s5.is_passively_opened());

	net_socket s6;
	s6.listen(9003);
	EXPECT_TRUE(s6.is_passively_opened());

	net_socket s7;
	s7.listen("127.0.0.1", 9004);
	EXPECT_TRUE(s7.is_passively_opened());

	// Throw exception on already used port
	net_socket s8;
	EXPECT_THROW(s8.listen(9004), runtime_error);

	// Throw exception if assigning to/from an open socket
	EXPECT_THROW(net_socket s9 = s1, runtime_error);
	EXPECT_THROW(net_socket s10(s1), runtime_error);
	net_socket s0;
	EXPECT_THROW(s2 = s0, runtime_error);
}

TEST( NetSocket, StatefulGetterSetterTests ) {
	net_socket s0;
	s0.listen("localhost", 9000);
	EXPECT_THROW(s0.set_network_protocol(net_socket::network_protocol::IPv6), runtime_error);
	EXPECT_THROW(s0.set_transport_protocol(net_socket::transport_protocol::UDP), runtime_error);
	EXPECT_THROW(s0.set_backlog(s0.get_backlog()+1), runtime_error);
}
