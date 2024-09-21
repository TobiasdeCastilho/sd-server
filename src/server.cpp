#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <map>

#include "out.cpp"

enum Protocol {
	TCP,
	UDP
};

struct socket {
	int id, name, bind;
	unsigned int len;
	struct sockaddr_in addr;
};

struct Connection {
	unsigned int id;
	void * server;
};

class Server {
	private:
		std::map<int, pthread_t> connections;		
		bool listening = false;
		pthread_t listen_thread;
		unsigned int max_connections = 0;
		struct socket sock;				

		void (*server_function) (int) = NULL;
	public:
		Server(unsigned int port, Protocol _p) __THROW;
		~Server();		

		bool add_connection(int id) __THROW;		
		bool run_connection(int id) __THROW;

		bool listen_start(unsigned int max) __THROW;
		bool listen_stop() __THROW;		

		void set_server_function(void (*_f) (int)) __THROW;				
};

Server::Server(unsigned int port, Protocol _p) __THROW {
	_INFO << "Starting " << (_p == TCP ? "TCP" : "UDP") << " server" << std::endl;

	this->sock.id = socket(AF_INET, _p == TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	if (this->sock.id < 0) {
		_ERROR << "Unable to open socket stream" << std::endl;
		exit(1);
	}

	this->sock.addr.sin_family = AF_INET;
	this->sock.addr.sin_addr.s_addr = INADDR_ANY;
	if (port <= 0) {
		_ERROR << "Invalid port: " << port << std::endl;
		close(this->sock.id);
		exit(1);
	}
	
	this->sock.addr.sin_port = htons(port);
	this->sock.len = sizeof(this->sock.addr);
	
	this->sock.bind = bind(this->sock.id, (struct sockaddr *)&this->sock.addr, this->sock.len);
	if(this->sock.bind < 0) {
		_ERROR << "Bind failed" << std::endl;
		close(this->sock.id);
		exit(1);
	}

	this->sock.name = getsockname(this->sock.id, (struct sockaddr *)&this->sock.addr, &(this->sock.len));
	if(this->sock.name < 0) {
		_ERROR << "Unable to get socket name" << std::endl;
		close(this->sock.id);
		exit(1);
	}

	_INFO << "Server started on port " << port << std::endl;
}

Server::~Server() {
	close(this->sock.id);
}

bool Server::add_connection(int id) __THROW {
	if(this->connections.find(id) != this->connections.end()) {
		_ERROR << "Connection already exists" << std::endl;
		return false;
	}

	if(this->max_connections && this->connections.size() >= this->max_connections) {
		_ERROR << "Max connections reached" << std::endl;
		return false;
	}

	struct Connection connection;
	connection.id = id;
	connection.server = (void *) this;
	this->connections.insert(std::pair<int, pthread_t>(id, 0));

	pthread_create(&this->connections[id], NULL, [](void* _cn) -> void* {
			struct Connection _c = *(struct Connection*) _cn;
			Server* _s = (Server*) _c.server;
			
			_s->run_connection(_c.id);

			return NULL;
	}, (void *) &connection);

	return true;
}

bool Server::run_connection(int id) __THROW {	
	this->server_function(id);	

	pthread_join(this->connections[id], NULL);	
	this->connections.erase(id);
	close(id);	
	return true;
}

bool Server::listen_start(unsigned int max) __THROW { 
	this->max_connections = max;	

	if(this->server_function == NULL){
		_ERROR << "Server function not defined" << std::endl;
		return false;
	}
	
	listen(this->sock.id, this->max_connections);

	pthread_create(&this->listen_thread, NULL, 
		[](void* _this) -> void* {
			Server* _server = (Server*)_this;
			_server->listening = true;
			
			struct sockaddr_in client;
			unsigned int client_len = sizeof(client);

			while(_server->listening) {				
				int connection = accept(_server->sock.id, (struct sockaddr *)&client, &client_len);
				if(connection < 0) continue;

				if (!_server->add_connection(connection)) {
					_ERROR << "Unable to add connection" << std::endl;
					close(connection);
					continue;
				}														
			}

			return NULL;
	}, this);

	_INFO << "Server listening" << std::endl;
	return true;
}

bool Server::listen_stop() __THROW {
	_INFO << "Stopping server" << std::endl;	
	
	this->listening = false;
	pthread_detach(this->listen_thread);

	shutdown(this->sock.id, SHUT_RDWR);
	_INFO << "Server stopped" << std::endl;
	return true;
}

void Server::set_server_function(void (*_f) (int))  __THROW {
	if(this->listening) {
		_ERROR << "Server is already listening" << std::endl;
		return;
	}
	
	this->server_function = _f;
}	