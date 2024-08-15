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

SymbolData SymbolTable::find(std::string &symbol_name, unsigned int &scope) {
    // Try finding in current scope
    auto key = std::make_pair(symbol_name, scope);
    if (table.contains(key))
        return table.at(key);
    
    auto [start, end] = scope_map.equal_range(current_scope);
    SymbolData symbol;
    for (auto s = start; s != end; s++)
    {
        symbol = find(symbol_name, s->second);
        if (symbol.name == symbol_name)
            return symbol;
    }    
    
    return {};
}

SymbolData SymbolTable::find(std::string &symbol_name) {
    return find(symbol_name, current_scope);
}

void SymbolTable::update(std::string &symbol_name, SymbolData symbol) {
    table.insert_or_assign(std::make_pair(symbol_name, current_scope), symbol);
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