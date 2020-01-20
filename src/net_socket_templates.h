#include <cstring>

namespace network_socket {

using std::vector;

template<typename T>
ssize_t net_socket::send(const vector<T> &data, size_t max_size) const {
	if( max_size == 0 ) {
		max_size = data.size()*sizeof(T);
	}

	return send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::packet_error_send(const vector<T> &data, size_t max_size) const {
	if( max_size == 0 ) {
		max_size = data.size()*sizeof(T);
	}

	return packet_error_send(data.data(), max_size);
}

template<typename T>
ssize_t net_socket::send_all(const vector<T> &data) const {
	return send_all(data.data(), data.size()*sizeof(T));
}

template<typename T>
ssize_t net_socket::recv(vector<T> &data, size_t max_size) {
	if( max_size == 0 ) {
		if( data.empty() ) {
			data.resize(_recv_size);
			max_size = _recv_size;
		}
		else {
			max_size = data.size()*sizeof(T);
		}
	}
	else {
		data.resize(max_size);
	}

	ssize_t ss = recv(data.data(), max_size);
	if( ss >= 0 ) {
		data.resize(ss);
	}

	return ss;
}

template<typename T>
ssize_t net_socket::recv_all(vector<T> &data, size_t exact_size) {
	if( exact_size == 0 ) {
		if( data.empty() ) {
			data.resize(_recv_size);
			exact_size = _recv_size;
		}
		else {
			exact_size = data.size()*sizeof(T);
		}
	}
	else {
		data.resize(exact_size);
	}

	ssize_t ss = recv_all(data.data(), exact_size);
	if( ss >= 0 ) {
		data.resize(ss);
	}

	return ss;
}

} // namespace network_socket
