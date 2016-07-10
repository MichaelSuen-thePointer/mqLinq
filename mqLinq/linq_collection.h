#pragma once

#include <cassert>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <algorithm>

namespace pl
{
namespace linq
{
class collection_empty : public std::runtime_error
{
public:
    explicit collection_empty(const std::string& msg)
        : runtime_error(msg.c_str())
    {
    }

    explicit collection_empty(const char* msg)
        : runtime_error(msg)
    {
    }
};

class element_not_unique : public std::runtime_error
{
public:
    explicit element_not_unique(const std::string& _Message)
        : runtime_error(_Message.c_str())
    {
    }

    explicit element_not_unique(const char* _Message)
        : runtime_error(_Message)
    {
    }
};

template<class T>
struct lambda_tratis_impl;

template<class TRet, class TClass, class... TArgs>
struct lambda_tratis_impl<TRet(TClass::*)(TArgs...)>
{
    using return_type = TRet;
    using function_type = TRet(TArgs...);
};

template<class TRet, class TClass, class... TArgs>
struct lambda_tratis_impl<TRet(TClass::*)(TArgs...)const>
{
    using return_type = TRet;
    using function_type = TRet(TArgs...);
};

template<class TLambda>
struct function_traits : lambda_tratis_impl<decltype(&TLambda::operator())>
{
};

template <class TIter>
using deref_iter_t = std::decay_t<decltype(*std::declval<TIter>())>;

template <class... iters>
class iterator_common_impl;

template <class TIterator, class TValue>
class iterator_common_impl<TIterator, TValue>
{
protected:
    using self = iterator_common_impl<TIterator, TValue>;

    TIterator _iter;
public:
    using value_type = TValue;
    explicit iterator_common_impl(const TIterator& iter)
        : _iter(iter)
    {
    }

    virtual ~iterator_common_impl() = default;

    virtual self& operator++()
    {
        ++_iter;
        return *this;
    }

    virtual value_type operator*() const
    {
        return *_iter;
    }

    virtual bool operator==(const self& other) const
    {
        return _iter == other._iter;
    }

    virtual bool operator!=(const self& other) const
    {
        return _iter != other._iter;
    }
};

template<class TIterator>
class iterator_common_impl<TIterator> : public iterator_common_impl<TIterator, deref_iter_t<TIterator>>
{
public:
    explicit iterator_common_impl(const TIterator& iter)
        : iterator_common_impl<TIterator, deref_iter_t<TIterator>>(iter)
    {
    }
};

template <class TIterator, class TFunction>
class select_iterator : public iterator_common_impl<TIterator, typename function_traits<TFunction>::return_type>
{
private:
    using base = iterator_common_impl<TIterator, typename function_traits<TFunction>::return_type>;
    using self = select_iterator<TIterator, TFunction>;
public:
    using value_type = typename base::value_type;
    TFunction _func;
public:
    select_iterator(const TIterator& iter, const TFunction& pred)
        : base(iter)
        , _func(pred)
    {
    }

    auto operator*() const -> typename function_traits<TFunction>::return_type override
    {
        return _func(*base::_iter);
    }
};

template <class TIterator, class TFunction>
class where_iterator : public iterator_common_impl<TIterator>
{
private:
    using base = iterator_common_impl<TIterator>;
public:
    using self = where_iterator<TIterator, TFunction>;
    using value_type = typename base::value_type;
private:
    TIterator _end;
    TFunction _func;

    void check_move_iterator()
    {
        while (base::_iter != _end && !_func(*base::_iter))
        {
            ++base::_iter;
        }
    }

public:
    where_iterator(const TIterator& begin, const TIterator& end, const TFunction& func)
        : base(begin)
        , _end(end)
        , _func(func)
    {
        check_move_iterator();
    }


