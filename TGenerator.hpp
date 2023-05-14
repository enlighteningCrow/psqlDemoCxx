#include <coroutine>
#include <ranges>

template <class T> class TGenerator {
public:
  struct promise_type {
    T val;
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    std::suspend_always yield_value(T value) {
      val = std::move(value);
      return {};
    }
    auto get_return_object() {
      return TGenerator(
          std::coroutine_handle<promise_type>::from_promise(*this));
    }
    void unhandled_exception() {}
  };

private:
  std::coroutine_handle<promise_type> handle_;

public:
  TGenerator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  ~TGenerator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  class iterator {
    std::coroutine_handle<promise_type> handle_;

  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef iterator self_type;
    typedef T value_type;

    iterator() = default;

    iterator(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

    iterator &operator++() {
      handle_.resume();
      if (handle_.done()) {
        handle_ = nullptr;
      }
      return *this;
    }

    bool operator==(const iterator &other) const {
      return handle_ == other.handle_;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

    const T &operator*() const { return handle_.promise().val; }
  };

  iterator begin() {
    if (handle_) {
      handle_.resume();
    }
    return iterator(handle_);
  }

  iterator end() { return iterator(); }
};

template <class T>
  requires std::integral<T>
TGenerator<T> range(T start, T end, T step) {
  for (T i{start}; step > 0 ? i < end : i > end; i += step) {
    co_yield i;
  }
}

template <class T>
  requires std::integral<T>
TGenerator<T> range(T start, T end) {
  for (T i{start}; i < end; ++i) {
    co_yield i;
  }
}

template <class T>
  requires std::integral<T>
TGenerator<T> range(T end) {
  for (T i{0}; i < end; ++i) {
    co_yield i;
  }
}

using Generator = TGenerator<std::string>;
