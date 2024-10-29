#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

enum class SymbolType: int {
    LITERAL,
    VARIABLE,
    FUNCTION,
    CLASS,
    OBJECT,
    CLOUSURE,
    ARRAY
};

enum class SymbolDataType: int {
    UNDEFINED,
    NUMBER,
    STRING,
    BOOLEAN,
    NIL,
    ANY 
};

/*
name - Identificacion
label - Etiqueta identificadora:
    Si es variable de tipo de dato OBJECT, indica la clase de la que es instancia.
    Si es funcion, indica a cual clase pertenece.
    Si es clase, indica de que clase hereda
type - Tipo de simbolo
data_type - Tipo de dato. 
    Si es función o cerradura, es nil o any.
    Si es clase u objeto, es any.
value - Valor contenido en la variable.
arg_list - Lista de argumentos para una funcion o cerradura.
prop_list - Lista de propiedades de una clase u objeto.
size - Tamaño del símbolo
offset - ubicacion en memoria

*/
struct SymbolData {
    std::string name;
    std::string label;
    SymbolType type;
    SymbolDataType data_type; 
    std::any value;
    std::vector<std::string> arg_list;
    std::vector<std::string> prop_list;
    int size;
    int offset;
};

class SymbolTable
{
private:

    struct pair_hash {
        template <class T1, class T2>
        int operator() (const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };

    using Table = 
    std::unordered_map<
        std::pair<std::string, std::string>, 
        SymbolData, 
        pair_hash
    >;

    Table current_scope;
    std::vector<Table> scopes;
    
public:
    SymbolTable();
    ~SymbolTable();

    void insert(const SymbolData &symbol);

    /*Find symbols matching symbol label*/
    std::vector<SymbolData> find_range(const std::string &label);

    std::pair<SymbolData, bool> find(const std::string &symbol_name, const std::string &label = "");

    bool update(const std::string &symbol_name, const SymbolData &symbol, const std::string &label = "");

    void enter(const std::vector<SymbolData> &initial_symbols = {});
    void exit();

};
