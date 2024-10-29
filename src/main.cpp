#include "antlr4-runtime.h"

#include "generated/CompiScriptBaseVisitor.h"
#include "generated/CompiScriptVisitor.h"
#include "generated/CompiScriptParser.h"
#include "generated/CompiScriptLexer.h"

#include "SymbolTable.h"

#include <fstream>
#include <vector>
#include <stack>
#include <any>

using std::string, std::vector, std::any, std::make_any, std::any_cast;

class CompiScriptSemanticChecker: public CompiScriptBaseVisitor
{
    SymbolTable table;
    string current_label;
    vector<SymbolData> symbol_store;
    bool classDef = false;

    public:
    any visitDeclaration(CompiScriptParser::DeclarationContext *ctx) override {
        return visitChildren(ctx);
    }

    any visitClassDecl(CompiScriptParser::ClassDeclContext *ctx) override {
        std::string name = ctx->IDENTIFIER(0)->getText();

        auto symbol_found = table.find(name);
        if (symbol_found.second)
            std::cerr << "Error: El nombre" << name << "ya está declarado\n";

        SymbolData new_symbol = SymbolData();
        new_symbol.name = name;
        if (ctx->IDENTIFIER().size() > 1) {
            std::string parent = ctx->IDENTIFIER(1)->getText();
            symbol_found = table.find(parent);
            if (!symbol_found.second)
                std::cerr << "Error: No existe la clase con el nombre" << parent << "\n";

            new_symbol.label = parent;
        }

        new_symbol.type = SymbolType::CLASS;
        new_symbol.data_type = SymbolDataType::ANY;

        classDef = true;
        for (auto fun: ctx->function()) {
            SymbolData func_symbol = any_cast<SymbolData>(visitFunction(fun, &new_symbol));
        }

        classDef = false;

        table.insert(new_symbol);

        // Code Generation
        return make_any<SymbolData>(new_symbol);
    }

    any visitFunDecl(CompiScriptParser::FunDeclContext *ctx) override {
        return visitFunction(ctx->function());
    }

    any visitVarDecl(CompiScriptParser::VarDeclContext *ctx) override {
        SymbolData new_symbol = SymbolData();
        new_symbol.type = SymbolType::VARIABLE;
        if (!ctx->expression()) {
            new_symbol.data_type = SymbolDataType::UNDEFINED;
            table.insert(new_symbol);
            return make_any<SymbolData>(new_symbol);
        }

        SymbolData result = any_cast<SymbolData>(visitExpression(ctx->expression()));
        switch (result.type)
        {
        case SymbolType::LITERAL:
        case SymbolType::VARIABLE:
        case SymbolType::ARRAY:
            new_symbol.data_type = result.data_type;
            new_symbol.value = result.value;
            break;
        case SymbolType::FUNCTION:
        case SymbolType::CLOUSURE:
            new_symbol.label = result.name;
            new_symbol.type = SymbolType::CLOUSURE;
            new_symbol.data_type = result.data_type;
            new_symbol.arg_list = result.arg_list;
            break;
        case SymbolType::OBJECT:
            new_symbol.label = result.name;
            new_symbol.type = result.type;
            new_symbol.data_type = result.data_type;
            new_symbol.prop_list = result.prop_list;
            new_symbol.value = result.value;
            break;
        default:
            break;
        }

        table.insert(new_symbol);
        return make_any<SymbolData>(new_symbol);
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

        return make_any<SymbolDataType>(SymbolDataType::NIL);
    }

    any visitBlock(CompiScriptParser::BlockContext *ctx, const vector<SymbolData> &symbols) {
        table.enter(symbols);
        visitChildren(ctx);

        any return_type = make_any<SymbolDataType>(SymbolDataType::NIL);
        for (auto decl: ctx->declaration()) {
            auto return_ptr = decl->statement()->returnStmt();
            if (return_ptr) {
                return_type = make_any<SymbolDataType>(SymbolDataType::ANY);
            }
        }
        table.exit();

        return return_type;
    }

    any visitAssignment(CompiScriptParser::AssignmentContext *ctx) override {
        if (ctx->logic_or() != nullptr)
            return visitLogic_or(ctx->logic_or());
        
        // Check if an object property is being called.
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

    any visitInstantiation(CompiScriptParser::InstantiationContext *ctx) override {
        string class_name = ctx->IDENTIFIER()->getText();
        auto [symbol, found] = table.find(class_name);

        if (!found) {
            std::cerr << "No existe una clase con este nombre.\n";
        }
        // Check if arguments have been passed
        
        int arg_count = 0;
        if (ctx->arguments())
            arg_count = any_cast<vector<SymbolData>>(ctx->arguments()).size();

        auto [symbol, found] = table.find("init", class_name);

        if (symbol.arg_list.size() != arg_count) {
            std::cerr << "La cantidad de argumentos no es coincide con la función constructor\n";
        }

        
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
        
        if (value == "this" || value == "super") {
            auto [symbol, found] = table.find("this");
            if (!found) 
                symbol.name = ctx->IDENTIFIER()->getText();
            


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
            .label = symbol_class->name,
            .type = SymbolType::FUNCTION,
            .data_type = SymbolDataType::ANY,
        };
        
        any params = visitParameters(ctx->parameters());
        vector<SymbolData> vec_params = any_cast<vector<SymbolData>>(params);

        SymbolData self = {
            .name = "this",
            .label = symbol_class->label,
            .type = SymbolType::OBJECT
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
        vector<SymbolData> symbols = {};
        for (auto arg: ctx->expression())
            symbols.push_back(any_cast<SymbolData>(visitExpression(arg)));
        
        return make_any<vector<SymbolData>>(symbols);
    }

};

int main (int argc, char** argv) {

    std::ifstream stream;
    if (argc < 2) 
        stream.open("../example/Ejemplo1.cspt");

    stream.open(argv[1]);

    auto input = antlr4::ANTLRInputStream(stream);
    CompiScriptLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    CompiScriptParser parser(&tokens);
    CompiScriptSemanticChecker checker;
    checker.visitProgram(parser.program());
    return 0;
}