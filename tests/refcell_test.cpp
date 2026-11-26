#include "refcell.hpp"

#include <cassert>
#include <vector>

template <typename Error, typename Function>
void expect_error(Function function) {
    bool caught = false;
    try { function(); } catch (const Error&) { caught = true; }
    assert(caught);
}

struct CopyOnly {
    int number;
    explicit CopyOnly(int number) : number(number) {}
    CopyOnly(const CopyOnly&) = default;
    CopyOnly(CopyOnly&&) = delete;
};

int main() {
    static_assert(!std::is_copy_constructible_v<RefCell<int>>);
    static_assert(!std::is_copy_constructible_v<RefCell<int>::RefMut>);
    static_assert(std::is_nothrow_move_constructible_v<RefCell<int>::Ref>);
    RefCell<int> a(1), b(2);
    {
        auto r = a.borrow();
        auto copy = r;
        auto moved = std::move(copy);
        auto other = b.borrow();
        other = moved;
        assert(b.try_borrow_mut());
        r = r;
        other = std::move(moved);
        assert(*r == 1 && *other == 1);
        assert(!a.try_borrow_mut());
        expect_error<BorrowMutError>([&] { a.borrow_mut(); });
    }
    {
        auto x = a.borrow_mut();
        *x = 5;
        assert(!a.try_borrow() && !a.try_borrow_mut());
        expect_error<BorrowError>([&] { a.borrow(); });
        expect_error<BorrowMutError>([&] { a.borrow_mut(); });
        auto y = b.borrow_mut();
        y = std::move(x);
        assert(b.try_borrow_mut());
        auto z = std::move(y);
        assert(*z == 5 && !a.try_borrow());
    }
    assert(*a.borrow() == 5);
    {
        std::vector<RefCell<int>::Ref> refs;
        for (int i = 0; i < 10000; ++i) refs.push_back(a.borrow());
        refs.erase(refs.begin(), refs.begin() + 5000);
        assert(!a.try_borrow_mut());
    }
    assert(a.try_borrow_mut());
    const RefCell<int> constant(9);
    assert(*constant.borrow() == 9);
    RefCell<std::unique_ptr<int>> move_only(std::make_unique<int>(7));
    assert(**move_only.borrow() == 7);
    RefCell<CopyOnly> copy_only(CopyOnly(4));
    assert(copy_only.borrow()->number == 4);
    RefCell<std::string> text(std::string("hello"));
    text.borrow_mut()->append(" world");
    assert(text.borrow()->size() == 11);
    {
        auto* cell = new RefCell<int>(3);
        auto ref = cell->borrow();
        expect_error<DestructionError>([&] { delete cell; });
    }
    {
        auto* cell = new RefCell<int>(3);
        auto ref = cell->borrow_mut();
        expect_error<DestructionError>([&] { delete cell; });
    }
}
