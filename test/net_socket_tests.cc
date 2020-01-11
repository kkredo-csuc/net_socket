#include <gtest/gtest.h>
#include <thread>
#include <random>
#include <atomic>
#include <sys/socket.h>
#include "net_socket.h"

using std::runtime_error;
using std::unique_ptr;
using std::thread;
using std::string;
using network_socket::net_socket;

std::atomic<bool> server_ready_mutex(false);

// Helper functions
unsigned short get_random_port();
void check_and_echo_server(unique_ptr<net_socket> server);
thread spawn_and_check_server(
	void (*func)(unique_ptr<net_socket> server),
	unsigned short port
	);
unique_ptr<net_socket> create_connected_client(unsigned short port);
unique_ptr<net_socket> create_connected_client(const string &service);

TEST( NetSocket, ConstructorTests ) {
	// Default constructor
	net_socket s;
	EXPECT_EQ(s.get_socket_descriptor(), -1);
	EXPECT_EQ(s.get_network_protocol(), net_socket::network_protocol::ANY);
	EXPECT_EQ(s.get_transport_protocol(), net_socket::transport_protocol::TCP);
	EXPECT_FALSE(s.is_passively_opened());
	EXPECT_EQ(s.get_backlog(), 5);
	EXPECT_FALSE(s.is_connected());

	// IPv4, TCP
	net_socket s0(net_socket::network_protocol::IPv4, net_socket::transport_protocol::TCP);
	EXPECT_EQ(s0.get_network_protocol(), net_socket::network_protocol::IPv4);
	EXPECT_EQ(s0.get_transport_protocol(), net_socket::transport_protocol::TCP);

	// IPv4, UDP
	EXPECT_THROW(
		net_socket s1(net_socket::network_protocol::IPv4, net_socket::transport_protocol::UDP),
		std::invalid_argument
		);
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
	EXPECT_THROW(
		net_socket s3(net_socket::network_protocol::IPv6, net_socket::transport_protocol::UDP),
		std::invalid_argument
		);
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
	EXPECT_THROW(
		s.set_network_protocol(static_cast<net_socket::network_protocol>(99)),
		std::invalid_argument
		);

	// Transport protocol
	net_socket::transport_protocol tproto = net_socket::transport_protocol::UDP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	tproto = net_socket::transport_protocol::TCP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	EXPECT_THROW(
		s.set_transport_protocol(static_cast<net_socket::transport_protocol>(99)),
		std::invalid_argument
		);

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
}

TEST( NetSocket, AssignmentOperatorTests ) {
	net_socket s0, s1;
	s1.set_network_protocol(net_socket::network_protocol::IPv6);
	s1.set_transport_protocol(net_socket::transport_protocol::UDP);
	s1.set_backlog(s1.get_backlog()+10);
	// No way to change these items for an unopened socket
	//  - socket descriptor
	//  - passively opened flag
	//  - connected flag
	ASSERT_NE(s0.get_network_protocol(), s1.get_network_protocol());
	ASSERT_NE(s0.get_transport_protocol(), s1.get_transport_protocol());
	ASSERT_NE(s0.get_backlog(), s1.get_backlog());

	s0 = s1;
	ASSERT_EQ(s0.get_socket_descriptor(), s1.get_socket_descriptor());
	ASSERT_EQ(s0.get_network_protocol(), s1.get_network_protocol());
	ASSERT_EQ(s0.get_transport_protocol(), s1.get_transport_protocol());
	ASSERT_EQ(s0.is_passively_opened(), s1.is_passively_opened());
	ASSERT_EQ(s0.get_backlog(), s1.get_backlog());
	ASSERT_EQ(s0.is_connected(), s1.is_connected());
}

TEST( NetSocket, ConnectTests ) {
	unsigned short port = get_random_port();
	thread st0 = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c0 = create_connected_client(port);
	EXPECT_TRUE(c0->is_connected());

	st0.join();

	thread st1 = spawn_and_check_server(check_and_echo_server, 34980); // ethercat at port 34980
	unique_ptr<net_socket> c1 = create_connected_client("ethercat");

	st1.join();

	// No server running at name and port
	net_socket c2;
	EXPECT_THROW(c2.connect("localhost", 30000), runtime_error);

	// Invalid hostname
	EXPECT_THROW(c2.connect("nowayto.xist[0]", 30000), runtime_error);

	// Invalid service name
	EXPECT_THROW(c2.connect("localhost", "qszldfg"), runtime_error);
}

