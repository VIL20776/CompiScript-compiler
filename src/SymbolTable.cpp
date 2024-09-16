#include "SymbolTable.h"

#include <utility>
#include <set>

SymbolTable::SymbolTable():
    scopes()
{
}

SymbolTable::~SymbolTable()
{
}

void SymbolTable::insert(const SymbolData &symbol) {
    auto key = std::make_pair(symbol.name, "");
    current_scope.emplace(key, symbol);
}

std::vector<SymbolData>
SymbolTable::find_range(const std::string &label) {
    std::vector<SymbolData> symbols;
    for (auto &scope: scopes)
    for (auto &kv : scope) 
    {
        auto &key = kv.first;
        if (key.second == label)
            symbols.push_back(kv.second);
    }

    return symbols;
}

std::pair<SymbolData, bool>
SymbolTable::find(const std::string &symbol_name, const std::string &label) {
    // Try finding in current scope
    auto key = std::make_pair(symbol_name, label);
    if (current_scope.contains(key))
        return {current_scope.at(key), true};

    // Find in previous scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); it++)
        if (it->contains(key))
            return {current_scope.at(key), true};
    
    return {{}, false};
}


bool SymbolTable::update(const std::string &symbol_name, const SymbolData &symbol, const std::string &label) {
    // Try finding in current scope
    auto key = std::make_pair(symbol_name, label);
    if (current_scope.contains(key)) {
        current_scope.insert_or_assign(key, symbol);
        return true;
    }

    // Find in previous scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); it++)
        if (it->contains(key)) {
            it->insert_or_assign(key, symbol);
            return true;
        }
    
    return false;
    
}

void SymbolTable::enter(const std::vector<SymbolData> &initial_symbols) {
    scopes.push_back(current_scope);
    current_scope = {};
}

void SymbolTable::exit() {
    current_scope = scopes.back();
    scopes.pop_back();
}
