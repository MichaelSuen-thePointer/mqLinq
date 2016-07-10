#include "linq_collection.h"
#include <vector>
#include <iostream>
using namespace std;
using namespace pl::linq;

int test(int x)
{
    return x;
}

int main()
{
    auto lamb = [](int x) {return x; };

    callable_traits<decltype(lamb)>::param_type x;

    //////////////////////////////////////////////////////////////////
    // from
    //////////////////////////////////////////////////////////////////
    {
        vector<int> xs = {1, 2, 3, 4, 5};
        int sum = 0;
        for (auto x : from(xs.begin(), xs.end()))
        {
            sum += x;
        }
        assert(sum == 15);
    }
    {
        vector<int> xs = {1, 2, 3, 4, 5};
        int sum = 0;
        for (auto x : from(xs))
        {
            sum += x;
        }
        assert(sum == 15);
    }
    {
        int sum = 0;
        for (auto x : from_values({1, 2, 3, 4, 5}))
        {
            sum += x;
        }
        assert(sum == 15);
    }
    {
        int xs[] = {1, 2, 3, 4, 5};
        int sum = 0;
        for (auto x : from(from(from(xs))))
        {
            sum += x;
        }
        assert(sum == 15);
    }
    //////////////////////////////////////////////////////////////////
    // select
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 2, 3, 4, 5};
        assert(from(xs).select([](int x) {return x * 2; }).sequence_equal({2, 4, 6, 8, 10}));
    }
    //////////////////////////////////////////////////////////////////
    // hide type test
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 2, 3, 4, 5};
        linq<int> hidden = from(xs).select([](int x) {return x * 2; });
        assert(hidden.sequence_equal({2, 4, 6, 8, 10}));
    }
    //////////////////////////////////////////////////////////////////
    // where
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 2, 3, 4, 5};
        assert(from(xs).where([](int x) {return x % 2 == 0; }).sequence_equal({2, 4}));
    }
    //////////////////////////////////////////////////////////////////
    // iterating
    //////////////////////////////////////////////////////////////////
    {
        vector<int> empty;
        int xs[] = {1, 2, 3, 4, 5};
        int ys[] = {1, 2, 3};
        int zs[] = {4, 5};
        assert(from(xs).take(3).sequence_equal(ys));
        assert(from(xs).skip(3).sequence_equal(zs));
        assert(from(xs).take_while([](int a) {return a != 4; }).sequence_equal(ys));
        assert(from(xs).skip_while([](int a) {return a != 4; }).sequence_equal(zs));
        assert(from(xs).take(0).sequence_equal(empty));
        assert(from(xs).skip(5).sequence_equal(empty));
        assert(from(ys).concat(from(zs)).sequence_equal(xs));
        assert(from(xs).concat(from(empty)).sequence_equal(xs));
        assert(from(empty).concat(from(xs)).sequence_equal(xs));
        assert(from(empty).concat(from(empty)).sequence_equal(empty));
        assert(from(ys).concat(zs).sequence_equal(xs));
        assert(from(xs).concat(empty).sequence_equal(xs));
        assert(from(empty).concat(xs).sequence_equal(xs));
        assert(from(empty).concat(empty).sequence_equal(empty));
    }
    //////////////////////////////////////////////////////////////////
    // counting
    //////////////////////////////////////////////////////////////////
    {
        int a[] = {1, 2, 3, 4, 5};
        vector<int> b = {1, 2, 3, 4, 5};
        vector<int> c;
        int d[] = {1, 2, 3, 4};
        int e[] = {1, 2, 3, 4, 5, 6};
        int f[] = {6, 7, 8, 9, 10};
        int g[] = {0};
        linq<int> xs[] = {from(b), from(c), from(d), from(e), from(f)};

        assert(from(a).sequence_equal(b));
        for (auto& x : xs)
        {
            for (auto& y : xs)
            {
                assert(x.sequence_equal(y) == (&x == &y));
            }
        }

        assert(from(a).contains(1));
        assert(from(a).contains(5));
        assert(!from(a).contains(6));
        assert(!from(c).contains(6));

        assert(from(a).count() == 5);
        assert(from(c).count() == 0);

        assert(from(a).default_if_empty(0).sequence_equal(b));
        assert(from(c).default_if_empty(0).sequence_equal(g));

        assert(from(a).element_at(0) == 1);
        assert(from(a).element_at(4) == 5);
        try { from(a).element_at(-1); assert(false); }
        catch (const std::exception&) {}
        try { from(a).element_at(6); assert(false); }
        catch (const std::exception&) {}
        try { from(c).element_at(0); assert(false); }
        catch (const std::exception&) {}

        assert(!from(a).empty());
        assert(from(c).empty());

        assert(from(a).first() == 1);
        assert(from(a).first_or_default(0) == 1);
        assert(from(a).last() == 5);
        assert(from(a).last_or_default(0) == 5);
        assert(from(c).first_or_default(0) == 0);
        assert(from(c).last_or_default(0) == 0);
        try { from(c).first(); assert(false); }
        catch (const std::exception&) {}
        try { from(c).last(); assert(false); }
        catch (const std::exception&) {}

        assert(from(c).single_or_default(0).sequence_equal(g));
        assert(from(g).single().sequence_equal(g));
        try { from(a).single(); assert(false); }
        catch (const std::exception&) {}
        try { from(a).single_or_default(0); assert(false); }
        catch (const std::exception&) {}
        try { from(c).single(); assert(false); }
        catch (const std::exception&) {}
    }
    //////////////////////////////////////////////////////////////////
    // containers
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 2, 3, 4, 5};
        assert(from(xs).sequence_equal(from(xs).to_vector()));
        assert(from(xs).sequence_equal(from(xs).to_deque()));
        assert(from(xs).sequence_equal(from(xs).to_list()));
        assert(from(xs).sequence_equal(from(xs).to_set()));

        auto f = [](int x) {return x; };
        assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p) {return p.first; })));
        assert(from(xs).sequence_equal(from(from(xs).to_map(f)).select([](pair<int, int> p) {return p.second; })));
        assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p) {return p.first; })));
        assert(from(xs).sequence_equal(from(from(xs).to_multimap(f)).select([](pair<int, int> p) {return p.second; })));
    }
    //////////////////////////////////////////////////////////////////
    // aggregating
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 2, 3, 4, 5};
        assert(from(xs).aggregate([](int a, int b) {return a + b; }) == 15);
        assert(from(xs).aggregate(0, [](int a, int b) {return a + b; }) == 15);
        assert(from(xs).sum() == 15);
        assert(from(xs).aggregate([](int a, int b) {return a * b; }) == 120);
        assert(from(xs).aggregate(1, [](int a, int b) {return a * b; }) == 120);
        assert(from(xs).product() == 120);
        assert(from(xs).all([](int a) {return a > 1; }) == false);
        assert(from(xs).all([](int a) {return a > 0; }) == true);
        assert(from(xs).any([](int a) {return a > 1; }) == true);
        assert(from(xs).any([](int a) {return a > 0; }) == true);
        assert(from(xs).min() == 1);
        assert(from(xs).max() == 5);
        assert(from(xs).average<double>() == 3.0);

        vector<int> ys;
        try { from(ys).product(); assert(false); }
        catch (const std::exception&) {}
        try { from(ys).min(); assert(false); }
        catch (const std::exception&) {}
        try { from(ys).max(); assert(false); }
        catch (const std::exception&) {}
        try { from(ys).average<int>(); assert(false); }
        catch (const std::exception&) {}
    }
    //////////////////////////////////////////////////////////////////
    // set
    //////////////////////////////////////////////////////////////////
    {
        int xs[] = {1, 1, 2, 2, 3, 3};
        int ys[] = {2, 2, 3, 3, 4, 4};
        assert(from(xs).distinct().sequence_equal({1, 2, 3}));
        assert(from(ys).distinct().sequence_equal({2, 3, 4}));
        assert(from(xs).except_with(ys).sequence_equal({1}));
        assert(from(xs).intersect_with(ys).sequence_equal({2, 3}));
        assert(from(xs).union_with(ys).sequence_equal({1, 2, 3, 4}));
    }

}
