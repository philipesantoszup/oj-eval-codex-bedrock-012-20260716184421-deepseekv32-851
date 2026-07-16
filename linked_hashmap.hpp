/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include <cstring>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

private:
    static const size_t INITIAL_CAPACITY = 16;
    static const double LOAD_FACTOR;
    
    struct Node {
        value_type data;
        Node* next_in_bucket;
        Node* prev_in_bucket;
        Node* next_in_list;
        Node* prev_in_list;
        
        Node(const value_type& val) : data(val), next_in_bucket(nullptr), 
                                     prev_in_bucket(nullptr), next_in_list(nullptr),
                                     prev_in_list(nullptr) {}
    };
    
    Node** buckets;
    size_t bucket_capacity;
    Node* head;
    Node* tail;
    size_t element_count;
    Hash hasher;
    Equal equal;
    
    size_t get_bucket_index(const Key& key) const {
        return hasher(key) % bucket_capacity;
    }
    
    void rehash(size_t new_capacity) {
        Node** new_buckets = new Node*[new_capacity];
        std::memset(new_buckets, 0, sizeof(Node*) * new_capacity);
        
        for (size_t i = 0; i < bucket_capacity; i++) {
            Node* node = buckets[i];
            while (node) {
                Node* next_node = node->next_in_bucket;
                size_t new_index = hasher(node->data.first) % new_capacity;
                
                node->next_in_bucket = new_buckets[new_index];
                node->prev_in_bucket = nullptr;
                if (new_buckets[new_index]) {
                    new_buckets[new_index]->prev_in_bucket = node;
                }
                new_buckets[new_index] = node;
                
                node = next_node;
            }
        }
        
        delete[] buckets;
        buckets = new_buckets;
        bucket_capacity = new_capacity;
    }
    
    void ensure_capacity() {
        if (element_count >= bucket_capacity * LOAD_FACTOR) {
            rehash(bucket_capacity * 2);
        }
    }
    
    void cleanup() {
        Node* current = head;
        while (current) {
            Node* next = current->next_in_list;
            delete current;
            current = next;
        }
        head = tail = nullptr;
        element_count = 0;
        // Clear bucket pointers
        if (buckets) {
            std::memset(buckets, 0, sizeof(Node*) * bucket_capacity);
        }
    }
    
    Node* find_node(const Key& key) const {
        if (bucket_capacity == 0) return nullptr;
        
        size_t index = get_bucket_index(key);
        Node* node = buckets[index];
        
        while (node) {
            if (equal(node->data.first, key)) {
                return node;
            }
            node = node->next_in_bucket;
        }
        return nullptr;
    }

