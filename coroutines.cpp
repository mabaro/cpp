/*
compile with -std=c++17 and ensure the compiler supports coroutines (i.e., gcc with coroutines)
*/

#include <future>
#include <coroutine>
#include <iostream>

using namespace std;

future<int> async_add(int a, int b)
{
    auto fut = std::async([=]() {
        int c = a + b;
        return c;
    });

    return fut;
}

future<int> async_fib(int n)
{
    if (n <= 2)
        co_return 1;

    int a = 1;
    int b = 1;

    // iterate computing fib(n)
    for (int i = 0; i < n - 2; ++i)
    {
        int c = co_await async_add(a, b);
        a = b;
        b = c;
    }

    co_return b;
}

future<void> test_async_fib()
{
    for (int i = 1; i < 10; ++i)
    {
        int ret = co_await async_fib(i);
        cout << "async_fib(" << i << ") returns " << ret << endl;
    }
}

int main()
{
    auto fut = test_async_fib();
    fut.wait();

    return 0;
}
