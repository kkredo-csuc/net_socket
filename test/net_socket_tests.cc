#include <gtest/gtest.h>
#include <thread>
#include <random>
#include <atomic>
#include <sys/socket.h>
#include "net_socket.h"

using std::runtime_error;
using std::invalid_argument;
using std::unique_ptr;
using std::thread;
using std::string;
using std::vector;
using network_socket::net_socket;
using network_socket::timeout_exception;

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
	EXPECT_FALSE(s.timeout_is_set());
	EXPECT_EQ(s.get_timeout(), 0.0);
	EXPECT_EQ(s.get_recv_size(), 1400);

	// IPv4, TCP
	net_socket s0(net_socket::network_protocol::IPv4, net_socket::transport_protocol::TCP);
	EXPECT_EQ(s0.get_network_protocol(), net_socket::network_protocol::IPv4);
	EXPECT_EQ(s0.get_transport_protocol(), net_socket::transport_protocol::TCP);

	// IPv4, UDP
	EXPECT_THROW(
		net_socket s1(net_socket::network_protocol::IPv4, net_socket::transport_protocol::UDP),
		invalid_argument
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
		invalid_argument
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
		invalid_argument
		);

	// Transport protocol
	net_socket::transport_protocol tproto = net_socket::transport_protocol::UDP;
	EXPECT_THROW(s.set_transport_protocol(tproto), invalid_argument); // For now
	//EXPECT_EQ(s.get_transport_protocol(), tproto);                  // until UDP supported
	tproto = net_socket::transport_protocol::TCP;
	s.set_transport_protocol(tproto);
	EXPECT_EQ(s.get_transport_protocol(), tproto);
	EXPECT_THROW(
		s.set_transport_protocol(static_cast<net_socket::transport_protocol>(99)),
		invalid_argument
		);

	// Backlog
	int bl = s.get_backlog();
	s.set_backlog(bl+10);
	EXPECT_EQ(s.get_backlog(), bl+10);
	ASSERT_THROW(s.set_backlog(-10), invalid_argument);

	// Timeout
	double to = s.get_timeout();
	s.set_timeout(to+1.5);
	EXPECT_EQ(s.get_timeout(), to+1.5);
	EXPECT_TRUE(s.timeout_is_set());
	s.clear_timeout();
	EXPECT_FALSE(s.timeout_is_set());
	EXPECT_EQ(s.get_timeout(), 0.0);
	ASSERT_THROW(s.set_timeout(-1.0), invalid_argument);
	s.set_timeout(1.0);
	EXPECT_TRUE(s.timeout_is_set());
	s.set_timeout(0.0);
	EXPECT_FALSE(s.timeout_is_set());

	// Receive size
	size_t rs = s.get_recv_size();
	s.set_recv_size(rs+100);
	EXPECT_EQ(s.get_recv_size(), rs+100);
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
	// Until UDP supported
	EXPECT_THROW(s1.set_transport_protocol(net_socket::transport_protocol::UDP), invalid_argument);
	s1.set_backlog(s1.get_backlog()+10);
	s1.set_timeout(s1.get_timeout()+2.3);
	s1.set_recv_size(s1.get_recv_size()+100);
	// No way to change these items for an unopened socket
	//  - socket descriptor
	//  - passively opened flag
	//  - connected flag
	EXPECT_NE(s0.get_network_protocol(), s1.get_network_protocol());
	//EXPECT_NE(s0.get_transport_protocol(), s1.get_transport_protocol());
	EXPECT_EQ(s0.get_transport_protocol(), s1.get_transport_protocol()); // Until UDP supported
	EXPECT_NE(s0.get_backlog(), s1.get_backlog());
	EXPECT_NE(s0.timeout_is_set(), s1.timeout_is_set());
	EXPECT_NE(s0.get_timeout(), s1.get_timeout());
	EXPECT_NE(s0.get_recv_size(), s1.get_recv_size());

	s0 = s1;
	EXPECT_EQ(s0.get_socket_descriptor(), s1.get_socket_descriptor());
	EXPECT_EQ(s0.get_network_protocol(), s1.get_network_protocol());
	EXPECT_EQ(s0.get_transport_protocol(), s1.get_transport_protocol());
	EXPECT_EQ(s0.is_passively_opened(), s1.is_passively_opened());
	EXPECT_EQ(s0.get_backlog(), s1.get_backlog());
	EXPECT_EQ(s0.is_connected(), s1.is_connected());
	EXPECT_EQ(s0.get_timeout(), s1.get_timeout());
	EXPECT_EQ(s0.timeout_is_set(), s1.timeout_is_set());
	EXPECT_EQ(s0.get_recv_size(), s1.get_recv_size());
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

	// Invalid functions on an unopened socket
	net_socket s0;
	char arr[50] = {0, 1, 2, 3, 4, 5, 6};
	vector<char> vec = {'a', 'b'};
	string str = "YZ";
	EXPECT_THROW(s0.accept(), runtime_error);
	EXPECT_THROW(s0.send(arr, 1), runtime_error);
	EXPECT_THROW(s0.send(vec, 1), runtime_error);
	EXPECT_THROW(s0.send(str, 1), runtime_error);
	EXPECT_THROW(s0.send_all(arr, 1), runtime_error);
	EXPECT_THROW(s0.send_all(vec), runtime_error);
	EXPECT_THROW(s0.send_all(str), runtime_error);
	EXPECT_THROW(s0.recv(arr, 1), runtime_error);
	EXPECT_THROW(s0.recv(vec, 1), runtime_error);
	EXPECT_THROW(s0.recv(str, 1), runtime_error);
	EXPECT_THROW(s0.recv_all(arr, 1), runtime_error);
	EXPECT_THROW(s0.recv_all(vec, 1), runtime_error);
	EXPECT_THROW(s0.recv_all(vec), runtime_error);
	EXPECT_THROW(s0.recv_all(str, 1), runtime_error);
	EXPECT_THROW(s0.recv_all(str), runtime_error);
}

