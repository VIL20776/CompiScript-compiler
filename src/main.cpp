#include "antlr4-runtime.h"

#include "generated/CompiScriptBaseVisitor.h"
#include "generated/CompiScriptVisitor.h"
#include "generated/CompiScriptParser.h"
#include "generated/CompiScriptLexer.h"

#include "SymbolTable.h"

#include <vector>
#include <stack>
#include <any>

using std::string, std::vector, std::any, std::make_any, std::any_cast;

class CompiScriptSemanticChecker: public CompiScriptBaseVisitor
{
    SymbolTable table;
    SymbolData current_symbol;
    string current_label;
    vector<SymbolData> symbol_store;
    bool classDef = false;

    public:
    any visitDeclaration(CompiScriptParser::DeclarationContext *ctx) override {

    }

    any visitClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        std::string name = ctx->IDENTIFIER(0)->getText();

        auto symbol_found = table.find(name);
        if (symbol_found.second)
            std::cerr << "Error: El nombre" << name << "ya está declarado\n";

        SymbolData symbol = SymbolData();
        symbol.name = name;
        if (ctx->IDENTIFIER().size() > 1) {
            std::string parent = ctx->IDENTIFIER(1)->getText();
            symbol_found = table.find(parent);
            if (!symbol_found.second)
                std::cerr << "Error: No existe la clase con el nombre" << parent << "\n";

            symbol.label = parent;
        }

        symbol.type = SymbolType::CLASS;
        symbol.data_type = SymbolDataType::ANY;

        classDef = true;
        for (auto fun: ctx->function()) {
            SymbolData func_symbol = any_cast<SymbolData>(visitFunction(fun, &symbol));
        }
        classDef = false;

