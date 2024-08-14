#include "antlr4-runtime.h"

#include "generated/CompiScriptBaseListener.h"
#include "generated/CompiScriptListener.h"
#include "generated/CompiScriptParser.h"
#include "generated/CompiScriptLexer.h"

#include "SymbolTable.h"

#include <vector>
#include <stack>

int tableIndex = 0;
std::stack<SymbolTable> tables;
SymbolTable currentTable = SymbolTable();

class CompiScriptSemanticChecker: public CompiScriptBaseListener
{
    SymbolData currentSymbol = {};

    void enterClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        auto ids = ctx->IDENTIFIER();

        if (currentTable.lookup(ids[0]->getText())) {
            std::cerr << "Error: Class " << ids[0]->getText() << " already declared" << std::endl;
            exit(1);
        }

        currentSymbol.name = ids[0]->getText();
        currentSymbol.type = SymbolType::CLASS;
        currentSymbol.data_type = "";

        currentTable.bind(currentSymbol);

        tables.push(currentTable);
        currentTable = SymbolTable();
        tableIndex++;
    }

    void exitClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        tables.pop();
        tableIndex--;
        currentTable = tables.top();

        currentSymbol.method_table_index = tableIndex + 1;
        currentTable.update(currentSymbol.name, currentSymbol);

        currentSymbol = SymbolData();
    }

    void enterFunction(CompiScriptParser::FunctionContext *ctx) override {
        auto id = ctx->IDENTIFIER();
        auto params = ctx->parameters();
        auto block = ctx->block();

        if (currentTable.lookup(id->getText())) {
            std::cerr << "Error: Function " << id->getText() << " already declared" << std::endl;
            exit(1);
        }

        currentSymbol.name = id->getText();
        currentSymbol.type = SymbolType::FUNCTION;
        currentSymbol.data_type = "";

        currentTable.bind(currentSymbol);

    }
};

int main () {
    return 0;
}