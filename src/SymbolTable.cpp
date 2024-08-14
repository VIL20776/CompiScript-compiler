#include "SymbolTable.h"

SymbolTable::SymbolTable():
    table()
{
}

SymbolTable::~SymbolTable()
{
}

bool SymbolTable::bind(SymbolData symbol)
{
    if (table.contains(symbol.name))
        return false;

    table[symbol.name] = symbol;
    return true;
}

bool SymbolTable::lookup(std::string name)
{
    return table.contains(name);
}

bool SymbolTable::update(std::string name, SymbolData symbol)
{
    if (!table.contains(name))
        return false;

    table[name] = symbol;
    return true;
}