    self& operator++() override
    {
        ++base::_iter;
        check_move_iterator();

        return *this;
    }
};

template <class TIterator>
class skip_iterator : public iterator_common_impl<TIterator>
{
private:
    using base = iterator_common_impl<TIterator>;
    using self = skip_iterator<TIterator>;
public:
    using value_type = typename base::value_type;
private:
    TIterator _end;
public:
    skip_iterator(const TIterator& iter, const TIterator& end, size_t count)
        : base(iter)
        , _end(end)
    {
        assert(count >= 0);
        for (size_t i = 0; i != count && base::_iter != _end; i++, ++base::_iter)
        {
            //nothing
        }
    }
};

template <class TIterator, class TFunction>
class skip_while_iterator : public iterator_common_impl<TIterator>
{
private:
    using base = iterator_common_impl<TIterator>;
    using self = skip_while_iterator<TIterator, TFunction>;
    using value_type = typename base::value_type;

    TIterator _end;
    TFunction _func;
public:
    skip_while_iterator(const TIterator& iter, const TIterator& end, const TFunction& func)
        : base(iter)
        , _end(end)
        , _func(func)
    {
        while (base::_iter != end && func(*base::_iter))
        {
            ++base::_iter;
        }
    }
};

template <class TIterator>
class take_iterator : public iterator_common_impl<TIterator>
{
    using base = iterator_common_impl<TIterator>;
    using self = take_iterator<TIterator>;
public:
    using value_type = typename base::value_type;
private:
    TIterator _end;
    size_t _current;
    size_t _count;
public:
    take_iterator(const TIterator& iter, const TIterator& end, size_t count)
        : iterator_common_impl<TIterator>(iter)
        , _end(end)
        , _current(0)
        , _count(count)
    {
        assert(_count >= 0);
        if (_count == 0)
        {
            base::_iter = _end;
        }
    }

    self& operator++() override
    {
        if (++_current == _count)
        {
            base::_iter = _end;
        }
        else
        {
            ++base::_iter;
        }
        return *this;
    }
};

template <class TIterator, class TFunction>
class take_while_iterator : public iterator_common_impl<TIterator>
{
    using base = iterator_common_impl<TIterator>;
    using self = take_while_iterator<TIterator, TFunction>;
public:
    using value_type = typename base::value_type;
private:
    TIterator _end;
    TFunction _func;
public:
    take_while_iterator(const TIterator& begin, const TIterator& end, const TFunction& func)
        : iterator_common_impl<TIterator>(begin)
        , _end(end)
        , _func(func)
    {
        if (base::_iter != _end && !_func(*base::_iter))
        {
            base::_iter = end;
        }
    }

    self& operator++() override
    {
        if (!_func(*++base::_iter))
        {
            base::_iter = _end;
        }
        return *this;
    }
};

template <class TIterator1, class TIterator2, class TValue>
class iterator_common_impl<TIterator1, TIterator2, TValue>
{
private:
    using self = iterator_common_impl<TIterator1, TIterator2>;
public:
    using value_type = TValue;
    TIterator1 _iter1;
    TIterator2 _iter2;

    iterator_common_impl(TIterator1 iter1, TIterator2 iter2)
        : _iter1(iter1)
        , _iter2(iter2)
    {
    }

    virtual ~iterator_common_impl() = default;

    virtual self& operator++() = 0;

    virtual value_type operator*() const = 0;

    virtual bool operator==(const self& other) const
    {
        return _iter1 == other._iter1 && _iter2 == other._iter2;
    }

    virtual bool operator!=(const self& other) const
    {
        return !((*this) == other);
    }
};

template <class TIterator1, class TIterator2>
class concat_iterator : public iterator_common_impl<TIterator1, TIterator2, deref_iter_t<TIterator1>>
{
private:
    using self = concat_iterator<TIterator1, TIterator2>;
    using base = iterator_common_impl<TIterator1, TIterator2>;
    static_assert(std::is_same<deref_iter_t<TIterator1>,
                  deref_iter_t<TIterator2>>::value,
                  "iterators must point to the same type.");
public:
    using value_type = typename base::value_type;
private:
    TIterator1 _end1;
    TIterator2 _end2;
public:
    concat_iterator(const TIterator1& iter1, const TIterator1& end1, const TIterator2& iter2, const TIterator2& end2)
        : base(iter1, iter2)
        , _end1(end1)
        , _end2(end2)
    {
    }

