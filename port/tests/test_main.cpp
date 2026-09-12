#include "test.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

namespace melee::test {

std::vector<std::pair<std::string, TestFunction>>& registry()
{
    static std::vector<std::pair<std::string, TestFunction>> tests;
    return tests;
}

Register::Register(std::string name, TestFunction function)
{
    registry().emplace_back(std::move(name), std::move(function));
}

void require(bool condition, const char* expression, const char* file, int line)
{
    if (!condition) {
        throw std::runtime_error(std::string(file) + ":" +
                                 std::to_string(line) + ": " + expression);
    }
}

} // namespace melee::test

int main()
{
    std::size_t failures = 0;
    for (const auto& [name, function] : melee::test::registry()) {
        try {
            function();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }

    std::cout << (melee::test::registry().size() - failures) << "/"
              << melee::test::registry().size() << " tests passed\n";
    return failures == 0 ? 0 : 1;
}