        // Code Generation
    }

    any visitFunDecl(CompiScriptParser::FunDeclContext *ctx) override {
        return visitFunction(ctx->function());
    }

    any visitVarDecl(CompiScriptParser::VarDeclContext *ctx) override {
        current_symbol = SymbolData();
        current_symbol.type = SymbolType::VARIABLE;
        if (!ctx->expression())
            current_symbol.data_type = SymbolDataType::UNDEFINED;
    }

    any visitReturnStmt(CompiScriptParser::ReturnStmtContext *ctx) override {
        return visitExpression(ctx->expression());
    }

    any visitBlock(CompiScriptParser::BlockContext *ctx) override {
        table.enter();
        for (auto decl: ctx->declaration()) {
            any result = visitDeclaration(decl);
        }
        table.exit();
    }

    any visitBlock(CompiScriptParser::BlockContext *ctx, const vector<SymbolData> &symbols) {
        table.enter(symbols);
        visitChildren(ctx);
        for (auto decl: ctx->declaration()) {
            auto return_ptr = decl->statement()->returnStmt();
            if (return_ptr) {
                SymbolDataType return_type = any_cast<SymbolDataType>(visitReturnStmt(return_ptr));
                return return_type;
            }
        }
        table.exit();

        return make_any<SymbolDataType>(SymbolDataType::NIL);
    }

    any visitAssignment(CompiScriptParser::AssignmentContext *ctx) override {
        if (ctx->logic_or() != nullptr)
            return visitLogic_or(ctx->logic_or());
        
        string label = "";
        if (ctx->call() != nullptr) {
            any call_value = visitCall(ctx->call());
            SymbolData object = std::any_cast<SymbolData>(call_value);
            label = object.name;
        }
        
        auto id = ctx->IDENTIFIER()->getText();
        auto [symbol, found] = table.find(id, label);
        if (!found)
            std::cerr << "Error: Symbolo no definido" << "\n";
        
        std::any assignment_value = visitAssignment(ctx->assignment());
    }

    any visitCall(CompiScriptParser::CallContext *ctx) override {
        if (ctx->funAnon() != nullptr)
            return visitFunAnon(ctx->funAnon());
        
        std::any primary_value = visitPrimary(ctx->primary());
        SymbolData symbol = std::any_cast<SymbolData>(primary_value);

        int arg_index = 0;
        int expr_index = 0;
        int id_index = 0;
        for (auto child: ctx->children) {

            if (child->getText() == "(") {
                // Check if symbol is function

                visitArguments(ctx->arguments(arg_index++));
                continue;
            }

            if (child->getText() == "[") {
                // Check if symbol is array

                visitExpression(ctx->expression(expr_index++));
                continue;
            }

            if (child->getText() == ".") {
                // Check if symbol is object prop

                std::string prop = ctx->IDENTIFIER(id_index++)->getText();
                continue;
            }
        }
    }

    any visitPrimary(CompiScriptParser::PrimaryContext *ctx) override {
        if (ctx->NUMBER() != nullptr) {
            SymbolData symbol = {
                .type = SymbolType::LITERAL,
                .data_type = SymbolDataType::NUMBER,
                .value = ctx->NUMBER()->getText()
            };

            return std::make_any<SymbolData>(symbol);
        }

        if (ctx->STRING() != nullptr) {
            SymbolData symbol = {
                .type = SymbolType::LITERAL,
                .data_type = SymbolDataType::STRING,
                .value = ctx->STRING()->getText()
            };

            return std::make_any<SymbolData>(symbol);
        }

        if (ctx->IDENTIFIER() != nullptr) {
            auto [symbol, found] = table.find(ctx->IDENTIFIER()->getText());
            if (!found) {
                symbol.name = ctx->IDENTIFIER()->getText();
            }

            return std::make_any<SymbolData>(symbol);
        }

        string value = ctx->getText();
        if (value == "true" || value == "false") {
            SymbolData symbol = {
                .type = SymbolType::LITERAL,
                .data_type = SymbolDataType::BOOLEAN,
                .value = value
            };

            return std::make_any<SymbolData>(symbol);
        }

        if (value == "nil") {
            SymbolData symbol = {
                .type = SymbolType::LITERAL,
                .data_type = SymbolDataType::NIL,
                .value = value
            };

            return std::make_any<SymbolData>(symbol);
        }

        // this super TODO
    }

    any visitFunction(CompiScriptParser::FunctionContext *ctx) override {    
        SymbolData new_symbol = {
            .name = ctx->IDENTIFIER()->getText(),
            .type = SymbolType::FUNCTION,
            .data_type = SymbolDataType::ANY
        };
        
        any params = visitParameters(ctx->parameters());
        vector<SymbolData> vec_params = any_cast<vector<SymbolData>>(params);

        for (auto param: vec_params) {
            new_symbol.arg_list.push_back(param.name);
        }
        
        new_symbol.data_type = any_cast<SymbolDataType>(visitBlock(ctx->block(), vec_params));

        return make_any<SymbolData>(new_symbol);
    }

    any visitFunction(CompiScriptParser::FunctionContext *ctx, SymbolData *symbol_class) {
        auto [symbol, found] = table.find(ctx->IDENTIFIER()->getText(), symbol_class->name);
        if (found) {
            std::cerr << "Error: No se puede redefinir una función.\n";
            return make_any<SymbolData>();
        }

        SymbolData new_symbol = {
            .name = ctx->IDENTIFIER()->getText(),
            .type = SymbolType::FUNCTION,
            .data_type = SymbolDataType::ANY
        };
        
        any params = visitParameters(ctx->parameters());
        vector<SymbolData> vec_params = any_cast<vector<SymbolData>>(params);

        SymbolData self = {
            .name = "this",
            .label = symbol_class->label,
            .type = SymbolType::VARIABLE,
            .data_type = SymbolDataType::OBJECT,
        };
        vec_params.insert(vec_params.begin(), self);
        

        for (auto param: vec_params)
            new_symbol.arg_list.push_back(param.name);
        
        new_symbol.data_type = any_cast<SymbolDataType>(visitBlock(ctx->block(), vec_params));

        return make_any<SymbolData>(new_symbol);
    }

    any visitParameters(CompiScriptParser::ParametersContext *ctx) override {
        vector<SymbolData> params = {};

        for (auto param: ctx->IDENTIFIER()) {
            SymbolData new_symbol = {
                .name = param->getText(),
                .label = "",
                .type = SymbolType::VARIABLE,
                .data_type = SymbolDataType::ANY
            };

            params.push_back(new_symbol);
        }

        return make_any<vector<SymbolData>>(params);
    }

    any visitArguments(CompiScriptParser::ArgumentsContext *ctx) override {
        int arg_count = ctx->expression().size();

        if (current_label != "") {
            int arg_expectected = table.find_range(current_label).size();
            if (arg_count != arg_expectected)
                std::cerr << "Error: La cantidad de argumentos recibidos no coincide con la esperada";
        }
    }

};

int main () {
    auto input = antlr4::ANTLRInputStream("super.my_id;");
    CompiScriptLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    CompiScriptParser parser(&tokens);
    CompiScriptSemanticChecker checker;
    checker.visitProgram(parser.program());
    return 0;
}