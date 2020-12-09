/*
std::transform/std::reduce now accept an std::execution policy (seq, par, par_unseq) that enables parallel computations

map reduce can be efficiently implemented as:
  vec1{}, vec2{}
  
  std::transform(std::execution::par_unseq
    begin(vec1), end(vec2), begin(vec2), end(vec2), std::back_inserter(output), [](auto v1, auto v2){
      return v1 < v2 ? v1 : v2;
  });
  std::reduce(std::execution::par_unseq
    begin(output), end(output), std::back_inserter(output2), [](auto v1, auto v2){
      return v1 + v2;
  });
  
*/
#include <execution> // sd::par[allel]
#include <numeric> // adjacent_difference
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

template<typename TCHRONO = std::chrono::high_resolution_clock>
std::ostream& operator<< (std::ostream& os, typename TCHRONO::duration d)
{
    const std::chrono::microseconds micros = std::chrono::duration_cast<std::chrono::microseconds>(d);
    if (micros.count() < 1000)
    {
        os << micros.count() <<"us";
        return os;
    }
 
    const std::chrono::milliseconds millis = std::chrono::duration_cast<std::chrono::milliseconds>(d);
    if (millis.count() < 1000)
    {
        os << d.count() << "ms";
        return os;
    }

    const std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(d);
    if (seconds.count() < 60)
    {
        os << d.count() << "s";
        return os;
    }

    const std::chrono::minutes minutes = std::chrono::duration_cast<std::chrono::minutes>(d);
    if (minutes.count() < 60)
    {
        os << d.count() << "min";
        return os;
    }

    const std::chrono::hours hours = std::chrono::duration_cast<std::chrono::hours>(d);
    if (hours.count() < 24)
    {
        os << d.count() << "h";
        return os;
    }

    os << d.count() / 24 << "days";

    return os;
}

template<typename TCHRONO = std::chrono::high_resolution_clock>
class Timer
{
public:
    using chrono_t = TCHRONO;
    using  timepoint_t = typename chrono_t::time_point;
    using duration_t = typename chrono_t::duration;

public:
    Timer(const std::string& tag = "") : m_tag(tag), m_t0() { Tick(); }
    ~Timer() { if (!m_tag.empty()) Print(std::cerr); }
    
    void Print(std::ostream& os = std::cout) const
    {
        os << "Duration(" << GetTag() << "): " << Tock() << std::endl;
    }

    void SetTag(const std::string& tag) { m_tag = tag;}
    const std::string& GetTag() const { return m_tag; }

    void Tick() { m_t0 = chrono_t::now(); }
    duration_t Tock() const { return  chrono_t::now() - m_t0; } 

private:
    timepoint_t m_t0;
    std::string m_tag;
};

template<typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& c)
{
    bool isNotFirst = false;
    for(const auto& v : c)
    {
        if (isNotFirst)
        {
            os << ", ";
        }
        else
        {
            isNotFirst = true;
        }
        os << v;
    }

    return os;
}

////////////////////////////////////////////////////////////

template<typename C>
C fibonacci(size_t i_count)
{
    C result(i_count, 1);
    //std::generate(std::begin(result), std::end(result), [](){return 1; } );

    std::adjacent_difference(std::begin(result), std::prev(std::end(result)),
        std::next(std::begin(result)), std::plus<>());
    return result;
}

////////////////////////////////////////////////////////////

void test_adjacent(size_t i_count)
{
    using num_t = int;
    using container_t = std::vector<num_t>;
    container_t nums;
    nums.resize(i_count);
    num_t acc = 0;
    std::generate(std::begin(nums), std::end(nums), [&acc](){return acc+=acc+1; } );

    container_t nums2;
    std::adjacent_difference(std::begin(nums), std::end(nums), std::back_inserter(nums2));

    std::cout << "nums: " << nums << std::endl;
    std::cout << "adj_diff: " << nums2 << std::endl;

}

////////////////////////////////////////////////////////////

void test_map_reduce(size_t i_count)
{
    using num_t = int;
    std::vector<num_t> data0;
    std::vector<num_t> data1;

    data0.resize(i_count);
    data1.resize(i_count);

    size_t count = 0;
    std::generate(begin(data0), end(data0), [&count](){ return count++; });
    std::generate(begin(data1), end(data1), [&count](){ return count++ % 2 == 0; });

    Timer timer;

    std::vector<num_t> transformed;
    transformed.resize(data0.size());

    {
        Timer timer("seq map/transform");
        std::transform(std::execution::seq,
            begin(data0), end(data0),
            begin(transformed),
                [](const num_t& a) -> num_t {
                    return a + 1;
        });
    }    
    {
        Timer timer("par map/transform");
        std::transform(std::execution::par,
            begin(data0), end(data0),
            begin(transformed),
                [](const num_t& a) -> num_t {
                    return a + 1;
        });
    }    
    {
        Timer timer("par_unseq map/transform");
        std::transform(std::execution::par_unseq,
            begin(data0), end(data0),
            begin(transformed),
                [](const num_t& a) -> num_t {
                    return a + 1;
        });
    }

    //////////////////////////////////////////

    num_t reduceResult = -1;
    {
        Timer timer("seq reduce");
        reduceResult = std::reduce(
            std::execution::seq,
            begin(data0), end(data0), 0);
    }
    {
        Timer timer("par reduce");
        reduceResult = std::reduce(
            std::execution::par,
            begin(data0), end(data0), 0);
    }
    {
        Timer timer("par_unseq reduce");
        reduceResult = std::reduce(
            std::execution::par_unseq,
            begin(data0), end(data0), 0);
    }
    std::cout << "reduce: " << reduceResult << std::endl;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

int main(int, char**)
{
    std::cout << "fib: " << fibonacci<std::vector<int>>(10) << std::endl;

    test_adjacent(5);

    test_map_reduce(5000);

    return 0;
}
