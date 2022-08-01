/**
https://www.foonathan.net/2021/07/concepts-structural-nominal/
**/
template <typename T>
concept equality_comparable = requires (T obj) {
  { obj == obj } -> std::same_as<bool>;
  { obj != obj } -> std::same_as<bool>;
};

struct vec2
{
    float x, y;

    friend bool operator==(vec2 lhs, vec2 rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    // operator!= not needed in C++20 due to operator rewrite rules!
};

static_assert(equality_comparable<vec2>);