public:
	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node* current;
        const linked_hashmap* container;
        
        friend class linked_hashmap;
        friend class const_iterator;
        
        iterator(Node* node, const linked_hashmap* cont) : current(node), container(cont) {}

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : current(nullptr), container(nullptr) {}
		iterator(const iterator &other) : current(other.current), container(other.container) {}
        
        iterator& operator=(const iterator& other) {
            current = other.current;
            container = other.container;
            return *this;
        }
        
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
            if (!container) throw invalid_iterator();
            if (!current) throw invalid_iterator();
            iterator temp = *this;
            current = current->next_in_list;
            return temp;
        }
        
		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
            if (!container) throw invalid_iterator();
            if (!current) throw invalid_iterator();
            current = current->next_in_list;
            return *this;
        }
        
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
            if (!container) throw invalid_iterator();
            iterator temp = *this;
            if (!current) {
                if (container->tail) {
                    current = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else {
                if (!current->prev_in_list) throw invalid_iterator();
                current = current->prev_in_list;
            }
            return temp;
        }
        
		/**
		 * TODO --iter
		 */
		iterator & operator--() {
            if (!container) throw invalid_iterator();
            if (!current) {
                if (container->tail) {
                    current = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else {
                if (!current->prev_in_list) throw invalid_iterator();
                current = current->prev_in_list;
            }
            return *this;
        }
        
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
            if (!current) throw invalid_iterator();
            return current->data;
        }
        
		bool operator==(const iterator &rhs) const {
            return current == rhs.current;
        }
        
		bool operator==(const const_iterator &rhs) const {
            return current == rhs.current;
        }
        
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }
        
		bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }

		/**
		 * for the support of it->first. 
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const noexcept {
            if (!current) return nullptr;
            return &(current->data);
        }
	};
 
	class const_iterator {
	private:
        const Node* current;
        const linked_hashmap* container;
        
        friend class iterator;
        
	public:
        const_iterator(const Node* node, const linked_hashmap* cont) : current(node), container(cont) {}
        
		const_iterator() : current(nullptr), container(nullptr) {}
		const_iterator(const const_iterator &other) : current(other.current), container(other.container) {}
		const_iterator(const iterator &other) : current(other.current), container(other.container) {}
        
        const_iterator& operator=(const const_iterator& other) {
            current = other.current;
            container = other.container;
            return *this;
        }
        
        const_iterator operator++(int) {
            if (!container) throw invalid_iterator();
            if (!current) throw invalid_iterator();
            const_iterator temp = *this;
            current = current->next_in_list;
            return temp;
        }
        
        const_iterator& operator++() {
            if (!container) throw invalid_iterator();
            if (!current) throw invalid_iterator();
            current = current->next_in_list;
            return *this;
        }
        
        const_iterator operator--(int) {
            if (!container) throw invalid_iterator();
            const_iterator temp = *this;
            if (!current) {
                if (container->tail) {
                    current = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else {
                if (!current->prev_in_list) throw invalid_iterator();
                current = current->prev_in_list;
            }
            return temp;
        }
        
        const_iterator& operator--() {
            if (!container) throw invalid_iterator();
            if (!current) {
                if (container->tail) {
                    current = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else {
                if (!current->prev_in_list) throw invalid_iterator();
                current = current->prev_in_list;
            }
            return *this;
        }
        
        const value_type& operator*() const {
            if (!current) throw invalid_iterator();
            return current->data;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return current == rhs.current;
        }
        
        bool operator==(const iterator &rhs) const {
            return current == rhs.current;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
        
        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }
        
        const value_type* operator->() const noexcept {
            if (!current) return nullptr;
            return &(current->data);
        }
	};
 
	/**
	 * TODO two constructors
	 */
	linked_hashmap() : head(nullptr), tail(nullptr), element_count(0), hasher(), equal() {
        bucket_capacity = INITIAL_CAPACITY;
        buckets = new Node*[bucket_capacity];
        std::memset(buckets, 0, sizeof(Node*) * bucket_capacity);
    }
    
	linked_hashmap(const linked_hashmap &other) : head(nullptr), tail(nullptr), 
                                                element_count(0), hasher(other.hasher),
                                                equal(other.equal) {
        bucket_capacity = other.bucket_capacity;
        buckets = new Node*[bucket_capacity];
        std::memset(buckets, 0, sizeof(Node*) * bucket_capacity);
        
        // Copy all nodes from other
        const Node* other_node = other.head;
        while (other_node) {
            insert(other_node->data);
            other_node = other_node->next_in_list;
        }
    }
 
	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
        if (this == &other) return *this;
        
        // Clear current data
        cleanup();
        
        // Delete old buckets if they exist
        if (buckets) {
            delete[] buckets;
        }
        
        // Copy parameters
        bucket_capacity = other.bucket_capacity;
        hasher = other.hasher;
        equal = other.equal;
        
        // Create new buckets
        buckets = new Node*[bucket_capacity];
        std::memset(buckets, 0, sizeof(Node*) * bucket_capacity);
        
        // Copy nodes
        const Node* other_node = other.head;
        while (other_node) {
            insert(other_node->data);
            other_node = other_node->next_in_list;
        }
        
        return *this;
    }
 
	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
        cleanup();
        if (buckets) {
            delete[] buckets;
            buckets = nullptr;
        }
    }
 
	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
        Node* node = find_node(key);
        if (!node) throw index_out_of_bound();
        return node->data.second;
    }
    
	const T & at(const Key &key) const {
        const Node* node = find_node(key);
        if (!node) throw index_out_of_bound();
        return node->data.second;
    }
 
	/**
	 * TODO
	 * access specified element 
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
        Node* node = find_node(key);
        if (node) {
            return node->data.second;
        }
        
        // Insert new element
        value_type new_pair(key, T());
        auto result = insert(new_pair);
        return (*(result.first)).second;
    }
 
	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
        return at(key);
    }
 
	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
        return iterator(head, this);
    }
    
	const_iterator cbegin() const {
        return const_iterator(head, this);
    }
 
	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
        return iterator(nullptr, this);
    }
    
	const_iterator cend() const {
        return const_iterator(nullptr, this);
    }
 
	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
        return element_count == 0;
    }
 
	/**
	 * returns the number of elements.
	 */
	size_t size() const {
        return element_count;
    }
 
	/**
	 * clears the contents
	 */
	void clear() {
        cleanup();
    }
 
	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion), 
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
        ensure_capacity();
        
        Node* existing = find_node(value.first);
        if (existing) {
            return pair<iterator, bool>(iterator(existing, this), false);
        }
        
        // Create new node
        Node* new_node = new Node(value);
        element_count++;
        
        // Add to hash table
        size_t index = get_bucket_index(value.first);
        new_node->next_in_bucket = buckets[index];
        if (buckets[index]) {
            buckets[index]->prev_in_bucket = new_node;
        }
        buckets[index] = new_node;
        
        // Add to linked list (at the end)
        if (!head) {
            head = tail = new_node;
        } else {
            tail->next_in_list = new_node;
            new_node->prev_in_list = tail;
            tail = new_node;
        }
        
        return pair<iterator, bool>(iterator(new_node, this), true);
    }
 
	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
        if (!pos.container || pos.container != this || !pos.current) {
            throw invalid_iterator();
        }
        
        Node* node = pos.current;
        
        // Remove from hash table bucket
        size_t index = get_bucket_index(node->data.first);
        
        if (node->prev_in_bucket) {
            node->prev_in_bucket->next_in_bucket = node->next_in_bucket;
        } else {
            buckets[index] = node->next_in_bucket;
        }
        
        if (node->next_in_bucket) {
            node->next_in_bucket->prev_in_bucket = node->prev_in_bucket;
        }
        
        // Remove from linked list
        if (node->prev_in_list) {
            node->prev_in_list->next_in_list = node->next_in_list;
        } else {
            head = node->next_in_list;
        }
        
        if (node->next_in_list) {
            node->next_in_list->prev_in_list = node->prev_in_list;
        } else {
            tail = node->prev_in_list;
        }
        
        delete node;
        element_count--;
    }
 
	/**
	 * Returns the number of elements with key 
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0 
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
        return find_node(key) ? 1 : 0;
    }
 
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
        Node* node = find_node(key);
        if (!node) return end();
        return iterator(node, this);
    }
    
	const_iterator find(const Key &key) const {
        const Node* node = find_node(key);
        if (!node) return cend();
        return const_iterator(node, this);
    }
};

template<class Key, class T, class Hash, class Equal>
const double linked_hashmap<Key, T, Hash, Equal>::LOAD_FACTOR = 0.75;

}

#endif
