#include "MapBasedGlobalLockImpl.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
	op_mutex.lock();

	//Check if the element can be put
	if(key.size() + value.size() > _max_size){
		std::cerr << "Too large" << std::endl;
		op_mutex.unlock();
		return false;
	}

	std::map<std::string, Entry*>::iterator it = _backend.find(key);
	if(it == _backend.end()){
		//Element does't exist
		//Clear space for the element
		_current_size += key.size() + value.size();
		deleteLRU();

		//Create new node and put it at the head
		Entry* node = new Entry();
		node->next  = _head;
		node->prev  = nullptr;
		if(_head) _head->prev = node;
		node->key   = key;
		node->value = value;
		_head       = node;
		if(!_tail) _tail = node;

		//Add element to the dict
		_backend[key] = node;

		op_mutex.unlock();
		return true;
	}
	else{
		//Element exists
		//Clear space for the element
		_current_size += value.size() - (it->second->value).size();
		deleteLRU();
		
		//Change value
		it->second->value = value;
		it->second->putFront(_head, _tail);
		
		op_mutex.unlock();
		return true;
	}
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) { 
	op_mutex.lock();

	std::map<std::string, Entry*>::iterator it = _backend.find(key);
	if(it == _backend.end()){
		//Check if the element can be put
		if(key.size() + value.size() > _max_size){
			std::cerr << "Too large" << std::endl;
			op_mutex.unlock();
			return false;
		}

		//Clear space for the element
		_current_size += key.size() + value.size();
		deleteLRU();

		//Create new node and put it at the head
		Entry* node = new Entry();
		node->next  = _head;
		node->prev  = nullptr;
		if(_head) _head->prev = node;
		node->key   = key;
		node->value  = value;
		_head       = node;
		if(!_tail) _tail = node;

		//Add element to the dict
		_backend[key] = node;

		op_mutex.unlock();
		return true;
	}

	op_mutex.unlock();
	return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) { 
	op_mutex.lock();

	//Check if the element exists
	std::map<std::string, Entry*>::iterator it = _backend.find(key);
	if(it == _backend.end()){
		op_mutex.unlock();
		return false;
	}

	//Check if the element can be put
	if(key.size() + value.size() > _max_size){
		std::cerr << "Too large" << std::endl;
		op_mutex.unlock();
		return false;
	}

	//Clear space for the element
	_current_size += value.size() - (it->second->value).size();
	deleteLRU();
	
	//Change value
	it->second->value = value;
	it->second->putFront(_head, _tail);

	op_mutex.unlock();
	return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) { 
	op_mutex.lock();

	//Check if the element exists
	std::map<std::string, Entry*>::iterator it = _backend.find(key);
	if(it == _backend.end()){
		op_mutex.unlock();
		return false;
	}

	//Delete node from the dict and the list
	Entry* node = it->second;
	_backend.erase(key);

	if(node->prev)
		node->prev->next  = node->next;
	else
		_head = node->next;
		
	if(node->next)
		node->next->prev  = node->prev;
	else
		_tail = node->prev;

	_current_size -= (node->value).size() + key.size();
	delete node;

	op_mutex.unlock();
	return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const { 
	op_mutex.lock();

	//Check if the element exists
	std::map<std::string, Entry*>::iterator it = _backend.find(key);
	if(it == _backend.end()){
		op_mutex.unlock();
		return false;
	}

	//Change value
	value = it->second->value;
	it->second->putFront(_head, _tail);

	op_mutex.unlock();
	return true; 
}

void MapBasedGlobalLockImpl::deleteLRU() {
	while(_current_size > _max_size){
		Entry* node = _tail;
		_tail       = _tail->prev;
		if(_tail) 
			_tail->next = nullptr;
		else
			_head = nullptr;
 
		_current_size -= (node->key).size() + (node->value).size();
		_backend.erase(node->key);
		delete node;
	}
} 

} // namespace Backend
} // namespace Afina
