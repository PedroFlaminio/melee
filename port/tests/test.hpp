#ifndef MELEE_HOST_TEST_HPP
#define MELEE_HOST_TEST_HPP

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace melee::test {

using TestFunction = std::function<void()>;

std::vector<std::pair<std::string, TestFunction>>& registry();

class Register final {
public:
    Register(std::string name, TestFunction function);
};

void require(bool condition, const char* expression, const char* file, int line);

} // namespace melee::test

#define MELEE_TEST_CONCAT_INNER(a, b) a##b
#define MELEE_TEST_CONCAT(a, b) MELEE_TEST_CONCAT_INNER(a, b)
#define TEST_CASE(name)                                                        \
    static void MELEE_TEST_CONCAT(test_, __LINE__)();                          \
    static const ::melee::test::Register MELEE_TEST_CONCAT(reg_, __LINE__)(    \
        name, MELEE_TEST_CONCAT(test_, __LINE__));                             \
    static void MELEE_TEST_CONCAT(test_, __LINE__)()
#define REQUIRE(expression)                                                    \
    ::melee::test::require((expression), #expression, __FILE__, __LINE__)

#endif

