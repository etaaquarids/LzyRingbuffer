// LzyArray.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once
#include <atomic>
#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
#include <cassert>

namespace Lzy::Ringbuffer {
	
	// if Array push much faster than pop, this will not work
	template<typename T>
	struct Array {
	private:
		std::atomic<uint64_t> in_index{ 0 };
		std::atomic<uint64_t> out_index{ 0 };
		std::vector<T> buffer;
	public:
		// better to be multiple ex of 2;
		Array(size_t size) : buffer(size) {
		}
		Array(const Array& another) : buffer(another.buffer), in_index(another.in_index.load()), out_index(another.out_index.load()) {
		}
		Array(Array&& another) : buffer(std::move(another.buffer)), in_index(another.in_index.load()), out_index(another.out_index.load()) {
		}
		bool isEmpty() const noexcept {
			return in_index == out_index;
		}
		uint64_t unused_size() {
			return buffer.size() - (in_index - out_index);
		}
		uint64_t size() {
			return in_index - out_index;
		}

		bool push(std::ranges::range auto&& array_ref) {
			size_t booking_size = array_ref.size();
			uint64_t index = in_index;
			if (unused_size() < booking_size) {
				return false;
			};
			in_index = (index + booking_size) % buffer.size();

			auto left_size = (std::min<size_t>)(booking_size, buffer.size() - index);

			std::move(array_ref.begin(), array_ref.begin() + left_size, buffer.begin() + index);
			std::move(array_ref.begin() + left_size, array_ref.end(), buffer.begin());

			return true;
		}

		bool push(std::same_as<T> auto&&... array_ref) {
			size_t booking_size = sizeof...(array_ref);
			uint64_t index = in_index;
			if (unused_size() < booking_size) {
				return false;
			};
			in_index = (index + booking_size) % buffer.size();

			auto left_size = (std::min<size_t>)(booking_size, buffer.size() - index);

			(..., (*(buffer.begin() + (index++) % buffer.size()) = std::move(array_ref)));

			return true;
		}


		// must check if empty before work
		T& pop() {
			assert(!isEmpty() && "Must check is empty before pop");
			uint64_t index = out_index;
			out_index = (index + 1) % buffer.size();
			return buffer[index];
		}

		std::vector<T> pop_n(uint64_t len) {
			uint64_t index = out_index;
			len = (std::min<size_t>)(len, size());
			out_index = (index + len) % buffer.size();

			std::vector<T> results(len);

			auto left_size = (std::min<size_t>)(len, buffer.size() - index);

			std::move(buffer.begin() + index, buffer.begin() + index + left_size, results.begin());
			std::move(buffer.begin(), buffer.begin() + len - left_size, results.begin() + left_size);

			return results;
		}
	};

}