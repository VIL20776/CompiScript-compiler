#include <antlr4-runtime.h>

#include "generated/CompiScriptBaseListener.h"
#include "generated/CompiScriptListener.h"
#include "generated/CompiScriptParser.h"
#include "generated/CompiScriptLexer.h"

#include "SymbolTable.h"

#include <vector>
#include <stack>

class CompiScriptSemanticChecker: public CompiScriptBaseListener
{
    SymbolTable *table = new SymbolTable();
    SymbolData current_symbol;
    std::string named_scope = "local";
    std::stack<SymbolData> symbol_stack;

    public:
    void enterClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        std::string name = ctx->IDENTIFIER()[0]->getText();

        auto found_symbol = table->find(name);
        if (found_symbol.second)
            std::cerr << "Error: El nombre" << name << "ya ha sido usado\n";

        current_symbol = SymbolData();
        current_symbol.name = name;
        if (ctx->IDENTIFIER().size() > 1) {
            std::string scope = ctx->IDENTIFIER()[0]->getText();
            found_symbol = table->find(scope);
            if (!found_symbol.second)
                std::cerr << "Error: No existe la clase con el nombre" << scope << "\n";

            current_symbol.scope = scope;
        }

        current_symbol.type = SymbolType::CLASS;
        current_symbol.data_type = SymbolDataType::ANY;
        symbol_stack.push(current_symbol);
    }

    void exitClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        current_symbol = symbol_stack.top();
        table->insert(current_symbol);
        symbol_stack.pop();
    }

    void enterFunDecl(CompiScriptParser::FunDeclContext *ctx) override {
        current_symbol = SymbolData();
        current_symbol.type = SymbolType::FUNCTION;
    }

    void exitFunDecl(CompiScriptParser::FunDeclContext *ctx) override {
        table->insert(current_symbol);
    }

    void enterVarDecl(CompiScriptParser::VarDeclContext *ctx) override {
        current_symbol = SymbolData();
        current_symbol.type = SymbolType::VARIABLE;
    }

    void exitVarDecl(CompiScriptParser::VarDeclContext *ctx) override {
        table->insert(current_symbol);
    }

    void enterAssignment(CompiScriptParser::AssignmentContext *ctx) override {
        if (ctx->logic_or() != nullptr)
            return;
        
        auto id = ctx->IDENTIFIER()->getText();
        auto [symbol, found] = table->find(id, named_scope);
        if (!found)
            std::cerr << "Error: Symbolo no definido" << "\n";
    }

    void enterCall(CompiScriptParser::CallContext *ctx) override {
        if (ctx->funAnon() != nullptr)
            return;

        auto id = ctx->IDENTIFIER()[0]->getText();
        auto [symbol, found] = table->find(id, named_scope);
        if (!found)
            std::cerr << "Error: Symbolo no definido" << "\n";
    }

    void enterPrimary(CompiScriptParser::PrimaryContext *ctx) override {
        if (ctx->IDENTIFIER() != nullptr) {
            auto id = ctx->IDENTIFIER()->getText();
            auto prevToken = ctx->getStart();
            if (prevToken != nullptr && prevToken->getText() == "super") {
                printf("Super\n");
            }
        }
    }

};

int main () {
    auto input = antlr4::ANTLRInputStream("super.a;");
    CompiScriptLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    CompiScriptParser parser(&tokens);
    antlr4::tree::ParseTree *tree = parser.program();
    CompiScriptSemanticChecker checker;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&checker, tree);

    return 0;
}