    self& operator++() override
    {
        if (base::_iter1 != _end1)
        {
            ++base::_iter1;
        }
        else
        {
            ++base::_iter2;
        }
        return *this;
    }

    value_type operator*() const override
    {
        if (base::_iter1 != _end1)
        {
            return *base::_iter1;
        }
        return *base::_iter2;
    }
};

template <class TIterator1, class TIterator2>
class zip_iterator : public iterator_common_impl<TIterator1,
    TIterator2,
    std::pair<deref_iter_t<TIterator1>,
    deref_iter_t<TIterator2>>>
{
private:
    using self = zip_iterator<TIterator1, TIterator2>;
    using base = iterator_common_impl<TIterator1,
        TIterator2,
        std::pair<deref_iter_t<TIterator1>,
        deref_iter_t<TIterator2>>>;
    using value_type = typename base::value_type;

    TIterator1 _end1;
    TIterator2 _end2;
public:
    zip_iterator(const TIterator1& begin1, const TIterator1& end1, const TIterator2& begin2, const TIterator2& end2)
        : base(begin1, begin2)
        , _end1(end1)
        , _end2(end2)
    {
    }

    self& operator++() override
    {
        if (base::_iter1 != _end1 && base::_iter2 != _end2)
        {
            ++base::_iter1;
            ++base::_iter2;
        }
        return *this;
    }

    value_type operator*() const override
    {
        return value_type{*base::_iter, *base::_iter2};
    }

    bool operator==(const self& other) const
    {
        return base::_iter == other._iter && base::_iter2 == other._iter2;
    }

    bool operator!=(const self& other) const
    {
        return !((*this) == other);
    }
};

template <class TContainer>
class boxed_container_iterator : public iterator_common_impl<decltype(std::begin(std::declval<TContainer>()))>
{
private:
    using self = boxed_container_iterator<TContainer>;
    using base = iterator_common_impl<decltype(std::begin(std::declval<TContainer>()))>;
public:
    using iterator_type = decltype(std::begin(std::declval<TContainer>()));
    using value_type = deref_iter_t<iterator_type>;
    std::shared_ptr<TContainer> _container;

    boxed_container_iterator(const std::shared_ptr<TContainer>& container, const iterator_type& iterator)
        : base(iterator)
        , _container(container)
    {
    }
};

template <class T>
class empty_iterator
{
private:
    using self = empty_iterator<T>;
    using value_type = T;
public:
    self& operator++()
    {
        return *this;
    }

    [[noreturn]]

    value_type operator*() const
    {
        throw collection_empty("collection is empty");
    }

    bool operator==(const self&) const
    {
        return true;
    }

    bool operator!=(const self&) const
    {
        return false;
    }
};


template <class TValue>
class any_iterator
{
private:
    class any_base
    {
        using self = any_base;
    public:
        using value_type = TValue;
        virtual ~any_base() = default;
        virtual self* operator++() = 0;
        virtual value_type operator*() const = 0;
        virtual bool operator==(const self& other) const = 0;
        virtual bool operator!=(const self& other) const = 0;
    };

    template <class TIterator>
    class any_content : public any_base
    {
        using self = any_content<TIterator>;
        using base = any_base;
        TIterator _iter;
    public:
        using value_type = typename base::value_type;

        explicit any_content(const TIterator& iter)
            : _iter(iter)
        {
        }

        self* operator++() override
        {
            ++_iter;
            return this;
        }

        value_type operator*() const override
        {
            return *_iter;
        }

        bool operator==(const base& other) const override
        {
            auto real = dynamic_cast<const self*>(&other);
            return real && _iter == real->_iter;
        }

        bool operator!=(const base& other) const override
        {
            return !((*this) == other);
        }
    };

    using self = any_iterator;

    std::shared_ptr<any_base> _iter;
public:
    using value_type = TValue;

