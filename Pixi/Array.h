#pragma once
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <cassert>

template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
class ByteIterator {
	T* m_current;
public:
	ByteIterator(T* begin) : m_current(begin) {

	}

	T& operator*() {
		return *m_current;
	}

	ByteIterator<T> operator++(int) {
		m_current++;
		return *this;
	}

	ByteIterator<T>& operator++() {
		++m_current;
		return *this;
	}

	ByteIterator<T>& operator+(int value) {
		m_current += value;
		return *this;
	}

	ByteIterator<T>& operator+=(int value) {
		m_current += value;
		return *this;
	}

	bool operator==(const ByteIterator<T>& rhs) const { return m_current == rhs.m_current; }
	bool operator!=(const ByteIterator<T>& rhs) const { return m_current != rhs.m_current; }

};

template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
class ConstByteIterator {
	T const* m_current;

public:
	ConstByteIterator(T* begin) : m_current(begin) {
	}

	const T& operator*() const {
		return *m_current;
	}
	
	ConstByteIterator<T> operator++(int) {
		m_current++;
		return *this;
	}

	ConstByteIterator<T>& operator++() {
		++m_current;
		return *this;
	}

	ConstByteIterator<T> operator+(int value) {
		T* new_curr = const_cast<T*>(m_current) + value;
		ConstByteIterator<T> iter(new_curr);
		return iter;
	}

	ConstByteIterator<T>& operator+=(int value) {
		m_current += value;
		return *this;
	}

	bool operator==(const ConstByteIterator<T>& rhs) const { return m_current == rhs.m_current; }
	bool operator!=(const ConstByteIterator<T>& rhs) const { return m_current != rhs.m_current; }
};

template <typename T, size_t S, typename = std::enable_if_t<sizeof(T) == 1>>
class ByteArray {
	T* m_start;
	size_t m_capacity;
	size_t m_size;
public:
	ByteArray() : m_capacity(S) {
		m_start = new T[S];
		memset(m_start, 0, m_capacity);
		m_size = 0;
	}
	
	ByteArray(T val) : m_capacity(S) {
		m_start = new T[S];
		memset(m_start, val, m_capacity);
		m_size = m_capacity;
	}

	ByteArray(std::initializer_list<T> list) : m_capacity(list.size()) {
		m_start = new T[S];
		m_size = m_capacity;
		std::move(list.begin(), list.end(), start());
	}

	ByteArray(ByteArray<T, S>&& other) = delete;
	ByteArray(const ByteArray<T, S>& other) = delete;
	ByteArray<T, S>& operator=(const ByteArray<T, S>& other) = delete;

	~ByteArray() {
		delete[] m_start;
	}

	ByteArray<T, S>& operator=(ByteArray<T, S>&& other) noexcept {
		m_start = other.m_start;
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		other.m_start = nullptr;
		return *this;
	}

	void reset() {
		memset(m_start, 0, m_capacity);
	}

	// only grows bigger, not smaller
	void resize(size_t newsize) {
		assert(newsize > m_capacity);
		T* new_start = new T[newsize];
		memcpy(new_start, m_start, m_size);
		m_capacity = newsize;
		delete[] m_start;
		m_start = new_start;
	}

	void fill(T val) {
		memset(m_start, val, m_capacity);
		m_size = m_capacity;
	}

	T& operator[](size_t index) {
#ifdef DEBUG
		return at(index);
#else
		return m_start[index];
#endif
	}

	T& operator[](size_t index) const {
#ifdef DEBUG
		return at(index);
#else
		return m_start[index];
#endif
	}

	T& at(size_t index) const {
		if (index >= size()) {
			printf("[ ARRAY ] Trying to access array of size %zd with index of size %zd\n", size(), index);
			exit(-1);
		}
		return m_start[index];
	}

	ConstByteIterator<T> cbegin() const {
		return { m_start };
	}

	ConstByteIterator<T> cend() const {
		return { m_start + m_size };
	}

	ByteIterator<T> begin() {
		return { m_start };
	}

	ByteIterator<T> end() {
		return { m_start + m_size };
	}

	constexpr T* start() {
		return m_start;
	}
	
	constexpr T* start() const {
		return m_start;
	}

	constexpr T* ptr_at(size_t at) {
		return m_start + at;
	}

	constexpr T* ptr_at(size_t at) const {
		return m_start + at;
	}

	constexpr size_t size() {
		return m_size;
	}

	constexpr size_t size() const {
		return m_size;
	}

	size_t from_file(FILE* fp) {
		fseek(fp, 0, SEEK_END);
		size_t fsize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (fsize > m_capacity)
			resize(fsize);
		m_size = fsize / sizeof(T);
		return fread(m_start, sizeof(T), m_size, fp);
	}

	void write(const T* src, size_t size) {
		memcpy(m_start, src, size);
	}

	void write_at(const T* src, size_t size, size_t at) {
		memcpy(m_start + at, src, size);
	}
};


template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
class ByteArrayView {
	const T* ptr;
	const size_t size;
public:
	template <size_t S>
	ByteArrayView(const ByteArray<T, S>& arr) : ptr(arr.start()), size(arr.size()){

	}
	template <size_t S>
	ByteArrayView(const ByteArray<T, S>& arr, size_t offset) : ptr(arr.ptr_at(offset)), size(arr.size() - offset) {

	}
	ByteArrayView(const ByteArrayView<T>& other) = delete;
	ByteArrayView(ByteArrayView<T>&& other) = delete;
	ByteArrayView<T>& operator=(const ByteArrayView<T>& other) = delete;
	ByteArrayView<T>& operator=(ByteArrayView<T>&& other) = delete;

	constexpr T operator[](size_t index) const {
#ifdef DEBUG
		return at(index);
#else
		return ptr[index];
#endif
	}

	constexpr const T* ptr_at(size_t index) const {
		assert(index < size);
		return ptr + index;
	}

	constexpr const T& at(size_t index) const {
		if (index >= size) {
			printf("[ ARRAY ] Trying to access array of size %zd with index of size %zd\n", size, index);
			exit(-1);
		}
		return ptr[index];
	}

	const ConstByteIterator<T> cbegin() const {
		return { ptr };
	}

	const ConstByteIterator<T> cend() const {
		return { ptr + size };
	}
};