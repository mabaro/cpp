/**
https://en.cppreference.com/w/cpp/memory/align
**/
#include <vector>
#include <algorithm> // std::find
#include <numeric> // std::accumulate
#include <memory> // malloc
#include <iostream> // std::cout
#include <cassert>
#include <chrono>
#include <sstream>

template<typename TClock = std::chrono::high_resolution_clock>
class Timer
{
public:
    using chrono_t = TClock;
    using timepoint = typename chrono_t::time_point;
    using duration = typename chrono_t::duration;

    using secondsf = std::chrono::duration<float>;
    using millisecondsf = std::chrono::duration<float, std::milli>;

protected:
    timepoint m_begin;
    std::string m_tag;

public:
    Timer(const std::string& tag = "")
        : m_begin(Now())
        , m_tag(tag)
    {}
    ~Timer()
    {
        auto elapsedDuration = GetElapsed();
        auto humanReadableDuration = [](const duration& t) {
            std::ostringstream ostr;
            if (t > std::chrono::seconds(1))
            {
                ostr << std::chrono::duration_cast<secondsf>(t).count() << "s";
            }
            else if (t > std::chrono::milliseconds(1))
            {
                ostr << std::chrono::duration_cast<millisecondsf>(t).count() << "ms";
            }
            else if (t > std::chrono::microseconds(1))
            {
                ostr << std::chrono::duration_cast<std::chrono::microseconds>(t).count() << "us";
            }
            else
            {
                ostr << std::chrono::duration_cast<std::chrono::nanoseconds>(t).count() << "ns";
            }

            return ostr.str();
        };
        std::cout << "Duration(" << m_tag << "): " << humanReadableDuration(elapsedDuration) << std::endl;
    }

    static timepoint Now() { return chrono_t::now(); }

    void Start()
    {
        m_begin = Now();
    }

    duration GetElapsed() const
    {
        return Now() - m_begin;
    }

    duration GetElapsedAndReset() const
    {
        auto beginT = m_begin;
        m_begin = Now();
        return Now() - beginT;
    }

};

template<class T>
class IAllocator {
public:
    using value_type = T;

public:
    virtual ~IAllocator() = default;

    virtual T* allocate(int n) = 0;
    virtual void deallocate(T* p, int n) = 0;

    virtual void construct(T* p, const T& v) = 0;
    virtual void destroy(T* p) = 0;
};

template<typename T>
class DummyAllocator : public IAllocator<T>
{
    size_t m_countAllocs = 0;
    size_t m_countConstructs = 0;

public:
    DummyAllocator() {}
    ~DummyAllocator()
    {
        assert(m_countAllocs == 0);
        std::cout << "Allocations: " << m_countAllocs << std::endl;
        std::cout << "Constructs: " << m_countConstructs << std::endl;
    }

    T* allocate(int n)
    {
        T* alloc = reinterpret_cast<T*>(malloc(n * sizeof(T)));
        if (alloc)
        {
            m_countAllocs += n;
        }
        return alloc;
    }
    void deallocate(T* p, int n)
    {
        free(p);
        m_countAllocs -= n;
    }

    void construct(T* p, const T& v)
    {
        p = new (p)T(v);
        m_countConstructs++;
    }
    void destroy(T* p)
    {
        p->~T();
        m_countConstructs--;
    }

};

template<typename T>
class LinearAllocator : public IAllocator<T>
{
    T* m_buffer;
    size_t m_bufferSize;
    T* m_next = nullptr;

    size_t m_countAllocs = 0;
    size_t m_countConstructs = 0;

public:
    LinearAllocator(void* buffer, size_t bufferBytes)
        : m_buffer(reinterpret_cast<T*>(buffer))
        , m_bufferSize(bufferBytes / sizeof(T))
        , m_next(m_buffer)
    {
    }
    ~LinearAllocator()
    {
        assert(m_countAllocs == 0);
        std::cout << "Allocations: " << (m_next - m_buffer) / sizeof(T)  << std::endl;
        std::cout << "Constructs: " << m_countConstructs << std::endl;
    }

    T* allocate(int n)
    {
        T* alloc = nullptr;

        if (m_next + n < m_buffer + m_bufferSize)
        {
            alloc = m_next;
            m_next += n;
            m_countAllocs += n;
        }
        else
        {
            std::cerr << "No memory available" << std::endl;
        }
        return alloc;
    }
    void deallocate(T* p, int n)
    {
        //free(p);
        m_countAllocs -= n;
    }

    void construct(T* p, const T& v)
    {
        p = new (p)T(v);
        m_countConstructs++;
    }
    void destroy(T* p)
    {
        p->~T();
        m_countConstructs--;
    }
};

template<size_t Capacity, size_t Alignment>
class arena_naive
{
    char m_buffer[Capacity + Alignment];
    
