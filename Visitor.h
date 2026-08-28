#pragma once
#include <fstream>
#include <string>
#include <any>

#include "CSubsetBaseVisitor.h"
#include "CSubsetParser.h"
#include "SymbolTable.cpp"

using namespace antlr4;
using namespace std;

class Visitor : public CSubsetBaseVisitor {
public:
    Visitor();
    ~Visitor();

    any visitStart(CSubsetParser::StartContext *ctx) override;

    any visitProgramUnit(CSubsetParser::ProgramUnitContext *ctx) override;
    any visitUnitOnly(CSubsetParser::UnitOnlyContext *ctx) override;

    any visitUnitVarDec(CSubsetParser::UnitVarDecContext *ctx) override;
    any visitFuncDec(CSubsetParser::FuncDecContext *ctx) override;
    any visitFuncDef(CSubsetParser::FuncDefContext *ctx) override;

    any visitFuncDecWithParam(CSubsetParser::FuncDecWithParamContext *ctx) override;
    any visitFuncDecNoParam(CSubsetParser::FuncDecNoParamContext *ctx) override;

    any visitFuncDefWithParam(CSubsetParser::FuncDefWithParamContext *ctx) override;
    any visitFuncDefNoParam(CSubsetParser::FuncDefNoParamContext *ctx) override;

    any visitParamListMultipleID(CSubsetParser::ParamListMultipleIDContext *ctx) override;
    any visitParamListMultipleWithNoIDLast(CSubsetParser::ParamListMultipleWithNoIDLastContext *ctx) override;
    any visitParamListSingleID(CSubsetParser::ParamListSingleIDContext *ctx) override;
    any visitParamListNoID(CSubsetParser::ParamListNoIDContext *ctx) override;

    any visitCodeBlock(CSubsetParser::CodeBlockContext *ctx) override;
    any visitBlankBlock(CSubsetParser::BlankBlockContext *ctx) override;

    any visitTypeSpecifiedVarDeclarationList(CSubsetParser::TypeSpecifiedVarDeclarationListContext *ctx) override;

    any visitTypeInt(CSubsetParser::TypeIntContext *ctx) override;
    any visitTypeFloat(CSubsetParser::TypeFloatContext *ctx) override;
    any visitTypeVoid(CSubsetParser::TypeVoidContext *ctx) override;

    any visitMultipleIDDeclaration(CSubsetParser::MultipleIDDeclarationContext *ctx) override;
    any visitMutlipleIDDeclarationWithArray(CSubsetParser::MutlipleIDDeclarationWithArrayContext *ctx) override;
    any visitSingleIDDeclaration(CSubsetParser::SingleIDDeclarationContext *ctx) override;
    any visitSignleIDArrayDeclaration(CSubsetParser::SignleIDArrayDeclarationContext *ctx) override;
    
    any visitSingleStatement(CSubsetParser::SingleStatementContext *ctx) override;
    any visitMultipleStatement(CSubsetParser::MultipleStatementContext *ctx) override;

    any visitStatementVarDec(CSubsetParser::StatementVarDecContext *ctx) override;
    any visitSingleExpressionStatement(CSubsetParser::SingleExpressionStatementContext *ctx) override;
    any visitCompoundStatement(CSubsetParser::CompoundStatementContext *ctx) override;
    any visitForLoopStatement(CSubsetParser::ForLoopStatementContext *ctx) override;
    any visitIfStatement(CSubsetParser::IfStatementContext *ctx) override;
    any visitIfElseStatement(CSubsetParser::IfElseStatementContext *ctx) override;
    any visitWhileLoopStatement(CSubsetParser::WhileLoopStatementContext *ctx) override;
    any visitPrintStatement(CSubsetParser::PrintStatementContext *ctx) override;
    any visitReturnExpressionStatement(CSubsetParser::ReturnExpressionStatementContext *ctx) override;

    any visitNoExpression(CSubsetParser::NoExpressionContext *ctx) override;
    any visitExpressionStatement(CSubsetParser::ExpressionStatementContext *ctx) override;

    any visitAnID(CSubsetParser::AnIDContext *ctx) override;
    any visitAnArrayIndex(CSubsetParser::AnArrayIndexContext *ctx) override;

    any visitLogicalExpression(CSubsetParser::LogicalExpressionContext *ctx) override;
    any visitAssignExpression(CSubsetParser::AssignExpressionContext *ctx) override;

    any visitRelationalExpression(CSubsetParser::RelationalExpressionContext *ctx) override;
    any visitMultipleRelationalExpression(CSubsetParser::MultipleRelationalExpressionContext *ctx) override;

    any visitSimpleExpression(CSubsetParser::SimpleExpressionContext *ctx) override;
    any visitSimpleCompareSimple(CSubsetParser::SimpleCompareSimpleContext *ctx) override;

    any visitSimpleTerm(CSubsetParser::SimpleTermContext *ctx) override;
    any visitSimpleExpressionAddTerm(CSubsetParser::SimpleExpressionAddTermContext *ctx) override;

    any visitUnaryExpression(CSubsetParser::UnaryExpressionContext *ctx) override;
    any visitTermMultipliedUnaryExpression(CSubsetParser::TermMultipliedUnaryExpressionContext *ctx) override;

    any visitAddUnaryExpression(CSubsetParser::AddUnaryExpressionContext *ctx) override;
    any visitNotUnaryExpression(CSubsetParser::NotUnaryExpressionContext *ctx) override;
    any visitSingleFactor(CSubsetParser::SingleFactorContext *ctx) override;

    any visitFactorVariable(CSubsetParser::FactorVariableContext *ctx) override;
    any visitIDOfArgList(CSubsetParser::IDOfArgListContext *ctx) override;
    any visitBracketedExpression(CSubsetParser::BracketedExpressionContext *ctx) override;
    any visitConstInt(CSubsetParser::ConstIntContext *ctx) override;
    any visitConstFloat(CSubsetParser::ConstFloatContext *ctx) override;
    any visitVariableIncrement(CSubsetParser::VariableIncrementContext *ctx) override;
    any visitVariableDecrement(CSubsetParser::VariableDecrementContext *ctx) override;

    any visitNonEmptyArguments(CSubsetParser::NonEmptyArgumentsContext *ctx) override;
    any visitBlankArgument(CSubsetParser::BlankArgumentContext *ctx) override;

    any visitMultipleArguments(CSubsetParser::MultipleArgumentsContext *ctx) override;
    any visitSingleArgumentLogic(CSubsetParser::SingleArgumentLogicContext *ctx) override;

private:
    ofstream asmFile;
    SymbolTable symbolTable;
    
    int labelCounter = 0;
    
    int nextLocalOffset = -4;
    int nextParamOffset = 8;
    vector<map<string, int>> offsetStack;
    
    string currentFuncName = "";
    string currentFuncReturnType = "";
    int currentFuncParamCount = 0;
    bool inFunction = false;
    bool isMain = false;
    int totalLocalSize = 0;
    
    vector<string> typeStack;
    string currentDeclType = "int";
    
    // Global variable tracking
    set<string> globalVars;
    bool inDataSegment = true;
    bool isGlobal(const string& name);
    string getMemRef(const string& name, int offset, const string& indexReg);

    string newLabel();
    void emit(const string& s);
    void emitLabel(const string& s);
    int lookupOffset(const string& name);
    
};
