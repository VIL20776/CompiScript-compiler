#include "SymbolTable.h"

#include <utility>
#include <set>

SymbolTable::SymbolTable(SymbolTable *parent):
    parent(parent), table()
{
}

SymbolTable::~SymbolTable()
{
}

void SymbolTable::insert(const SymbolData &symbol) {
    auto key = std::make_pair(symbol.name, "local");
    table.emplace(key, symbol);
}

std::vector<SymbolData>
SymbolTable::find_range(const std::string &scope) {
    std::vector<SymbolData> symbols;
    for (auto &kv : table) 
    {
        auto &key = kv.first;
        if (key.second == scope)
            symbols.push_back(kv.second);
    }

    return symbols;
}

std::pair<SymbolData, bool>
SymbolTable::find(const std::string &symbol_name, const std::string &scope) {
    // Try finding in current scope
    auto key = std::make_pair(symbol_name, scope);
    if (table.contains(key))
        return {table.at(key), true};

    if (parent == nullptr)
        return {{}, false};

    // Get parent scope
    auto [symbol, found] = parent->find(symbol_name, scope);
    if (found)
        return {symbol, found};
    
}


bool SymbolTable::update(const std::string &symbol_name, const SymbolData &symbol, const std::string &scope) {

    auto [old_symbol, found] = find(symbol_name, scope);
    if (!found)
        return false;

    table.insert_or_assign({symbol_name, scope}, symbol);
    return true;
}
