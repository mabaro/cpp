/*
https:  // en.cppreference.com/w/cpp/language/user_literal
*/
namespace test {
class testclass
{
public:
};
}  // namespace test
struct km_tag
{};

template <typename T, typename Tag>
struct units
{
    T value;
};

constexpr units<long double, km_tag> operator"" _km(long double value)
{
    return units<long double, km_tag>{value};
}

int main(int, char**) { return 0; }
