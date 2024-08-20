#include "SymbolTable.h"

#include <utility>
#include <set>

SymbolTable::SymbolTable():
    scopes(0), current_scope(0),
    scope_map(), table()
{
}

SymbolTable::~SymbolTable()
{
}

void SymbolTable::insert(SymbolData &symbol) {
    auto key = std::make_pair(symbol.name, current_scope);
    table.emplace(key, symbol);
}

std::pair<SymbolData, int> SymbolTable::find(std::string &symbol_name, unsigned int &scope) {
    // Try finding in current scope
    auto key = std::make_pair(symbol_name, scope);
    if (table.contains(key))
        return {table.at(key), scope};

    // Get parent scope
    auto [found, end] = scope_map.equal_range(current_scope);
    if (found != end)
    {
        auto [symbol, scope] = find(symbol_name, found->second);
        if (symbol.name == symbol_name)
            return {symbol, scope};
    }    
    
    return {{}, -1};
}

std::pair<SymbolData, int> SymbolTable::find(std::string &symbol_name) {
    return find(symbol_name, current_scope);
}

int SymbolTable::update(std::string &symbol_name, SymbolData symbol) {

    auto [old_symbol, scope] = find(symbol_name, current_scope);
    if (scope < 0)
        return -1;

    table.insert_or_assign({symbol_name, scope}, symbol);
    return scope;
}

int SymbolTable::enter() {
    scope_map.emplace(++scopes, current_scope);
    current_scope = scopes;
    return current_scope;
}

int SymbolTable::exit() {
    auto [start, end] = scope_map.equal_range(current_scope);
    current_scope = start->second;
    return current_scope;
}