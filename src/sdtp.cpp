#include <regex>
#include <string>
#include <map>

#include "out.cpp"

namespace SDTP {
	enum DataType {
		Number,
		String,
		Bool,
	};

	struct _Data {	
		DataType type;
		char * name;
		void * value;
	};	
	typedef struct _Data Data;

	struct _Sensor {
		char * name;
		int data_size;
		Data ** data;
	};
	typedef struct _Sensor Sensor;

	class Server {
		private: 
			std::map <std::string, Sensor *> _sensors;
			std::map <std::string, Sensor *> public_sensors;

			std::string
				__new_sensor = "[_a-zA-Z][_a-zA-Z0-9]*\\/(([_a-zA-Z][_a-zA-Z0-9]*\\[(str|num|bin)\\])(&|$))+",
				__add_data = "[a-zA-Z0-9]+\\/([a-zA-Z0-9\\.]*(&|$))+",
				__get_data = "([a-zA-Z0-9]+(&|$))+",
				__get_defs = "([a-zA-Z0-9]+\\[?\\](&|$))+";

			bool new_sensor(std::string * value);
			bool add_data(std::string * value);			
			bool get_data(std::string * value);			
			bool get_defs(std::string * value);

			std::string generate_key(std::string name);			
		public:	
			void recieve_request(int connection);			
	};

	bool Server::new_sensor(std::string * value) {	
		Sensor * sensor = (Sensor *) malloc(sizeof(Sensor));			
		sensor->name = (char *) malloc(sizeof(char) * value->find_first_of('/'));		
		strcpy(sensor->name, value->substr(0, value->find_first_of('/')).c_str());				
		*value = value->substr(value->find_last_of('/') + 1);		
		
		int _count = 1;
		for(int i = 0; i < value->size(); i++)
			if(value->at(i) == '&')
				_count++;		

		sensor->data = (Data **) malloc(sizeof(Data) * _count);
		sensor->data_size = _count;

		std::string _data;		
		Data * data;
		_count = 0;
		do {			
			_data.clear();
			_data = value->substr(0, value->find_first_of('&'));						

			*value = value->substr(0,value->find_first_of(']'));					

			data = (Data *) malloc(sizeof(Data));		
			data->name = (char *) malloc(sizeof(char) * _data.find_first_of('['));
			strcpy(data->name, _data.substr(0, _data.find_first_of('[')).c_str());						
			
			std::string _type = _data.substr(_data.find_first_of('[') + 1, 3);	
			if(_type == "str") {
				data->type = String;
				data->value = malloc(sizeof(std::string));
				*((std::string *) data->value) = "";
			} else if(_type == "num") {
				data->type = Number;
				data->value = malloc(sizeof(float));
				*((float *) data->value) = 0;
			} else if(_type == "bin") {
				data->type = Bool;
				data->value = malloc(sizeof(bool));
				((bool *) data->value)[0] = false;
			} else {
				_ERROR << "Invalid data type" << std::endl;
				return false;
			}													

			sensor->data[_count] = data;
			_count++;
		} while (value->find_first_of('&') != std::string::npos);		
		
		std::string _priv = generate_key(sensor->name), _pub = generate_key(sensor->name);		
		this->_sensors[_priv] = sensor;				
		this->public_sensors[_pub] = sensor;								
		
		value->clear();		
		value->append("/priv[" + _priv + "]&pub[" + _pub + "]");		

		return true;
	};

	bool Server::add_data(std::string * value) {

		std::string _sensor = value->substr(0, value->find_first_of('/'));
		value->erase(0, value->find_first_of('/') + 1);

		if(this->_sensors.find(_sensor) == this->_sensors.end()) {
			_ERROR << "Sensor " << _sensor << " not found" << std::endl;
			return false;
		}

		Sensor * sensor = this->_sensors[_sensor];
		for(int i = 0; i < sensor->data_size; i ++){
			std::string _data = value->substr(0, value->find_first_of('&'));
			value->erase(0, value->find_first_of('&') + 1);

			switch(sensor->data[i]->type){
				case String:
					*((std::string *) sensor->data[i]->value) = _data;
					break;
				case Number:
					*((float *) sensor->data[i]->value) = std::stof(_data);
					break;
				case Bool:
					((bool *) sensor->data[i]->value)[0] = _data == "true" ? true : false;
					break;
			}


		}		

		value->clear();
		value->append("ok");

		return true;
	};

