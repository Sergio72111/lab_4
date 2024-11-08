#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <cassert>
#include <limits>

template <typename T, size_t BlockSize = 10>
class MyAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = MyAllocator<U, BlockSize>;
    };

    MyAllocator() noexcept = default;
    template <typename U>
    MyAllocator(const MyAllocator<U, BlockSize>&) noexcept {}

    pointer allocate(size_type n) {
        if (n > BlockSize) {
            return static_cast<pointer>(::operator new(n * sizeof(T)));
        }
        if (free_blocks.empty()) {
            expand();
        }
        pointer result = free_blocks.back();
        free_blocks.pop_back();
        return result;
    }

    void deallocate(pointer p, size_type n) noexcept {
        if (n > BlockSize) {
            ::operator delete(p);
        } else {
            free_blocks.push_back(p);
        }
    }

    size_type max_size() const noexcept {
        return std::min<size_type>(
            std::numeric_limits<size_type>::max(),
            std::numeric_limits<size_type>::max() / sizeof(T)
        );
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

private:
    void expand() {
        for (size_t i = 0; i < BlockSize; ++i) {
            free_blocks.push_back(static_cast<pointer>(::operator new(sizeof(T))));
        }
    }

    std::vector<pointer> free_blocks;
};

template <typename T, typename U, size_t BlockSize>
bool operator==(const MyAllocator<T, BlockSize>&, const MyAllocator<U, BlockSize>&) noexcept {
    return true;
}

template <typename T, typename U, size_t BlockSize>
bool operator!=(const MyAllocator<T, BlockSize>&, const MyAllocator<U, BlockSize>&) noexcept {
    return false;
}

template <typename T, typename Allocator = std::allocator<T>>
class MyContainer {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using pointer = typename std::allocator_traits<Allocator>::pointer;

    MyContainer(const allocator_type& alloc = allocator_type()) : alloc_(alloc), size_(0) {}

    void push_back(const T& value) {
        pointer ptr = std::allocator_traits<Allocator>::allocate(alloc_, 1);
        std::allocator_traits<Allocator>::construct(alloc_, ptr, value);
        elements_.push_back(ptr);
        ++size_;
    }

    void print() const {
        for (const auto& ptr : elements_) {
            std::cout << *ptr << " ";
        }
        std::cout << std::endl;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    ~MyContainer() {
        for (auto ptr : elements_) {
            std::allocator_traits<Allocator>::destroy(alloc_, ptr);
            std::allocator_traits<Allocator>::deallocate(alloc_, ptr, 1);
        }
    }

private:
    allocator_type alloc_;
    std::vector<pointer> elements_;
    size_t size_;
};

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    // 1. Создание экземпляра std::map<int, int>;
    std::map<int, int> map1;
    
    // 2. Заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа;
    for (int i = 0; i < 10; ++i) {
        map1[i] = factorial(i);
    }

    // 3. Создание экземпляра std::map<int, int> с новым аллокатором, ограниченным 10 элементами;
    std::map<int, int, std::less<int>, MyAllocator<std::pair<const int, int>, 10>> map2;

    // 4. Заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа;
    for (int i = 0; i < 10; ++i) {
        map2[i] = factorial(i);
    }

    // 5. Вывод на экран всех значений (ключ и значение разделены пробелом), хранящихся в контейнере;
    std::cout << "map1:" << std::endl;
    for (const auto& [key, value] : map1) {
        std::cout << key << " " << value << std::endl;
    }

    std::cout << "map2:" << std::endl;
    for (const auto& [key, value] : map2) {
        std::cout << key << " " << value << std::endl;
    }

    // 6. Создание экземпляра своего контейнера для хранения значений типа int;
    MyContainer<int> container1;

    // 7. Заполнение 10 элементами от 0 до 9;
    for (int i = 0; i < 10; ++i) {
        container1.push_back(i);
    }

    // 8. Создание экземпляра своего контейнера для хранения значений типа int с новым аллокатором, ограниченным 10 элементами;
    MyContainer<int, MyAllocator<int, 10>> container2;

    // 9. Заполнение 10 элементами от 0 до 9;
    for (int i = 0; i < 10; ++i) {
        container2.push_back(i);
    }

    // 10. Вывод на экран всех значений, хранящихся в контейнере;
    std::cout << "container1: ";
    container1.print();

    std::cout << "container2: ";
    container2.print();

    return 0;
}