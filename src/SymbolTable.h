#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

enum SymbolType: int {
    VARIABLE,
    FUNCTION,
    PARAMETER,
    CLASS,
    OBJECT,
    CLOSURE
};

enum SymbolDataType: int {
    UNDEFINED = 0,
    NUMBER = -1,
    STRING = -2,
    BOOLEAN = -3,
    NIL = -4,
    ANY = -5 // Placeholder para otros tipos
};

/*
name - Identificacion
parent - Exclusivo de clases, indica de que clase hereda 
namespace - Si pertenece a una funcion o clase, el nombre se indica aqui
type - Tipo de simbolo
data_type - Tipo de dato. 
    Si es función o cerradura, es nil o el tipo de la variable a retornar.
    Si es clase, valor que lo identifica.
    Si es objeto, valor de la clase.
    Valores negativos para identificar datos primitivos.
    Valores positivos para clases definidas.
    El valor 0 indica que no está definido.
value - Valor contenido, exclusivo de variables.
    Vacio para funciones, clases, objetos y cerraduras.
scope - Ambito, 0 es global
size - Tamaño del símbolo
offset - ubicacion en memoria

*/
struct SymbolData {
    std::string name;
    std::string parent;
    SymbolType type;
    int data_type; 
    std::string value;
    int scope;
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

    unsigned int scopes;
    unsigned int current_scope;
    std::multimap<unsigned int, unsigned> scope_map; // to -> from
    std::unordered_map<std::pair<std::string, int>, SymbolData, pair_hash> table;
public:
    SymbolTable();
    ~SymbolTable();

    // Current Scope operations

    void insert(SymbolData &symbol);
    SymbolData find(std::string &symbol_name);
    void update(std::string &symbol_name, SymbolData symbol);

    // Selected Scope operations

    SymbolData find(std::string &symbol_name, unsigned int &scope);

    /*Crea un nuevo ambito y lo selecciona como el ambito actual*/
    int enter();

    /*Cierra el ambito actual y regresa al ambito anterior*/
    int exit();
};