	bool Server::get_data(std::string * value) {
		if(value->size() <= 1){
			value->clear();
			for(auto it = this->public_sensors.begin(); it != this->public_sensors.end(); it++) 
			value->append(std::string(it->second->name) + "[" + it->first + "]");										

			return true;
		};

		std::string _response = "", _data = "";
		do {
			std::string _sensor = value->substr(0, value->find_first_of('['));
			value->erase(0, value->find_first_of('[') + 1);

			std::string _data = value->substr(0, value->find_first_of(']'));
			value->erase(0, value->find_first_of(']') + 1);

			if(_response.size())
				_response.append("&");

			
			if(this->public_sensors.find(_sensor) == this->public_sensors.end()) {
				_ERROR << "Sensor " << _sensor << " not found" << std::endl;
				_response.append("not_found[]");				
				continue;
			}

			Sensor * sensor = this->public_sensors[_sensor];			
			
			_data.clear();
			for(int i = 0; i < sensor->data_size; i++) {				
				if(_data.size())
					_data.append("&");				
					
				switch(sensor->data[i]->type) {
					case String:
						_data.append(*((std::string *) sensor->data[i]->value));
						break;
					case Number:
						_data.append(std::to_string(*((int *) sensor->data[i]->value)));
						break;
					case Bool:
						_data.append(*((bool *) sensor->data[i]->value) ? "true" : "false");
						break;
				}				
			}

			_response.append(_sensor + "[" + _data + "]");			
		} while (value->find_first_of('&') != std::string::npos);

		value->clear();
		value->append(_response);

		return true;
	};

	bool Server::get_defs(std::string	* value) {
		std::string _response = "", _data = "";
		do {
			std::string _sensor = value->substr(0, value->find_first_of('['));
			value->erase(0, value->find_first_of('[') + 1);

			std::string _data = value->substr(0, value->find_first_of(']'));
			value->erase(0, value->find_first_of(']') + 1);

			if(_response.size())
				_response.append("&");

			if(this->public_sensors.find(_sensor) == this->public_sensors.end()) {
				_ERROR << "Sensor " << _sensor << " not found" << std::endl;
				_response.append("not_found[]");				
				continue;
			}

			Sensor * sensor = this->public_sensors[_sensor];			
			
			_data.clear();
			for(int i = 0; i < sensor->data_size; i++) {				
				if(_data.size())
					_data.append("&");				
					
				switch(sensor->data[i]->type) {
					case String:
						_data.append(sensor->data[i]->name + ":str");
						break;
					case Number:
						_data.append(sensor->data[i]->name + ":num");
						break;
					case Bool:
						_data.append(sensor->data[i]->name + ":bin");
						break;
				}				
			}

			_response.append(_sensor + "[" + _data + "]");			
		} while (value->find_first_of('&') != std::string::npos);

		value->clear();
		value->append(_response);

		return true;
	};

	std::string Server::generate_key(std::string name) {
		std::string _key = name;				
		int seed = rand() % name.size();		
		for(int i = 0; i < name.size() - 1; i++)
			_key[i] = _key[i] ^ seed;		

		return _key;
	};

	void Server::recieve_request(int connection) {			
		_SUCESS << "Connection " << connection << " established" << std::endl;
						
		int _recv;
		char _buffer[1000];								
		std::string buffer = "";	
		do {		
			memset(_buffer, 0, sizeof(_buffer));
			_recv = recv(connection, _buffer, 1000, 0);							
			buffer += _buffer;						
		} while (buffer.find('\n') == std::string::npos);
		buffer = buffer.substr(0, buffer.size() - 1);	

		if(buffer[0] == '/')
			buffer = buffer.substr(1);		

		_INFO << "Request: '" << buffer << "'" << std::endl;

		std::smatch _match;		
		if(std::regex_match(buffer, _match, std::regex(this->__new_sensor)))
			this->new_sensor(&buffer);
		else if(std::regex_match(buffer, _match, std::regex(this->__add_data)))
			this->add_data(&buffer);
		else if(std::regex_match(buffer, _match, std::regex(this->__get_data)) || buffer.size() == 0)
			this->get_data(&buffer);
		else if(std::regex_match(buffer, _match, std::regex(this->__get_defs)))
			this->get_defs(&buffer);
		else
			_WARNING << "Invalid request" << std::endl;			

		buffer += "\n";
		send(connection, buffer.c_str(), buffer.size(), 0);
		_SUCESS << "Connection " << connection << " closed" << std::endl;		
	};
};