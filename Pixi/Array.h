#include <cstring>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include "Util.h"

template <typename T, size_t S, typename = std::enable_if_t<sizeof(T) == 1>>
class ByteArray {
	T* m_start;
	const size_t m_capacity;
	size_t m_size;
public:
	ByteArray() : m_capacity(S) {
		m_start = new T[S];
		m_size = 0;
	}

	~ByteArray() {
		delete[] m_start;
	}

	T& operator[](size_t index) {
#ifdef DEBUG
		return at(index);
#else
		return m_start[index];
#endif
	}

	constexpr T operator[](size_t index) const {
#ifdef DEBUG
		return at(index);
#else
		return m_start[index];
#endif
	}

	constexpr T at(size_t index) const {
		if (index >= size()) {
			pixi_error("Trying to access array of size {} with index of size {}\n", size(), index);
		}
		return m_start[index];
	}

	T& at(size_t index) {
		if (index >= size()) {
			pixi_error("Trying to access array of size {} with index of size {}\n", size(), index);
		}
		return m_start[index];
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

	void from_file(FILE* fp) {
		fseek(fp, 0, SEEK_END);
		auto fsize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		m_size = fsize / sizeof(T);
		fread(m_start, sizeof(T), m_size, fp);
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
	ByteArrayView(const ByteArray<T, S>& arr, size_t offset) : ptr(arr.ptr_at(offset)), size(arr.size()) {

	}

	constexpr T operator[](size_t index) const {
#ifdef DEBUG
		return at(index);
#else
		return ptr[index];
#endif
	}

	constexpr T at(size_t index) const {
		if (index >= size) {
			pixi_error("Trying to access array of size {} with index of size {}\n", size, index);
		}
		return ptr[index];
	}
};
