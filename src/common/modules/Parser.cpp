#include "Parser.hpp"

static void* parse_arg(Type type, const std::string arg) {
    return  type == STRING ? (void*)new std::string(arg) : 
            type == UINT   ? (void*)new uint32_t(std::stoul(arg, nullptr, 0)) : 
            type == FLOAT  ? (void*)new float(std::stof(arg)) : 
                             (void*)new bool(arg == "true");
}

Parser::~Parser() {
    for (auto& [key, value] : flags) {
        if (types[key] == STRING)
            delete (std::string*)value;
        else if (types[key] == UINT)
            delete (uint32_t*)value;
        else if (types[key] == FLOAT)
            delete (float*)value;
        else 
            delete (bool*)value;
    }
}

void Parser::add(std::string flag, Type type, std::string def) {
    types[flag] = type;
    flags[flag] = def != "" ? parse_arg(type, def) : nullptr;
}

bool Parser::parsed(std::string flag) { return flags.find(flag) != flags.end() && flags[flag] != nullptr; }

void Parser::parse(uint32_t argc, const char** argv) {
    for (uint32_t i = 1; i + 1 < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {            
            std::string name = std::string(argv[i]).erase(0, 1);
            flags[name] = parse_arg(types[name], std::string(argv[i++ + 1]));
        }
    }
}