TEST( NetSocket, DestructorTests ){
	unsigned short port = get_random_port();
	thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c0 = create_connected_client(port);

	int sd = c0->get_socket_descriptor();

	// Check that a send works
	ASSERT_GT(::send(sd, &sd, sizeof(sd), 0), 0);

	// Desctructor should close the socket
	c0.reset();

	// Send should return -1 on a closed socket
	ssize_t ret = ::send(sd, &sd, sizeof(sd), 0);
	EXPECT_EQ(ret, -1);

	st.join();
}

TEST( NetSocket, CloseTests ) {
	unsigned short port = get_random_port();
	thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c0 = create_connected_client(port);

	int sd = c0->get_socket_descriptor();

	// Check that a send works
	ssize_t amt = ::send(sd, &sd, sizeof(sd), 0);
	ASSERT_GT(amt, 0);

	// Close the socket
	c0->close();
	EXPECT_FALSE(c0->is_connected());
	EXPECT_EQ(c0->get_socket_descriptor(), -1);

	// Send should return -1 on a closed socket
	ssize_t ret = ::send(sd, &sd, sizeof(sd), 0);
	EXPECT_EQ(ret, -1);

	st.join();

	net_socket s;
	s.listen("localhost", 9000);
	s.close();
	EXPECT_FALSE(s.is_passively_opened());
	EXPECT_EQ(s.get_socket_descriptor(), -1);
}

TEST( NetSocket, SendRecvTests ) {
	unsigned short port = get_random_port();
	std::thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c = create_connected_client(port);

	vector<char> tx_data;
	for( unsigned int i = 0; i < c->get_recv_size(); ++i ){
		tx_data.push_back(rand()%256);
	}
	vector<char> rx_data;

	// Send and recv the same data
	ASSERT_EQ(c->send(tx_data), c->get_recv_size());
	ASSERT_EQ(c->recv(rx_data), c->get_recv_size());
	EXPECT_EQ(rx_data, tx_data);

	st.join();

	// Server is now closed
	// Requesting 0 bytes should not close socket
	EXPECT_TRUE(c->is_connected());
	char cdata;
	size_t size;
	size = c->recv(&cdata, 0);
	EXPECT_TRUE(c->is_connected());
	EXPECT_EQ(size, 0);
	size = c->recv(&cdata, 1);
	EXPECT_FALSE(c->is_connected());
	EXPECT_EQ(c->get_socket_descriptor(), -1);
	EXPECT_EQ(size, 0);
}

