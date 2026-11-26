#ifndef REFCELL_HPP
#define REFCELL_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    ~RefCellError() override = default;
};

class BorrowError : public RefCellError {
public:
    using RefCellError::RefCellError;
};

class BorrowMutError : public RefCellError {
public:
    using RefCellError::RefCellError;
};

class DestructionError : public RefCellError {
public:
    using RefCellError::RefCellError;
};

template <typename T>
class RefCell {
    struct BorrowState {
        std::size_t readers = 0;
        bool writer = false;
    };

    T value;
    std::shared_ptr<BorrowState> state = std::make_shared<BorrowState>();

public:
    class Ref {
        friend class RefCell;
        const T* value = nullptr;
        std::shared_ptr<BorrowState> state;

        Ref(const T* value, const std::shared_ptr<BorrowState>& state)
            : value(value), state(state) {
            ++state->readers;
        }

        void release() noexcept {
            if (state) --state->readers;
        }

    public:
        Ref() = default;
        ~Ref() { release(); }
        const T& operator*() const { return *value; }
        const T* operator->() const { return value; }

        Ref(const Ref& other) : value(other.value), state(other.state) {
            if (state) ++state->readers;
        }

        Ref& operator=(const Ref& other) {
            if (this != &other) {
                release();
                value = other.value;
                state = other.state;
                if (state) ++state->readers;
            }
            return *this;
        }

        Ref(Ref&& other) noexcept
            : value(std::exchange(other.value, nullptr)), state(std::move(other.state)) {}

        Ref& operator=(Ref&& other) noexcept {
            if (this != &other) {
                release();
                value = std::exchange(other.value, nullptr);
                state = std::move(other.state);
            }
            return *this;
        }
    };

    class RefMut {
        friend class RefCell;
        T* value = nullptr;
        std::shared_ptr<BorrowState> state;

        RefMut(T* value, const std::shared_ptr<BorrowState>& state)
            : value(value), state(state) {
            state->writer = true;
        }

        void release() noexcept {
            if (state) state->writer = false;
        }

    public:
        RefMut() = default;
        ~RefMut() { release(); }
        T& operator*() { return *value; }
        T* operator->() { return value; }
        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        RefMut(RefMut&& other) noexcept
            : value(std::exchange(other.value, nullptr)), state(std::move(other.state)) {}

        RefMut& operator=(RefMut&& other) noexcept {
            if (this != &other) {
                release();
                value = std::exchange(other.value, nullptr);
                state = std::move(other.state);
            }
            return *this;
        }
    };

    explicit RefCell(const T& initial_value) : value(initial_value) {}

    // Also support copy-only types with an explicitly deleted move constructor.
    explicit RefCell(T&& initial_value)
        : value(static_cast<std::conditional_t<std::is_move_constructible_v<T>,
                                              T&&, const T&>>(initial_value)) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    Ref borrow() const {
        if (state->writer) throw BorrowError("Mutable borrow is active");
        return Ref(&value, state);
    }

    std::optional<Ref> try_borrow() const {
        if (state->writer) return std::nullopt;
        return Ref(&value, state);
    }

    RefMut borrow_mut() {
        if (state->writer || state->readers) throw BorrowMutError("Borrow is active");
        return RefMut(&value, state);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (state->writer || state->readers) return std::nullopt;
        return RefMut(&value, state);
    }

    // noexcept(false) lets callers catch the required destruction error.
    ~RefCell() noexcept(false) {
        if (state->writer || state->readers)
            throw DestructionError("Cell destroyed with active borrows");
    }
};

#endif