    char* m_alignedPtr0;
    char* m_nextPtr;
protected:
    static inline intptr_t align(intptr_t n)
    {
        return (n + (Alignment-1)) & ~(Alignment-1);
    }
    static inline uintptr_t align(uintptr_t n)
    {
        return (n + (Alignment-1)) & ~(Alignment-1);
    }

public:
    arena_naive()
    : m_alignedPtr0(reinterpret_cast<char*>(align(reinterpret_cast<uintptr_t>(m_buffer))))
    , m_nextPtr(m_alignedPtr0)
    {
        static_assert((Alignment & 0x01) == 0, "Alignment has to be a power of 2");
    }
    
    char* allocate(size_t n)
    {
        const size_t alignedSize = align(n);
        if (m_nextPtr + alignedSize < m_alignedPtr0 + Capacity)
        {
            char* alloc = m_nextPtr;
            m_nextPtr += alignedSize;
            return alloc;
        }
        
        std::cerr << "No memory available in the arena" << std::endl;
        //assert(false);
        
        return static_cast<char*>(::operator new(n));
    }
    
    void deallocate(char* p, size_t n)
    {
        if (m_alignedPtr0 <= p && p <= m_alignedPtr0 + Capacity)
        {
            const size_t alignedSize = align(n);
            if (p + alignedSize == m_nextPtr) // 
            {
                m_nextPtr = p;
            }
        }
        else
        {
            std::cerr << "cannot deallocate memory which doesn't below to the arena" << std::endl;
        }
    }
};


template<size_t Capacity, size_t Alignment>
class arena_reusing
{
    char m_buffer[Capacity + Alignment];
    
    char* m_alignedPtr0;
    char* m_nextPtr;

    std::vector<std::pair<char*, size_t>> m_freed;

protected:
    static inline intptr_t align(intptr_t n)
    {
        return (n + (Alignment-1)) & ~(Alignment-1);
    }
    static inline uintptr_t align(uintptr_t n)
    {
        return (n + (Alignment-1)) & ~(Alignment-1);
    }

public:
    arena_reusing()
    : m_alignedPtr0(reinterpret_cast<char*>(align(reinterpret_cast<uintptr_t>(m_buffer))))
    , m_nextPtr(m_alignedPtr0)
    {
        static_assert((Alignment & 0x01) == 0, "Alignment has to be a power of 2");
    }
    
    char* allocate(size_t n)
    {
        const size_t alignedSize = align(n);
        if (m_nextPtr + alignedSize < m_alignedPtr0 + Capacity)
        {
            char* alloc = m_nextPtr;
            m_nextPtr += alignedSize;
            return alloc;
        }
        else if (!m_freed.empty())
        {// compact
            auto it = std::find_if(std::begin(m_freed), std::end(m_freed), [alignedSize](const std::pair<char*, size_t>& data) {
                return data.second >= alignedSize;
            });
            if (it != std::end(m_freed))
            {
                it->second -= alignedSize;
                char* alloc = it->first;
                if (it->second == 0)
                {
                    std::iter_swap(it, std::begin(m_freed) + m_freed.size());
                    m_freed.pop_back();
                }
            }
        }
        
        std::cerr << "No memory available in the arena" << std::endl;        
        char* alloc = static_cast<char*>(::operator new(n + Alignment));
        const intptr_t allocPos = reinterpret_cast<intptr_t>(alloc);
        const intptr_t alignedPos = align(allocPos);
        assert(alignedPos < allocPos + n + Alignment);
        return reinterpret_cast<char*>(alignedPos);
    }
    
    void deallocate(char* p, size_t n)
    {
        if (m_alignedPtr0 <= p && p <= m_alignedPtr0 + Capacity)
        {
            const size_t alignedSize = align(n);
            if (p + alignedSize == m_nextPtr) // 
            {
                m_nextPtr = p;
            }
            else
            {
                m_freed.emplace_back(p, n);
            }
        }
        else
        {
            std::cerr << "cannot deallocate memory which doesn't below to the arena" << std::endl;
        }
    }
};

template<typename T>
class ArenaAllocator : public IAllocator<T>
{
    T* m_buffer;
    size_t m_bufferSize;
    T* m_next = nullptr;

    std::vector<std::pair<T*, size_t>> m_freed;

public:
    ArenaAllocator(void* buffer, size_t bufferBytes)
        : m_buffer(reinterpret_cast<T*>(buffer))
        , m_bufferSize(bufferBytes / sizeof(T))
        , m_next(m_buffer)
    {
    }
    ~ArenaAllocator()
    {
        const size_t freed = std::accumulate(std::begin(m_freed), std::end(m_freed), 0,
            [](size_t accum, const std::pair<T*, size_t>& data) {
                return accum + data.second; 
        });
        if (freed != m_bufferSize)
        {
            std::cerr << "Freed memory: " << freed << " / " << m_bufferSize << std::endl;
        }
    }

    T* allocate(int n)
    {
        T* alloc = nullptr;

        if (m_next + n < m_buffer + m_bufferSize)
        {
            alloc = m_next;
            m_next += n;
        }
        else if (!m_freed.empty())
        {
            auto& freedData =  m_freed.back();
            alloc = freedData.first;
            freedData.first  += n;
            freedData.second -= n;
            if (freedData.second == 0)
            {
                m_freed.pop_back();
            }
        }
        else
        {
            std::cerr << "No memory available" << std::endl;
        }
        return alloc;
    }
    void deallocate(T* p, int n)
    {
        m_freed.emplace_back(p, n);
        // std::sort(std::begin(m_freed), std::end(m_freed), [](const std::pair<T*, size_t>& d1, const std::pair<T*, size_t>& d2) {
        //     return d1.second < d2.second;
        // });
    }

