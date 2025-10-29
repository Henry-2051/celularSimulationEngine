#include <map>
#include <cctype>
#include <iostream>

#ifndef CONFIG_HELPERS
#define CONFIG_HELPERS

namespace config_help {

enum class ConfigType : int {
    Int = 0,
    Size_t = 1,
    Bool = 2,
    String = 3
};

inline std::map<std::string, ConfigType> configTypes = {
    {"renderingEngine", ConfigType::String},
    {"screenWidth", ConfigType::Size_t},
    {"screenHeight", ConfigType::Size_t},
    {"fps", ConfigType::Int}
};

template<typename T> T parseValue(std::string input);

template<> 
inline int parseValue(std::string input) {
    int result{};
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), result);
    if (ec == std::errc()) {
        return result;
    } else {
        std::cerr << std::format("error while parsing {} to Int at character {}, {}\n", input, ptr - input.data(),  *ptr);
    }
}

template<> 
inline size_t parseValue(std::string input) {
    int parsed_int = parseValue<int>(input);
    if (parsed_int < 0) {
        std::cerr << std::format("error, {} cannot be converted to size_t", input);
    }
    size_t result = parsed_int;
    return result;
}

inline void to_upper(std::string & s)
{
    std::transform(
        s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return std::toupper(c); }
    );
}

template<>
inline  bool parseValue(std::string input) {
    std::string input_copy(input);
    to_upper(input_copy);

    if (!(input_copy == "TRUE" || input_copy == "FALSE")) {  
        std::cerr << std::format("error parsing {} to bool", input);
        return false;
    }

    return input_copy == "TRUE";
}

template<> 
inline std::string parseValue(std::string input) {
    if (!(input == "OpenGL" || input == "SFML2")) {
        std::cerr << std::format("error string input isnt valid"); 
    }
    return input;
}

inline void print_map(const std::map<std::string,std::variant<int, size_t, bool, std::string>>& m)
{
    for (const auto& [key, value] : m) {
        if (std::holds_alternative<int> (value)) {
            std::cout << '[' << key << "] = " << std::get<int>(value)  << "; ";
        } else if (std::holds_alternative<size_t>(value)) {
            std::cout << '[' << key << "] = " << std::get<size_t>(value) << "; ";
        } else if (std::holds_alternative<bool>(value)) {
            std::cout << '[' << key << "] = " << std::get<bool>(value) << "; ";
        } else if (std::holds_alternative<std::string>(value)) {
            std::cout << '[' << key << "] = " << std::get<std::string>(value) << "; ";
        } else {
            std::cout << "fallthrough \n";
        }
    }
}
}
#endif