#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <unordered_map>
#include <iostream>
#include <list>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # LRU list
 *
 *
 */

class Entry{
public:
    std::string key;
    std::string value;
    Entry* next;
    Entry* prev;

    //Moves node to the head
    void putFront(Entry* &head, Entry* &tail) {
    	if(prev)
			prev->next  = next;
		else
			return;
		
		if(next)
			next->prev  = prev;
		else
			tail = prev;

		next        = head;
		head->prev  = this;
		head        = this;
		head->prev  = nullptr;
    }
};

/**
 * # Map based implementation with global lock
 *
 *
 */
class MapBasedGlobalLockImpl : public Afina::Storage {
public:
    MapBasedGlobalLockImpl(size_t max_size = 1024) : _max_size(max_size) {
        _head = nullptr;
        _tail = _head;

        _current_size = 0;
    }
    
    ~MapBasedGlobalLockImpl() {}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:
    size_t _max_size;
    size_t _current_size;
    mutable std::mutex op_mutex;
    mutable std::unordered_map<std::string, Entry*> _backend;
    mutable Entry* _head;
    mutable Entry* _tail;

    // Deletes least recently used element
    void deleteLRU();
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