    template <class TIterator, std::enable_if_t<std::is_same<deref_iter_t<TIterator>, TValue>::value>* = nullptr>
    explicit any_iterator(const TIterator& iter)
        : _iter(std::make_shared<any_content<TIterator>>(iter))
    {
    }

    self& operator++()
    {
        ++(*_iter);
        return *this;
    }

    value_type operator*() const
    {
        return **_iter;
    }

    bool operator==(const self& other)
    {
        return (*_iter) == (*other._iter);
    }

    bool operator!=(const self& other)
    {
        return !((*this) == other);
    }
};

template <class TIterator>
class linq_collection;

template <class T>
class linq;

template <class TContainer>
auto from(const TContainer& cont)->linq_collection<decltype(std::cbegin(cont))>;

template <class TIterator>
auto from(const TIterator& a, const TIterator& b)->linq_collection<TIterator>;

template <class T>
linq<T> from_values(const std::initializer_list<T>& cont)
{
    using cont_type = std::vector<T>;
    auto xs = std::make_shared<cont_type>(cont);
    return linq_collection<boxed_container_iterator<cont_type>>(
        boxed_container_iterator<cont_type>(xs, xs->begin()),
        boxed_container_iterator<cont_type>(xs, xs->end())
        );
}

template <class TContainer, std::enable_if_t<!std::is_same<
    std::decay_t<TContainer>,
    std::initializer_list<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>>::value>* = nullptr>
    auto from_values(const TContainer& cont) -> linq<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>
{
    auto xs = std::make_shared<TContainer>(cont);
    using iter_type = boxed_container_iterator<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>;
    return linq_collection<iter_type>(
        iter_type(xs, xs->begin()),
        iter_type(xs, xs->end())
        );
}

template <class TContainer, std::enable_if_t<!std::is_same<
    std::decay_t<TContainer>,
    std::initializer_list<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>>::value>* = nullptr>
    auto from_values(TContainer&& cont) -> linq<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>
{
    auto xs = std::make_shared<TContainer>(std::move(cont));
    using iter_type = boxed_container_iterator<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>;
    return linq_collection<iter_type>(
        iter_type(xs, xs->begin()),
        iter_type(xs, xs->end())
        );
}

template <class T>
linq<T> from_value(const T& value)
{
    return from_values({value});
}

template <class T>
linq<T> from_empty()
{
    using iter_type = empty_iterator<T>;
    return linq_collection<iter_type>{
        iter_type{},
            iter_type{}
    };
}

template <class TIterator>
class linq_collection
{
public:
    using value_type = deref_iter_t<TIterator>;
private:
    using self = linq_collection<TIterator>;

    TIterator _begin;
    TIterator _end;

public:
    linq_collection(const TIterator& begin, const TIterator& end)
        : _begin(begin)
        , _end(end)
    {
    }

    TIterator begin() const
    {
        return _begin;
    }

    TIterator end() const
    {
        return _end;
    }

    template <class TFunction>
    linq_collection<select_iterator<TIterator, TFunction>> select(const TFunction& func) const
    {
        return{
            select_iterator<TIterator, TFunction>{_begin, func},
            select_iterator<TIterator, TFunction>{_end, func}
        };
    }