    void construct(T* p, const T& v)
    {
        p = new (p)T(v);
    }
    void destroy(T* p)
    {
        p->~T();
    }
};


template<typename T, typename TPointer  = std::unique_ptr<T>, typename Allocator = std::allocator<T>>
class Factory
{
    Allocator& m_allocator;

public:
    using pointer_t = TPointer;
    static_assert(std::is_same<T*, TPointer>::value == false, "Pointer type must be some kind of smart pointer");

public:
    Factory(Allocator& allocator)
    : m_allocator(allocator)
    {}

    template<typename... Args>
    pointer_t Create(Args... args)
    {
        T* object = m_allocator.allocate(1);
        m_allocator.contruct(object, args...);
    }

    void Destroy(pointer_t object)
    {
        m_allocator.destroy(object);
        m_allocator.deallocate(object);
    }

};

template<typename T, typename Allocator>
bool unit_test_allocator(Allocator& allocator)
{
    T initialValue = T(0);
    T* a = allocator.allocate(1);       // space for one int
    allocator.construct(a, initialValue); // construct the int
    std::cout << a[0] << '\n';
    allocator.destroy(a);           
    allocator.deallocate(a, 1);        // deallocate space for one int
 
    T* s = allocator.allocate(2); // space for 2 strings
 
    allocator.construct(s, initialValue);
    allocator.construct(s + 1, initialValue);
 
    std::cout << s[0] << ' ' << s[1] << '\n';
 
    allocator.destroy(s);
    allocator.destroy(s + 1);
    allocator.deallocate(s, 2);

    return true;
}
template<typename T, typename Allocator, size_t TestCount = 10>
bool unit_test_allocator_vector(Allocator& allocator)
{
    std::vector<T, Allocator> container(allocator);

    const int k_count = TestCount;
    container.reserve(k_count);
    for(int i=0; i<k_count; ++i)
    {
        container.push_back(T(i));        
    }
    std::cout << "container size: " << container.size() << std::endl;

    return true;
}

class Dummy
{
    int a = 0;
    int b = 0;
    std::string str = "";

    template<typename T>
    void initialize(T t)
    {
        a = t;
        b = t;
        str = "test";
    }

public:
    Dummy() {}
    Dummy(int d = -1) : a(d), b(d), str(std::to_string(d)) {}

    friend std::ostream& operator<<(std::ostream& os, const Dummy& d);
    std::ostream& operator<<(std::ostream& os)
    {
        os << "Dummy_"<< a << "_" << b << "_" << str;
        return os;
    }
};

std::ostream& operator<<(std::ostream& os, const Dummy& d)
{
    os << "Dummy_"<< d.a << "_" << d.b << "_" << d.str;
    return os;
}


int main(int, char**)
{
    using value_t = Dummy;
    std::cout << "Sizeof<T>: " << sizeof(value_t) << std::endl;

    /*
    {// simple test
        using allocator_t = DummyAllocator<value_t>;
        allocator_t allocator;
        unit_test_allocator<value_t, allocator_t>(allocator);
    }

    std::cout << std::endl << "vector tests" << std::endl << std::endl;

    // vector tests
    constexpr size_t k_testCount = 1024 * 20;
    {
        using allocator_t = DummyAllocator<value_t>;
        allocator_t allocator;        
        {
            Timer<> t("dummy allocator");
            unit_test_allocator_vector<value_t, allocator_t, k_testCount>(allocator);
        }
    }
    std::cout << std::endl;
    {
        using allocator_t = LinearAllocator<value_t>;
        constexpr size_t poolBytes = sizeof(value_t) * k_testCount * 4;
        char buffer[poolBytes];
        allocator_t allocator(buffer, poolBytes);
        {
            Timer<> t("linear allocator");
            unit_test_allocator_vector<value_t, allocator_t, k_testCount>(allocator);
        }
    }
    std::cout << std::endl;
    {
        using allocator_t = ArenaAllocator<value_t>;
        constexpr size_t poolBytes = sizeof(value_t) * k_testCount * 4;
        char buffer[poolBytes];
        allocator_t allocator(buffer, poolBytes);
        {
            Timer<> t("arena allocator");
            unit_test_allocator_vector<value_t, allocator_t, k_testCount>(allocator);
        }
    }
    */
    
    arena_reusing<10, 4> myArena;
    auto alloc0 = myArena.allocate(4);
    auto alloc1 = myArena.allocate(3);
    myArena.deallocate(alloc0, 4);
    auto alloc2 = myArena.allocate(4);

    // auto alloc2 = myArena.allocate(5);
    // auto alloc3 = myArena.allocate(5);
    // auto alloc4 = myArena.allocate(5);
    // auto alloc5 = myArena.allocate(5);

    return 0;
}
