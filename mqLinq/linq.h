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
#include <map>
#include <deque>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace pl
{
namespace linq
{
class linq_exception : public std::logic_error
{
public:
    explicit linq_exception(const std::string& _Message)
        : logic_error(_Message.c_str())
    {
    }

    explicit linq_exception(const char* _Message)
        : logic_error(_Message)
    {
    }
};

class collection_empty : public linq_exception
{
public:
    explicit collection_empty(const std::string& msg)
        : linq_exception(msg.c_str())
    {
    }

    explicit collection_empty(const char* msg)
        : linq_exception(msg)
    {
    }
};

class element_not_unique : public linq_exception
{
public:
    explicit element_not_unique(const std::string& _Message)
        : linq_exception(_Message.c_str())
    {
    }

    explicit element_not_unique(const char* _Message)
        : linq_exception(_Message)
    {
    }
};

template <class T>
struct memfunc_traits;

template <class TRet, class TClass, class... TArgs>
struct memfunc_traits<TRet(TClass::*)(TArgs ...)>
{
    using return_type = TRet;
    using param_type = std::tuple<TArgs...>;
    using is_const_this = std::false_type;
};

template <class TRet, class TClass, class... TArgs>
struct memfunc_traits<TRet(TClass::*)(TArgs ...) const>
{
    using return_type = TRet;
    using param_type = std::tuple<TArgs...>;
    using is_const_this = std::false_type;
};

template <class TFunction>
struct callable_traits;

template <class TFunction>
struct callable_traits : memfunc_traits<decltype(&TFunction::operator())>
{
};

template <class TRet, class... TArgs>
struct callable_traits<TRet(*)(TArgs ...)>
{
    using return_type = TRet;
    using function_type = TRet(TArgs ...);
    using param_type = std::tuple<TArgs...>;
};

template <class TRet, class... TArgs>
struct callable_traits<TRet(TArgs ...)>
{
    using return_type = TRet;
    using function_type = TRet(TArgs ...);
    using param_type = std::tuple<TArgs...>;
};

template <class TRet, class TClass, class... TArgs>
struct callable_traits<TRet(TClass::*)(TArgs ...)> : memfunc_traits<TRet(TClass::*)(TArgs ...)>
{
};

template <class TRet, class TClass, class... TArgs>
struct callable_traits<TRet(TClass::*)(TArgs ...) const> : memfunc_traits<TRet(TClass::*)(TArgs ...) const>
{
};

template <typename TIterator>
using deref_iter_t = std::decay_t<decltype(*(std::declval<TIterator>()))>;

template <class TValue>
class linq_iterator_traits
{
public:
    using value_type = TValue;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

template <class... iters>
class iterator_common_impl;

template <class TIterator, class TValue>
class iterator_common_impl<TIterator, TValue> : public linq_iterator_traits<TValue>
{
protected:
    using self = iterator_common_impl<TIterator, TValue>;

    TIterator _iter;

    explicit iterator_common_impl(const TIterator& iter)
        : _iter(iter)
    {
    }
public:
    self& operator++()
    {
        ++_iter;
        return *this;
    }

    value_type operator*() const
    {
        return *_iter;
    }

    bool operator==(const self& other) const
    {
        return _iter == other._iter;
    }

    bool operator!=(const self& other) const
    {
        return _iter != other._iter;
    }
};

template <class TIterator>
class iterator_common_impl<TIterator> : public iterator_common_impl<TIterator, deref_iter_t<TIterator>>
{
protected:
    explicit iterator_common_impl(const TIterator& iter)
        : iterator_common_impl<TIterator, deref_iter_t<TIterator>>(iter)
    {
    }
};

template <class TIterator, class TFunction>
class select_iterator : public iterator_common_impl<TIterator, decltype(std::declval<TFunction>()(std::declval<deref_iter_t<TIterator>>()))>
{
private:
    using base = iterator_common_impl<TIterator, decltype(std::declval<TFunction>()(std::declval<deref_iter_t<TIterator>>()))>;
    using self = select_iterator<TIterator, TFunction>;
    using return_type = decltype(std::declval<TFunction>()(std::declval<deref_iter_t<TIterator>>()));
public:
    using value_type = typename base::value_type;
    TFunction _func;
public:
    select_iterator(const TIterator& iter, const TFunction& pred)
        : base(iter)
        , _func(pred)
    {
    }

    auto operator*() const -> return_type
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


    self& operator++()
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
    skip_iterator(const TIterator& iter, const TIterator& end, std::size_t count)
        : base(iter)
        , _end(end)
    {
        assert(count >= 0);
        for (std::size_t i = 0; i != count && base::_iter != _end; i++ , ++base::_iter)
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
    std::size_t _current;
    std::size_t _count;
public:
    take_iterator(const TIterator& iter, const TIterator& end, std::size_t count)
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

    self& operator++()
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

    self& operator++()
    {
        if (!_func(*++base::_iter))
        {
            base::_iter = _end;
        }
        return *this;
    }
};

template <class TIterator1, class TIterator2, class TValue>
class iterator_common_impl<TIterator1, TIterator2, TValue> : public linq_iterator_traits<TValue>
{
private:
    using self = iterator_common_impl<TIterator1, TIterator2, TValue>;

protected:
    TIterator1 _iter1;
    TIterator2 _iter2;

    iterator_common_impl(TIterator1 iter1, TIterator2 iter2)
        : _iter1(iter1)
        , _iter2(iter2)
    {
    }

public:
    self& operator++()
    {
        return *this;
    }

    value_type operator*()
    {
        return value_type{};
    }

    bool operator==(const self& other) const
    {
        return _iter1 == other._iter1 && _iter2 == other._iter2;
    }

    bool operator!=(const self& other) const
    {
        return !((*this) == other);
    }
};

template <class TIterator1, class TIterator2>
class concat_iterator : public iterator_common_impl<TIterator1, TIterator2, deref_iter_t<TIterator1>>
{
private:
    using self = concat_iterator<TIterator1, TIterator2>;
    using base = iterator_common_impl<TIterator1, TIterator2, deref_iter_t<TIterator1>>;
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

    self& operator++()
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

    value_type operator*() const
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

    TIterator1 _end1;
    TIterator2 _end2;
public:
    using value_type = typename base::value_type;

    zip_iterator(const TIterator1& begin1, const TIterator1& end1, const TIterator2& begin2, const TIterator2& end2)
        : base(begin1, begin2)
        , _end1(end1)
        , _end2(end2)
    {
    }

    self& operator++()
    {
        if (base::_iter1 != _end1 && base::_iter2 != _end2)
        {
            ++base::_iter1;
            ++base::_iter2;
        }
        return *this;
    }

    value_type operator*() const
    {
        return value_type{*base::_iter1, *base::_iter2};
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
class empty_iterator : public linq_iterator_traits<T>
{
private:
    using self = empty_iterator<T>;
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
class any_iterator : public linq_iterator_traits<TValue>
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

    using self = any_iterator<TValue>;

    std::shared_ptr<any_base> _iter;
public:

    template <class TIterator>
    any_iterator(const TIterator& iter)
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

    bool operator==(const self& other) const
    {
        return (*_iter) == (*other._iter);
    }

    bool operator!=(const self& other) const
    {
        return !((*this) == other);
    }
};

template <class TIterator>
class linq_collection;

template <class T>
class linq;

template <class TContainer>
auto from(const TContainer& cont) -> linq_collection<decltype(std::cbegin(cont))>;

template <class TIterator>
auto from(const TIterator& a, const TIterator& b) -> linq_collection<TIterator>;

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

template <class TContainer/*, std::enable_if_t<!std::is_same<
              std::decay_t<TContainer>,
              std::initializer_list<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>>::value>* = nullptr*/>
auto from_values(const TContainer& cont) -> linq<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>
{
    auto xs = std::make_shared<TContainer>(cont);
    using iter_type = boxed_container_iterator<TContainer>;
    return linq_collection<iter_type>(
        iter_type(xs, std::begin(*xs)),
        iter_type(xs, std::end(*xs))
    );
}

template <class TContainer/*, std::enable_if_t<!std::is_same<
              std::decay_t<TContainer>,
              std::initializer_list<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>>::value>* = nullptr*/>
auto from_values(TContainer&& cont)
    -> linq<deref_iter_t<decltype(std::begin(std::declval<TContainer>()))>>
{
    auto xs = std::make_shared<TContainer>(std::move(cont));
    using iter_type = boxed_container_iterator<TContainer>;
    return linq_collection<iter_type>(
        iter_type(xs, std::begin(*xs)),
        iter_type(xs, std::end(*xs))
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
        return {
            select_iterator<TIterator, TFunction>{_begin, func},
            select_iterator<TIterator, TFunction>{_end, func}
        };
    }

    template <class TFunction>
    linq_collection<where_iterator<TIterator, TFunction>> where(const TFunction& func) const
    {
        return {
            where_iterator<TIterator, TFunction>{_begin, _end, func},
            where_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    linq_collection<skip_iterator<TIterator>> skip(std::size_t count) const
    {
        return {
            skip_iterator<TIterator>{_begin, _end, count},
            skip_iterator<TIterator>{_end, _end, count},
        };
    }

    template <class TFunction>
    linq_collection<skip_while_iterator<TIterator, TFunction>> skip_while(const TFunction& func) const
    {
        return {
            skip_while_iterator<TIterator, TFunction>{_begin, _end, func},
            skip_while_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    linq_collection<take_iterator<TIterator>> take(std::size_t count) const
    {
        return {
            take_iterator<TIterator>{_begin, _end, count},
            take_iterator<TIterator>{_end, _end, count}
        };
    }

    template <typename TFunction>
    linq_collection<take_while_iterator<TIterator, TFunction>> take_while(const TFunction& func) const
    {
        return {
            take_while_iterator<TIterator, TFunction>{_begin, _end, func},
            take_while_iterator<TIterator, TFunction>{_end, _end, func}
        };
    }

    template <class TIterator2>
    linq_collection<concat_iterator<TIterator, TIterator2>> concat_impl(const linq_collection<TIterator2>& other) const
    {
        return {
            concat_iterator<TIterator, TIterator2>{_begin, _end, other.begin(), other.end()},
            concat_iterator<TIterator, TIterator2>{_end, _end, other.end(), other.end()}
        };
    }

    template <class TIterator2>
    auto concat(const linq_collection<TIterator2>& cont)
    -> decltype(concat_impl(cont))
    {
        return concat_impl(cont);
    }

    template <class TContainer>
    auto concat(const TContainer& other) const
    -> decltype(concat_impl(from(other)))
    {
        return concat_impl(from(other));
    }

    template <class T>
    auto concat(const std::initializer_list<T>& ilist) const
    -> decltype(concat_impl(from(ilist)))
    {
        return concat_impl(from(ilist));
    }

    template <class T>
    auto contains(const T& t) const -> decltype(std::declval<value_type>() == std::declval<T>() , bool())
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

    std::size_t count() const
    {
        std::size_t c = 0;
        for (auto i = _begin; i != _end; ++i)
        {
            c++;
        }
        return c;
    }

    linq<value_type> default_if_empty(const value_type& v) const
    {
        if (empty())
        {
            return from_value(v);
        }
        return *this;
    }

    value_type element_at(std::size_t i) const
    {
        auto iter = _begin;
        for (; iter != _end && i != 0; ++iter , i--)
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
    bool sequence_equal_impl(const linq_collection<TIterator2>& e) const
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

    template <class TIterator2>
    auto sequence_equal(const linq_collection<TIterator2>& cont) const
    -> decltype(sequence_equal_impl(cont))
    {
        return sequence_equal_impl(cont);
    }

    template <class TContainer>
    auto sequence_equal(const TContainer& other) const
    -> decltype(sequence_equal_impl(from(other)))
    {
        return sequence_equal_impl(from(other));
    }

    template <class T>
    auto sequence_equal(const std::initializer_list<T>& ilist) const
    -> decltype(sequence_equal_impl(from(ilist)))
    {
        return sequence_equal_impl(from(ilist));
    }

    linq<value_type> distinct() const
    {
        std::set<value_type> s;
        std::vector<value_type> v;
        std::for_each(begin(), end(), [&s, &v](const auto& x)
                      {
                          if (s.insert(x).second)
                          {
                              v.push_back(x);
                          }
                      });

        return from_values(std::move(v));
    }

    template <class TIterator2>
    linq<value_type> except_with_impl(const linq_collection<TIterator2>& e) const
    {
        std::set<value_type> s(e.begin(), e.end());
        std::vector<value_type> xs;
        std::for_each(_begin, _end, [&](const value_type& value)
                      {
                          if (s.insert(value).second)
                          {
                              xs.push_back(value);
                          }
                      });
        return from_values(std::move(xs));
    }

    template <class TContainer>
    auto except_with(const TContainer& other) const
    -> decltype(except_with_impl(from(other)))
    {
        return except_with_impl(from(other));
    }

    template <class T>
    auto except_with(const std::initializer_list<T>& ilist) const
    -> decltype(except_with_impl(from(ilist)))
    {
        return except_with_impl(from(ilist));
    }

    template <class TIterator2>
    auto except_with(const linq_collection<TIterator2>& e) const
    -> decltype(except_with_impl(e))
    {
        return except_with_impl(e);
    }


    template <class TIterator2,
              std::enable_if_t<std::is_same<value_type, deref_iter_t<TIterator2>>::value>* = nullptr>
    linq<value_type> intersect_with_impl(const linq_collection<TIterator2>& e) const
    {
        std::set<value_type> s1, s2(e.begin(), e.end());
        std::vector<value_type> res;
        std::for_each(begin(), end(), [&s1, &s2, &res](const value_type& x)
                      {
                          if (s1.insert(x).second && !s2.insert(x).second)
                          {
                              res.push_back(x);
                          }
                      });
        return from_values(std::move(res));
    }

    template <class TIterator2>
    auto intersect_with(const linq_collection<TIterator2>& e) const
    -> decltype(intersect_with_impl(e))
    {
        return intersect_with_impl(e);
    }

    template <class TContainer>
    auto intersect_with(const TContainer& other) const
    -> decltype(intersect_with_impl(from(other)))
    {
        return intersect_with_impl(from(other));
    }

    template <class T>
    auto intersect_with(const std::initializer_list<T>& ilist) const
    -> decltype(intersect_with_impl(from(ilist)))
    {
        return intersect_with_impl(from(ilist));
    }

    template <class TIterator2,
              std::enable_if_t<std::is_same<value_type, deref_iter_t<TIterator2>>::value>* = nullptr>
    linq<value_type> union_with_impl(const linq_collection<TIterator2>& e) const
    {
        return concat(e).distinct();
    }

    template <class TIterator2>
    auto union_with(const linq_collection<TIterator2>& e) const
    -> decltype(union_with_impl(e))
    {
        return union_with_impl(e);
    }

    template <class TContainer>
    auto union_with(const TContainer& other) const
    -> decltype(union_with_impl(from(other)))
    {
        return union_with_impl(from(other));
    }

    template <class T>
    auto union_with(const std::initializer_list<T>& ilist) const
    -> decltype(union_with_impl(from(ilist)))
    {
        return union_with_impl(from(ilist));
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
        std::for_each(it, _end, [&res, &f](const value_type& x)
                      {
                          res = f(res, x);
                      });
        return res;
    }

    template <class TResult, class TFunction>
    TResult aggregate(const TResult& init, const TFunction& f) const
    {
        TResult res = init;
        for (auto it = _begin; it != _end; ++it)
        {
            res = f(res, *it);
        }
        return res;
    }

    template <class TFunction>
    bool all(const TFunction& f) const
    {
        return select(f).aggregate(true, [](bool a, bool b)
                                   {
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
        std::for_each(_begin, _end, [&sum, &counter](const value_type& x)
                      {
                          sum += x;
                          counter++;
                      });
        return sum / counter;
    }

    value_type max() const
    {
        return aggregate([](const auto& x, const auto& y)
            {
                return x > y ? x : y;
            });
    }

    value_type min() const
    {
        return aggregate([](const auto& x, const auto& y)
            {
                return x < y ? x : y;
            });
    }

    value_type sum() const
    {
        return aggregate([](const auto& x, const auto& y)
            {
                return x + y;
            });
    }

    value_type product() const
    {
        return aggregate([](const auto& x, const auto& y)
            {
                return x + y;
            });
    }


    template <typename TFunction>
    auto select_many(const TFunction& f) const
    -> linq<decltype(*f(std::declval<value_type>()).begin())>
    {
        using collection_type = decltype(f(std::declval<value_type>()));
        using collection_value_type = decltype(*f(std::declval<value_type>()).begin());

        return select(f).aggregate(from_empty<collection_value_type>(),
                                   [](const linq<collection_value_type>& a, const collection_type& b)
                                   {
                                       return a.concat(b);
                                   });
    }

    template <class TFunction>
    auto group_by(const TFunction& keySelector) const
    -> linq<std::pair<typename callable_traits<TFunction>::return_type, linq<value_type>>>
    {
        using key_type = typename callable_traits<TFunction>::return_type;
        using value_vector = std::vector<value_type>;

        std::map<key_type, value_vector> m;
        for (const auto& elm : *this)
        {
            auto key = keySelector(elm);
            auto iter = m.find(key);
            if (iter == m.end())
            {
                m[key].push_back(elm);
            }
            else
            {
                iter->second.push_back(elm);
            }
        }

        std::vector<std::pair<key_type, linq<value_type>>> res;
        for (const auto& p : m)
        {
            res.push_back({p.first, from_values(std::move(p.second))});
        }
        return from_values(std::move(res));
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto full_join_impl(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> linq<std::tuple<std::remove_reference_t<typename callable_traits<TFunction1>::return_type>,
                       linq<std::decay_t<deref_iter_t<TIterator>>>,
                       linq<std::decay_t<deref_iter_t<TIterator2>>>>>
    {
        using key_type = std::remove_reference_t<typename callable_traits<TFunction1>::return_type>;
        using value_type1 = std::decay_t<deref_iter_t<TIterator>>;
        using value_type2 = std::decay_t<deref_iter_t<TIterator2>>;
        using full_join_pair_t = std::tuple<key_type, linq<value_type1>, linq<value_type2>>;

        std::multimap<key_type, value_type1> map1;
        std::multimap<key_type, value_type2> map2;

        std::for_each(_begin, _end, [&map1, &keySelector1](const value_type1& value)
                      {
                          map1.insert({keySelector1(value), value});
                      });

        std::for_each(e.begin(), e.end(), [&map2, &keySelector2](const value_type2& value)
                      {
                          map2.insert({keySelector2(value), value});
                      });

        std::vector<full_join_pair_t> result;
        auto lower1 = map1.begin();
        auto lower2 = map2.begin();

        while (lower1 != map1.end() && lower2 != map2.end())
        {
            auto& key1 = lower1->first;
            auto& key2 = lower2->first;
            auto upper1 = map1.upper_bound(key1);
            auto upper2 = map2.upper_bound(key2);

            if (key1 < key2)
            {
                std::vector<value_type1> outers;
                std::for_each(lower1, upper1, [&outers](const std::pair<key_type, value_type1>& it)
                              {
                                  outers.push_back(it.second);
                              });
                lower1 = upper1;
                result.emplace_back(key1, from_values(std::move(outers)), from_empty<value_type2>());
            }
            else if (key1 > key2)
            {
                std::vector<value_type2> inners;
                std::for_each(lower2, upper2, [&inners](const std::pair<key_type, value_type2>& it)
                              {
                                  inners.push_back(it.second);
                              });
                lower2 = upper2;
                result.emplace_back(key2, from_empty<value_type1>(), from_values(std::move(inners)));
            }
            else
            {
                std::vector<value_type1> outers;
                std::for_each(lower1, upper1, [&outers](const std::pair<key_type, value_type1>& it)
                              {
                                  outers.push_back(it.second);
                              });
                std::vector<value_type2> inners;
                std::for_each(lower2, upper2, [&inners](const std::pair<key_type, value_type2>& it)
                              {
                                  inners.push_back(it.second);
                              });
                lower1 = upper1;
                lower2 = upper2;
                result.emplace_back(key1, from_values(std::move(outers)), from_values(std::move(inners)));
            }
        }
        return from_values(std::move(result));
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto full_join(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> decltype(full_join_impl(e, keySelector1, keySelector2))
    {
        return full_join_impl(e, keySelector1, keySelector2);
    }

    template <class TContainer, class TFunction1, class TFunction2>
    auto full_join(const TContainer& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(full_join_impl(from(e), keySelector1, keySelector2))
    {
        return full_join_impl(from(e), keySelector1, keySelector2);
    }

    template <class T, class TFunction1, class TFunction2>
    auto full_join(const std::initializer_list<T>& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(full_join_impl(from(e), keySelector1, keySelector2))
    {
        return full_join_impl(from(e), keySelector1, keySelector2);
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto group_join_impl(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> linq<std::tuple<std::remove_reference_t<decltype(keySelector1(std::declval<value_type>()))>,
                       std::decay_t<deref_iter_t<TIterator>>,
                       linq<std::decay_t<deref_iter_t<TIterator2>>>>>
    {
        using key_type = std::remove_reference_t<decltype(keySelector1(std::declval<value_type>()))>;
        using value_type1 = std::decay_t<deref_iter_t<TIterator>>;
        using value_type2 = std::decay_t<deref_iter_t<TIterator2>>;
        using full_join_pair_t = std::tuple<key_type, linq<value_type1>, linq<value_type2>>;
        using group_join_pair_t = std::tuple<key_type, value_type1, linq<value_type2>>;

        auto f = full_join(e, keySelector1, keySelector2);
        auto g = f.select_many([](const full_join_pair_t& item) -> linq<group_join_pair_t>
            {
                const auto& outers = std::get<1>(item);
                //MUST CAPTURE item BY VALUE
                return outers.select([item](const value_type1& outer) -> group_join_pair_t
                    {
                        return {std::get<0>(item), outer, std::get<2>(item)};
                    });
            });
        return g;
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto group_join(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> decltype(group_join_impl(e, keySelector1, keySelector2))
    {
        return group_join_impl(e, keySelector1, keySelector2);
    }

    template <class TContainer, class TFunction1, class TFunction2>
    auto group_join(const TContainer& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(group_join_impl(from(e), keySelector1, keySelector2))
    {
        return group_join_impl(from(e), keySelector1, keySelector2);
    }

    template <class T, class TFunction1, class TFunction2>
    auto group_join(const std::initializer_list<T>& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(group_join_impl(from(e), keySelector1, keySelector2))
    {
        return group_join_impl(from(e), keySelector1, keySelector2);
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto join_impl(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> linq<std::tuple<std::remove_reference_t<decltype(keySelector1(std::declval<value_type>()))>,
                       std::decay_t<deref_iter_t<TIterator>>,
                       std::decay_t<deref_iter_t<TIterator2>>>>
    {
        using key_type = std::remove_reference_t<decltype(keySelector1(std::declval<value_type>()))>;
        using value_type1 = std::decay_t<deref_iter_t<TIterator>>;
        using value_type2 = std::decay_t<deref_iter_t<TIterator2>>;
        using group_join_pair_t = std::tuple<key_type, value_type1, linq<value_type2>>;
        using join_pair_t = std::tuple<key_type, value_type1, value_type2>;

        auto g = group_join(e, keySelector1, keySelector2);
        auto j = g.select_many([](const group_join_pair_t& item)-> linq<join_pair_t>
            {
                const linq<value_type2>& inners = std::get<2>(item);
                return inners.select([item](const value_type2& inner)-> join_pair_t
                    {
                        return {std::get<0>(item), std::get<1>(item), inner};
                    });
            });
        return j;
    }

    template <class TIterator2, class TFunction1, class TFunction2>
    auto join(const linq_collection<TIterator2>& e, const TFunction1& keySelector1, const TFunction2& keySelector2) const
    -> decltype(join_impl(e, keySelector1, keySelector2))
    {
        return join_impl(e, keySelector1, keySelector2);
    }

    template <class TContainer, class TFunction1, class TFunction2>
    auto join(const TContainer& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(join_impl(from(e), keySelector1, keySelector2))
    {
        return join_impl(from(e), keySelector1, keySelector2);
    }

    template <class T, class TFunction1, class TFunction2>
    auto join(const std::initializer_list<T>& e, const TFunction1 keySelector1, const TFunction2 keySelector2) const
    -> decltype(join_impl(from(e), keySelector1, keySelector2))
    {
        return join_impl(from(e), keySelector1, keySelector2);
    }

    template <class TFunction>
    auto first_order_by(const TFunction& keySelector) const
    -> linq<linq<value_type>>
    {
        using key_type = std::decay_t<decltype(keySelector(std::declval<value_type>()))>;
        return group_by(keySelector).select([](const std::pair<key_type, linq<value_type>>& pair)
            {
                return pair.second;
            });
    }

    template <class TFunction>
    auto then_order_by(const TFunction& keySelector) const
    -> linq<value_type>
    {
        return select_many([&keySelector](const value_type& values)
            {
                return values.first_order_by(keySelector);
            });
    }

    template <class TFunction>
    auto order_by(const TFunction& keySelector) const
    -> linq<value_type>
    {
        return first_order_by(keySelector).select_many([](const linq<value_type>& values)
            {
                return values;
            });
    }

    template <class TIterator2>
    auto zip_with_impl(const linq_collection<TIterator2>& e) const
    -> linq_collection<zip_iterator<TIterator, TIterator2>>
    {
        return {
            zip_iterator<TIterator, TIterator2>{_begin, _end, e.begin(), e.end()},
            zip_iterator<TIterator, TIterator2>{_end, _end, e.end(), e.end()}
        };
    }

    template <class TIterator2>
    auto zip_with(const linq_collection<TIterator2>& e) const
    -> decltype(zip_with_impl(e))
    {
        return zip_with_impl(e);
    }

    template <class TContainer>
    auto zip_with(const TContainer& other) const
    -> decltype(zip_with_impl(from(other)))
    {
        return zip_with_impl(from(other));
    }

    template <class T>
    auto zip_with(const std::initializer_list<T>& ilist) const
    -> decltype(zip_with_impl(from(ilist)))
    {
        return zip_with_impl(from(ilist));
    }

    template <class TContainer>
    TContainer to_container() const
    {
        return TContainer{_begin, _end};
    }

    auto to_vector() const
    {
        return to_container<std::vector<value_type>>();
    }

    auto to_deque() const
    {
        return to_container<std::deque<value_type>>();
    }

    auto to_list() const
    {
        return to_container<std::list<value_type>>();
    }

    template <class TContainer, class TFunction>
    TContainer to_key_value_container(const TFunction& keySelector) const
    {
        auto g = select(keySelector).zip_with(*this);
        return {g.begin(), g.end()};
    }

    template <class TFunction>
    auto to_map(const TFunction& keySelector)
    {
        return to_key_value_container<std::map<typename callable_traits<TFunction>::return_type, value_type>>(keySelector);
    }

    template <class TFunction>
    auto to_multimap(const TFunction& keySelector)
    {
        return to_key_value_container<std::multimap<typename callable_traits<TFunction>::return_type, value_type>>(keySelector);
    }

    template <class TFunction>
    auto to_unordered_map(const TFunction& keySelector)
    {
        return to_key_value_container<std::unordered_map<typename callable_traits<TFunction>::return_type, value_type>>(keySelector);
    }

    template <class TFunction>
    auto to_unordered_multimap(const TFunction& keySelector)
    {
        return to_key_value_container<std::unordered_multimap<typename callable_traits<TFunction>::return_type, value_type>>(keySelector);
    }

    auto to_set()
    {
        return to_container<std::set<value_type>>();
    }

    auto to_multiset()
    {
        return to_key_value_container<std::multiset<value_type>>();
    }

    auto to_unordered_set()
    {
        return to_key_value_container<std::unordered_set<value_type>>();
    }

    auto to_unordered_multiset()
    {
        return to_key_value_container<std::unordered_multiset<value_type>>();
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
        : base(any_iterator<T>(e.begin()),
               any_iterator<T>(e.end())
        )
    {
    }
};

template <class T>
static linq<T> flatten(const linq<linq<T>>& xs)
{
    return xs.select_many([](const linq<T>& ys)
        {
            return ys;
        });
}

template <class TContainer>
auto from(const TContainer& cont) -> linq_collection<decltype(std::cbegin(cont))>
{
    return {std::cbegin(cont), std::cend(cont)};
}

template <class TIterator>
auto from(const TIterator& a, const TIterator& b) -> linq_collection<TIterator>
{
    return {a, b};
}
}
}