    template <class TFunction>
    linq_collection<where_iterator<TIterator, TFunction>> where(const TFunction& func) const
    {
        return{
            where_iterator<TIterator, TFunction>{_begin, _end, func},
            where_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    linq_collection<skip_iterator<TIterator>> skip(int count) const
    {
        return{
            skip_iterator<TIterator>{_begin, _end, count},
            skip_iterator<TIterator>{_end, _end, count},
        };
    }

    template <class TFunction>
    linq_collection<skip_while_iterator<TIterator, TFunction>> skip_while(const TFunction& func) const
    {
        return{
            skip_while_iterator<TIterator, TFunction>{_begin, _end, func},
            skip_while_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    linq_collection<take_iterator<TIterator>> take(size_t count) const
    {
        return{
            take_iterator<TIterator>{_begin, _end, count},
            take_iterator<TIterator>{_end, _end, count}
        };
    }

    template <typename TFunction>
    linq_collection<take_while_iterator<TIterator, TFunction>> take_while(const TFunction& func) const
    {
        return{
            take_while_iterator<TIterator, TFunction>{_begin, _end, func},
            take_while_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    template <class TIterator2>
    linq_collection<concat_iterator<TIterator, TIterator2>> concat(const linq_collection<TIterator2>& other) const
    {
        return{
            concat_iterator<TIterator, TIterator2>{_begin, _end, other.begin(), other.end()},
            concat_iterator<TIterator, TIterator2>{_end, _end, other.end(), other.end()}
        };
    }

    template <class TContainer>
    auto concat(const TContainer& other) const -> decltype(concat(from(other)))
    {
        return concat(from(other));
    }

    template <class T>
    auto concat(const std::initializer_list<T>& ilist) const -> decltype(concat(from(ilist)))
    {
        return concat(from(ilist));
    }

    template <class T>
    auto contains(const T& t) const -> decltype(std::declval<value_type>() == std::declval<T>(), bool())
    {
        for (const auto& x : *this)
        {
            if (x == t)
            {
                return true;
            }
        }
        return false;
    }

    size_t count() const
    {
        int c = 0;
        for (auto i = _begin; i != _end; ++i)
        {
            c++;
        }
        return c;
    }

    value_type element_at(size_t i) const
    {
        auto iter = _begin;
        for (; iter != _end && i != 0; ++iter, i--)
        {
            //nothing
        }
        if (iter != _end)
        {
            return *iter;
        }
        throw std::out_of_range("index out of range: " + std::to_string(i));
    }

    bool empty() const
    {
        return _begin == _end;
    }

    value_type first() const
    {
        if (empty())
        {
            empty_err();
        }
        return *_begin;
    }

    value_type first_or_default(const value_type& v) const
    {
        return empty() ? v : *_begin;
    }

    value_type last() const
    {
        if (empty())
        {
            empty_err();
        }
        auto resit = _begin;
        auto it = _begin;
        while (it != _end)
        {
            resit = it;
            ++it;
        }
        return *resit;
    }

    value_type last_or_default(const value_type& v) const
    {
        auto resit = _begin;
        auto it = _begin;
        while (it != _end)
        {
            resit = it;
            ++it;
        }
        if (resit == _end)
        {
            return v;
        }
        return *resit;
    }

    linq_collection<TIterator> single() const
    {
        auto it = _begin;
        if (it == _end)
        {
            empty_err();
        }
        ++it;
        if (it != _end)
        {
            throw element_not_unique("more than one element in collection");
        }
        return *this;
    }

    linq<value_type> single_or_default(const value_type& v) const
    {
        auto it = _begin;
        if (it == _end)
        {
            return from_value(v);
        }
        ++it;
        if (it != _end)
        {
            throw element_not_unique("more than one element in collection");
        }
        return *this;
    }

    template <class TIterator2>
    auto sequence_equal(const linq_collection<TIterator2>& e) const -> decltype(std::declval<value_type>() ==
                                                                                std::declval<typename decltype(e)::value_type>(), bool())
    {
        auto it1 = _begin;
        auto it1e = _end;
        auto it2 = e.begin();
        auto it2e = e.end();

        while (it1 != it1e && it2 != it2e)
        {
            if (*it1 != *it2)
            {
                return false;
            }
            ++it1;
            ++it2;
        }
        return it1 == it1e && it2 == it2e;
    }

    template <class TContainer>
    auto sequence_equal(const TContainer& other) const -> decltype(sequence_equal(from(other)))
    {
        return sequence_equal(from(other));
    }

    template <class T>
    auto sequence_equal(const std::initializer_list<T>& ilist) const -> decltype(sequence_equal(from(ilist)))
    {
        return sequence_equal(from(ilist));
    }

    linq<value_type> distinct() const
    {
        std::set<value_type> s;
        std::vector<value_type> v;

        std::for_each(begin(), end(), [&s, &v](const auto& x) {
            if (s.insert(x).second)
            {
                v.push_back(x);
            }
        });

        return from_values(std::move(v));
    }

    template <class TIterator2, std::enable_if_t<std::is_same<value_type, deref_iter_t<TIterator2>>::value>* = nullptr>
    linq<value_type> intersect_with(const linq_collection<TIterator2>& e) const
    {
        std::set<value_type> s1, s2(e.begin(), e.end());
        std::vector<value_type> res;
        std::for_each(begin(), end(), [&s1, &s2, &res](const auto& x) {
            if (s1.insert(x).second && !s2.insert(x).second)
            {
                res.push_back(x);
            }
        });
        return from_values(std::move(res));
    }

    template <class TContainer>
    auto intersect_with(const TContainer& other) const -> decltype(intersect_with(from(other)))
    {
        return intersect_with(from(other));
    }

    template <class T>
    auto intersect_with(const std::initializer_list<T>& ilist) const -> decltype(intersect_with(from(ilist)))
    {
        return intersect_with(from(ilist));
    }

    template <class TIterator2, std::enable_if_t<std::is_same<value_type, deref_iter_t<TIterator2>>::value>* = nullptr>
    linq<value_type> union_with(const linq_collection<TIterator2>& e) const
    {
        return concat(e).distinct();
    }

    template <class TContainer>
    auto union_with(const TContainer& other) const -> decltype(union_with(from(other)))
    {
        return union_with(from(other));
    }

    template <class T>
    auto union_with(const std::initializer_list<T>& ilist) const -> decltype(union_with(from(ilist)))
    {
        return union_with(from(ilist));
    }

    template <class TFunction>
    value_type aggregate(const TFunction& f) const
    {
        if (empty())
        {
            empty_err();
        }
        auto it = _begin;
        value_type res = *it;
        ++it;
        std::for_each(it, _end, [&res, &f](const auto& x) {
            res = f(res, x);
        });
        return res;
    }

    template <class TResult, class TFunction>
    TResult aggregate(const TResult& init, const TFunction& f) const
    {
        TResult res = init;
        std::for_each(_begin, _end, [&res, &f](const auto& x) {
            res = f(res, x);
        });
        return res;
    }

    template <class TFunction>
    bool all(const TFunction& f) const
    {
        return select(f).aggregate(true, [](bool a, bool b) {
            return a && b;
        });
    }

    template <class TFunction>
    bool any(const TFunction& f) const
    {
        return !where(f).empty();
    }

    template <class TResult>
    TResult average() const
    {
        if (empty())
        {
            empty_err();
        }
        TResult sum{};
        int counter = 0;
        std::for_each(_begin, _end, [&sum, &counter](const auto& x) {
            sum += x;
            counter++;
        });
        return sum / counter;
    }

    value_type max() const
    {
        return aggregate([](const auto& x, const auto& y) {
            return x > y ? x : y;
        });
    }

    value_type min() const
    {
        return aggregate([](const auto& x, const auto& y) {
            return x < y ? x : y;
        });
    }

    value_type sum() const
    {
        return aggregate([](const auto& x, const auto& y) {
            return x + y;
        });
    }

    value_type product() const
    {
        return aggregate([](const auto& x, const auto& y) {
            return x + y;
        });
    }

private:
    [[noreturn]]

    static void empty_err()
    {
        throw collection_empty("collection empty");
    }
};

template <typename T>
class linq : public linq_collection<any_iterator<T>>
{
    using base = linq_collection<any_iterator<T>>;
public:
    linq()
        : linq_collection<any_iterator<T>>()
    {
    }

    template <typename TIterator>
    linq(const linq_collection<TIterator>& e)
        : base(
            any_iterator<T>(e.begin()),
            any_iterator<T>(e.end())
        )
    {
    }
};

template <class TContainer>
auto from(const TContainer& cont) -> linq_collection<decltype(std::cbegin(cont))>
{
    return{std::cbegin(cont), std::cend(cont)};
}

template <class TIterator>
auto from(const TIterator& a, const TIterator& b) -> linq_collection<TIterator>
{
    return{a, b};
}
}
}
