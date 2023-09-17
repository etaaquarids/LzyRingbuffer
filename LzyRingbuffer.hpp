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
		bool empty() const noexcept {
			return in_index == out_index;
		}
		uint64_t unused_size() const noexcept {
			return buffer.size() - size();
		}
		uint64_t size() const noexcept {
			return in_index - out_index;
		}

	private:
		uint64_t book_space(uint64_t booking_size) {
			assert(booking_size <= unused_size() && "Must check unused_size before push");
			uint64_t index = in_index;
			in_index = (index + booking_size) % buffer.size();
			return index;
		}

	public:
		void push(std::ranges::range auto&& array_ref) {
			auto index = book_space(array_ref.size());
			auto left_size = (std::min<size_t>)(array_ref.size(), buffer.size() - index);

			std::move(array_ref.begin(), array_ref.begin() + left_size, buffer.begin() + index);
			std::move(array_ref.begin() + left_size, array_ref.end(), buffer.begin());
		}

		void push(std::same_as<T> auto&&... array_ref) {
			auto index = book_space(sizeof...(array_ref));
			auto left_size = (std::min<size_t>)(sizeof...(array_ref), buffer.size() - index);

			(..., (*(buffer.begin() + (index++) % buffer.size()) = std::move(array_ref)));
		}


		// must check if empty before work
		T& pop() {
			assert(!empty() && "Must check if empty before pop");
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
		void wait() const {
			uint64_t index = in_index;
			in_index.wait(index);
		}
		void notify_one() noexcept {
			in_index.notify_one();
		}
		void notify_all() noexcept {
			in_index.notify_all();
		}


	};

}