TEST( NetSocket, InvalidOperationTests ) {
	unsigned short port = get_random_port();
	thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c0 = create_connected_client(port);

	// Invalid functions on an already connected socket
	EXPECT_THROW(c0->connect("localhost", 30000), runtime_error);
	EXPECT_THROW(c0->listen(9000), runtime_error);
	EXPECT_THROW(c0->accept(), runtime_error);
	EXPECT_THROW(c0->set_network_protocol(net_socket::network_protocol::IPv6), runtime_error);
	EXPECT_THROW(c0->set_transport_protocol(net_socket::transport_protocol::UDP), runtime_error);

	// Throw exception if assigning to/from an open socket
	EXPECT_THROW(net_socket e3 = *c0, runtime_error);
	EXPECT_THROW(net_socket e4(*c0), runtime_error);
	net_socket e5;
	EXPECT_THROW(e5 = *c0, runtime_error);

	st.join();

	// Can't call accept on an unopened socket
	net_socket s0;
	EXPECT_THROW(s0.accept(), runtime_error);
}

TEST( NetSocket, DestructorTests ){
	unsigned short port = get_random_port();
	thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c0 = create_connected_client(port);

	int sd = c0->get_socket_descriptor();

	// Check that a send works
	ssize_t amt = ::send(sd, &sd, sizeof(sd), 0);
	ASSERT_GT(amt, 0);

	// Desctructor should close the socket
	c0.reset();

	// Send should return -1 on a closed socket
	ssize_t ret = ::send(sd, &sd, sizeof(sd), 0);
	ASSERT_EQ(ret, -1);

	st.join();
}

// Helper function definitions
unsigned short get_random_port() {
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<unsigned short> dist(5000,50000);
	return dist(generator);
}

void check_and_echo_server(unique_ptr<net_socket> server) {
	unique_ptr<net_socket> worker = server->accept();

	EXPECT_TRUE(worker->is_connected());
	EXPECT_FALSE(worker->is_passively_opened());
}

thread spawn_and_check_server(void (*func)(unique_ptr<net_socket>), const unsigned short port){
	server_ready_mutex = false;

	unique_ptr<net_socket> server(new net_socket);
	try{
		server->listen("localhost", port);
	}
	catch( runtime_error &e ){
		server_ready_mutex = true; // Let client run and generate an error
		throw;
	}
	EXPECT_TRUE(server->is_passively_opened());
	EXPECT_FALSE(server->is_connected());
	EXPECT_THROW(server->set_network_protocol(net_socket::network_protocol::IPv6), runtime_error);
	EXPECT_THROW(
		server->set_transport_protocol(net_socket::transport_protocol::UDP),
		runtime_error
		);
	EXPECT_THROW(server->set_backlog(server->get_backlog()+1), runtime_error);

	// Throw exception if assigning to/from an open socket
	EXPECT_THROW(net_socket e0 = *server, runtime_error);
	EXPECT_THROW(net_socket e1(*server), runtime_error);
	net_socket e2;
	EXPECT_THROW(e2 = *server, runtime_error);

	// Can't call connect on a passively opened socket
	EXPECT_THROW(server->connect("localhost", 9000), runtime_error);

	server_ready_mutex = true;

	return std::thread(func, std::move(server));
}

void wait_until_server_ready() {
	// Wait for server to startup
	while(!server_ready_mutex) {
		std::this_thread::yield();
	}
}

unique_ptr<net_socket> create_connected_client(const unsigned short port) {
	wait_until_server_ready();
	unique_ptr<net_socket> client(new net_socket());
	client->connect("localhost", port);

	return client;
}

unique_ptr<net_socket> create_connected_client(const string &service) {
	wait_until_server_ready();
	unique_ptr<net_socket> client(new net_socket());

	client->connect("localhost", service);

	return client;
}