TEST(NetSocket, UnequalSendRecvTests ) {
	unsigned short port = get_random_port();
	thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c = create_connected_client(port);

	vector<char> tx_data;
	for( int i = 0; i < 10000; ++i ){
		tx_data.push_back(rand()%256);
	}
	vector<char> rx_data;

	// Send and receive vector
	ssize_t ss = c->send(tx_data);
	ASSERT_NE(ss, c->get_recv_size());
	ssize_t rs = c->recv(rx_data);
	EXPECT_EQ(rs, c->get_recv_size());
	EXPECT_EQ(rs, rx_data.size());
	EXPECT_NE(rx_data.size(), tx_data.size());
	EXPECT_NE(rx_data, tx_data);
	tx_data.resize(rs);
	EXPECT_EQ(rx_data, tx_data);

	st.join();
}

TEST(NetSocket, SendAllRecvAllTests ) {
	unsigned short port = get_random_port();
	std::thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c = create_connected_client(port);

	size_t tx_size = 10000;
	vector<char> tx_vec;
	char tx_arr[tx_size];
	string tx_str;
	char val;
	for( unsigned int i = 0; i < tx_size; ++i ){
		val = static_cast<char>(rand()%256);
		tx_vec.push_back(val);
		tx_arr[i] = val;
		tx_str.push_back(val);
	}
	vector<char> rx_vec;
	char rx_arr[tx_size];
	string rx_str;

	// Send and receive vector
	ASSERT_EQ(c->send_all(tx_vec), tx_size);
	EXPECT_EQ(c->recv_all(rx_vec, tx_size), tx_size);
	EXPECT_EQ(tx_vec.size(), rx_vec.size());
	EXPECT_EQ(tx_vec, rx_vec);

	// Send array and receive vector with defined size
	rx_vec.clear();
	rx_vec.resize(tx_size);
	ASSERT_NE(rx_vec, tx_vec);
	ASSERT_EQ(c->send_all(tx_arr, tx_size), tx_size);
	EXPECT_EQ(c->recv_all(rx_vec), tx_size);
	EXPECT_EQ(tx_vec.size(), rx_vec.size());
	EXPECT_EQ(tx_vec, rx_vec);

	// Send string and receive char array
	ASSERT_EQ(c->send_all(tx_str), tx_size);
	EXPECT_EQ(c->recv_all(rx_arr, tx_size), tx_size);
	EXPECT_EQ(memcmp(tx_vec.data(), rx_arr, tx_size), 0);

	// Send string and receive string
	ASSERT_NE(tx_str, rx_str);
	ASSERT_EQ(c->send_all(tx_str), tx_size);
	EXPECT_EQ(c->recv_all(rx_str, tx_size), tx_size);
	EXPECT_EQ(tx_str.size(), rx_str.size());
	EXPECT_EQ(tx_str, rx_str);

	// Send string and receive string with defined size
	rx_str.clear();
	rx_str.resize(tx_size);
	ASSERT_NE(tx_str, rx_str);
	ASSERT_EQ(c->send_all(tx_str), tx_size);
	EXPECT_EQ(c->recv_all(rx_str), tx_size);
	EXPECT_EQ(tx_str.size(), rx_str.size());
	EXPECT_EQ(tx_str, rx_str);

	st.join();
}

TEST(NetSocket, PacketErrorSendTests ) {
	unsigned short port = get_random_port();
	std::thread st = spawn_and_check_server(check_and_echo_server, port);
	unique_ptr<net_socket> c = create_connected_client(port);

	char a = 'A';
	const unsigned int drop_prob = 15; // 15%
	const unsigned int pkt_count = 10000;

	// Send pkt_count copies
	for( unsigned int i = 0; i < pkt_count; ++i ) {
		c->packet_error_send(&a, 1);
	}

	// Receive as much data as possible
	vector<char> rx;
	c->set_timeout(0.1);
	try{
		c->recv_all(rx, pkt_count);
	}
	catch( timeout_exception& ){} // Do nothing

	// Specified drop probability +/- 1%
	EXPECT_LT(rx.size(), pkt_count*(100-drop_prob)/100.0*1.01);
	EXPECT_GT(rx.size(), pkt_count*(100-drop_prob)/100.0*0.99);

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
	worker->set_timeout(0.1); // Quit if no data arrives in 100 ms

	vector<char> data;
	try{
		worker->recv(data);

		while( !data.empty() ) {
			worker->send(data);
			worker->recv(data);
		}
	}
	catch( timeout_exception& ){ } // Do nothing with timeout
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
