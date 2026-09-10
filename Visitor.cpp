#include "Visitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <set>
#include <functional>
using namespace antlr4;
using namespace std;

string safeCastString(const any &result) {
    if (result.has_value()) {
        try { 
            return any_cast<string>(result); 
        } catch (const bad_any_cast &) { 
            return ""; 
        }
    }
    return "";
}

static ostream* outptr = nullptr; 

Visitor::Visitor() {
    asmFile.open("code.asm");
    outptr = &asmFile;
    labelCounter = 0;
    inDataSegment = true;
}

Visitor::~Visitor() {
    if (asmFile.is_open()) asmFile.close();
}

string Visitor::newLabel() { 
    return "L" + to_string(labelCounter++); 
}

void Visitor::emit(const string& s) {
    *outptr << "\t" << s << endl;
}

void Visitor::emitLabel(const string& s) { 
    *outptr << s << ":" << endl; 
}

int Visitor::lookupOffset(const string& name) {
    for (int i = (int)offsetStack.size() - 1; i >= 0; i--) {
        auto it = offsetStack[i].find(name);
        if (it != offsetStack[i].end()) return it->second;
    }
    return 0;
}

bool Visitor::isGlobal(const string& name) {
    if (isLocal(name)) return false;  
    return globalVars.find(name) != globalVars.end();
}

bool Visitor::isLocal(const string& name) {
    for (int i = (int)offsetStack.size() - 1; i >= 0; i--) {
        if (offsetStack[i].count(name)) return true;
    }
    return false;
}

string Visitor::getMemRef(const string& name, int offset, const string& indexReg) {
    if (isGlobal(name)) {
        if (indexReg.empty()) return "[" + name + "]";
        return "[" + name + "+" + indexReg + "]";
    } else {
        string offStr = (offset >= 0 ? "+" : "") + to_string(offset);
        if (indexReg.empty()) return "[EBP" + offStr + "]";
        return "[EBP" + offStr + "+" + indexReg + "]";
    }
}

// ---- start ----
any Visitor::visitStart(CSubsetParser::StartContext *ctx) {
    asmFile<<"format ELF executable 3"<<endl;
    asmFile<<"entry main"<<endl;

    visitChildren(ctx);

    asmFile << "segment readable writeable" << endl;
    asmFile << dataSection.str();
    asmFile << "segment readable executable" << endl;
    asmFile << codeSection.str();

    outptr = &asmFile;
    asmFile<<""<<endl;
    asmFile<<"print_number:"<<endl;
    emit("push eax");
    emit("push ebx");
    emit("push ecx");
    emit("push edx");
    emit("push esi");
    emit("push edi");
    emit("sub esp, 32");
    emit("test eax, eax");
    emit("jns .positive");
    emit("push eax");
    emit("sub esp, 1");
    emit("mov byte [esp], '-'");
    emit("mov eax, 4");
    emit("mov ebx, 1");
    emit("mov ecx, esp");
    emit("mov edx, 1");
    emit("int 0x80");
    emit("add esp, 1");
    emit("pop eax");
    emit("neg eax");
    emitLabel(".positive");
    emit("mov ebx, 10");
    emit("lea esi, [esp + 31]");
    emit("mov byte [esi], 10");
    emit("dec esi");
    emitLabel(".convert");
    emit("xor edx, edx");
    emit("div ebx");
    emit("add dl, '0'");
    emit("mov [esi], dl");
    emit("dec esi");
    emit("test eax, eax");
    emit("jnz .convert");
    emit("inc esi");
    emit("lea edx, [esp + 32]");
    emit("sub edx, esi");
    emit("mov eax, 4");
    emit("mov ebx, 1");
    emit("mov ecx, esi");
    emit("int 0x80");
    emit("add esp, 32");
    emit("pop edi");
    emit("pop esi");
    emit("pop edx");
    emit("pop ecx");
    emit("pop ebx");
    emit("pop eax");
    emit("ret");
    return nullptr;
}

// ---- program ----
any Visitor::visitProgramUnit(CSubsetParser::ProgramUnitContext *ctx) { 
    return visitChildren(ctx); 
}
any Visitor::visitUnitOnly(CSubsetParser::UnitOnlyContext *ctx) { 
    return visitChildren(ctx); 
}

// ---- unit ----
any Visitor::visitUnitVarDec(CSubsetParser::UnitVarDecContext *ctx) { 
    return visitChildren(ctx); 
}
any Visitor::visitFuncDec(CSubsetParser::FuncDecContext *ctx) { 
    return visitChildren(ctx);
}
any Visitor::visitFuncDef(CSubsetParser::FuncDefContext *ctx) { 
    return visitChildren(ctx); 
}

// ---- func_declaration ----
any Visitor::visitFuncDecWithParam(CSubsetParser::FuncDecWithParamContext *ctx) { 
    string name = ctx->ID()->getText();
    string type = safeCastString(visit(ctx->type_specifier()));
    symbolTable.InsertParent(name, "ID", type, true, true, false);
    return nullptr;
}
any Visitor::visitFuncDecNoParam(CSubsetParser::FuncDecNoParamContext *ctx) { 
    string name = ctx->ID()->getText();
    string type = safeCastString(visit(ctx->type_specifier()));
    symbolTable.InsertParent(name, "ID", type, true, true, false);
    return nullptr;
}

// ---- func_definition ----
any Visitor::visitFuncDefWithParam(CSubsetParser::FuncDefWithParamContext *ctx) {
    string funcName = ctx->ID()->getText();
    currentFuncName = funcName;
    isMain = (funcName == "main");
    nextLocalOffset = -4;
    nextParamOffset = 8;
    totalLocalSize = 0;
    currentFuncParamCount = 0;
    offsetStack.clear();
    offsetStack.push_back({});
    symbolTable.EnterScope(30);

    visit(ctx->parameter_list());

    stringstream body;
    ostream* old = outptr;
    outptr = &body;
    visit(ctx->compound_statement());
    outptr = &codeSection;  

    emitLabel(funcName);
    emit("PUSH EBP");
    emit("MOV EBP, ESP");
    if (totalLocalSize > 0) emit("SUB ESP, " + to_string(totalLocalSize));
    codeSection << body.str();
    
    emitLabel(funcName + "_exit");
    if (totalLocalSize > 0) emit("ADD ESP, " + to_string(totalLocalSize));
    emit("POP EBP");
    
    if (isMain) {
        emit("MOV EAX, 1");
        emit("XOR EBX, EBX");
        emit("INT 0x80");
        emit("RET");
    } else {
        emit("RET");
    }
    symbolTable.ExitScope();
    return nullptr;
}

any Visitor::visitFuncDefNoParam(CSubsetParser::FuncDefNoParamContext *ctx) {    
    string funcName = ctx->ID()->getText();
    currentFuncName = funcName;
    isMain = (funcName == "main");
    nextLocalOffset = -4;
    nextParamOffset = 8;
    totalLocalSize = 0;
    currentFuncParamCount = 0;
    offsetStack.clear();
    offsetStack.push_back({});
    
    symbolTable.EnterScope(30);
    
    stringstream body;
    ostream* old = outptr;
    outptr = &body;
    visit(ctx->compound_statement());
    outptr = &codeSection;  
    
    emitLabel(funcName);
    emit("PUSH EBP");
    emit("MOV EBP, ESP");
    if (totalLocalSize > 0) emit("SUB ESP, " + to_string(totalLocalSize));
    codeSection << body.str();
    
    emitLabel(funcName + "_exit");
    if (totalLocalSize > 0) emit("ADD ESP, " + to_string(totalLocalSize));
    emit("POP EBP");
    
    if (isMain) {
        emit("MOV EAX, 1");
        emit("XOR EBX, EBX");
        emit("INT 0x80");
        emit("RET");
    } else {
        emit("RET 0");
    }
    
    symbolTable.ExitScope();
    return nullptr;
}

// ---- parameter_list ----
any Visitor::visitParamListMultipleID(CSubsetParser::ParamListMultipleIDContext *ctx) { 
    visit(ctx->parameter_list()); // Process left side first
    string type = safeCastString(visit(ctx->type_specifier()));
    string name = ctx->ID()->getText();
    symbolTable.Insert(name, "ID", type, false, true, false);
    offsetStack.back()[name] = nextParamOffset;
    nextParamOffset += 4;
    currentFuncParamCount++;
    return nullptr;
}
any Visitor::visitParamListMultipleWithNoIDLast(CSubsetParser::ParamListMultipleWithNoIDLastContext *ctx) { 
    visit(ctx->parameter_list());
    nextParamOffset += 4; // Unnamed still uses stack
    currentFuncParamCount++;
    return nullptr;
}

any Visitor::visitParamListSingleID(CSubsetParser::ParamListSingleIDContext *ctx) {
    string type = safeCastString(visit(ctx->type_specifier()));
    string name = ctx->ID()->getText();
    symbolTable.Insert(name, "ID", type, false, true, false);
    offsetStack.back()[name] = nextParamOffset;
    nextParamOffset += 4;
    currentFuncParamCount++;
    return nullptr;
} 

any Visitor::visitParamListNoID(CSubsetParser::ParamListNoIDContext *ctx) {
    return safeCastString(visit(ctx->type_specifier()));
}

// ---- compound_statement ----
any Visitor::visitCodeBlock(CSubsetParser::CodeBlockContext *ctx) {
    symbolTable.EnterScope(30);
    offsetStack.push_back({});
    if (ctx->statements()) visit(ctx->statements());
    symbolTable.ExitScope();
    offsetStack.pop_back();
    return nullptr;
}

any Visitor::visitBlankBlock(CSubsetParser::BlankBlockContext *ctx) { 
    return nullptr; 
}

// ---- var_declaration ----
any Visitor::visitTypeSpecifiedVarDeclarationList(CSubsetParser::TypeSpecifiedVarDeclarationListContext *ctx) {
    currentDeclType = safeCastString(visit(ctx->type_specifier()));
    visit(ctx->declaration_list());
    return nullptr;
}

// ---- type_specifier ----
any Visitor::visitTypeInt(CSubsetParser::TypeIntContext *ctx) { 
    return string("int"); 
}
any Visitor::visitTypeFloat(CSubsetParser::TypeFloatContext *ctx) { 
    return string("float"); 
}
any Visitor::visitTypeVoid(CSubsetParser::TypeVoidContext *ctx) { 
    return string("void"); 
}

// ---- declaration_list ----
any Visitor::visitMultipleIDDeclaration(CSubsetParser::MultipleIDDeclarationContext *ctx) {
    visit(ctx->declaration_list());
    string name = ctx->ID()->getText();
    symbolTable.Insert(name, "ID", currentDeclType, false, true, false);
    
    if (symbolTable.currentScope->getScopeID() == "1") {
        globalVars.insert(name);
        dataSection << "\t" << name << " dd 1 dup (0)" << endl;
    } else {
        offsetStack.back()[name] = nextLocalOffset;
        nextLocalOffset -= 4;
        totalLocalSize += 4;
    }
    return nullptr;
}
any Visitor::visitMutlipleIDDeclarationWithArray(CSubsetParser::MutlipleIDDeclarationWithArrayContext *ctx) { 
    visit(ctx->declaration_list());
    string name = ctx->ID()->getText();
    int size = stoi(ctx->CONST_INT()->getText());
    symbolTable.Insert(name, "ID", currentDeclType, false, true, true);
    arraySizes[name] = size;
    
    if (symbolTable.currentScope->getScopeID() == "1") {
        globalVars.insert(name);
        dataSection << "\t" << name << " dd " << size << " dup (0)" << endl;
    } else {
        int bytes = size * 4;
        offsetStack.back()[name] = nextLocalOffset - bytes + 4;
        nextLocalOffset -= bytes;
        totalLocalSize += bytes;
    }
    return nullptr;
}

any Visitor::visitSingleIDDeclaration(CSubsetParser::SingleIDDeclarationContext *ctx) {
    string name = ctx->ID()->getText();
    symbolTable.Insert(name, "ID", currentDeclType, false, true, false);
    
    if (symbolTable.currentScope->getScopeID() == "1") {
        globalVars.insert(name);
        dataSection << "\t" << name << " dd 1 dup (0)" << endl;
    } else {
        offsetStack.back()[name] = nextLocalOffset;
        nextLocalOffset -= 4;
        totalLocalSize += 4;
    }
    return nullptr;
}

any Visitor::visitSignleIDArrayDeclaration(CSubsetParser::SignleIDArrayDeclarationContext *ctx) {
    string name = ctx->ID()->getText();
    int size = stoi(ctx->CONST_INT()->getText());
    symbolTable.Insert(name, "ID", currentDeclType, false, true, true);
    arraySizes[name] = size;
    
    if (symbolTable.currentScope->getScopeID() == "1") {
        globalVars.insert(name);
        dataSection << "\t" << name << " dd " << size << " dup (0)" << endl;
    } else {
        int bytes = size * 4;
        offsetStack.back()[name] = nextLocalOffset - bytes + 4;
        nextLocalOffset -= bytes;
        totalLocalSize += bytes;
    }
    return nullptr;
}

// ---- statements ----
any Visitor::visitSingleStatement(CSubsetParser::SingleStatementContext *ctx) { 
    return visitChildren(ctx); 
}
any Visitor::visitMultipleStatement(CSubsetParser::MultipleStatementContext *ctx) { 
    return visitChildren(ctx); 
}

// ---- statement ----
any Visitor::visitStatementVarDec(CSubsetParser::StatementVarDecContext *ctx) { 
    return visitChildren(ctx); 
}
any Visitor::visitSingleExpressionStatement(CSubsetParser::SingleExpressionStatementContext *ctx) { 
    return visitChildren(ctx); 
}
any Visitor::visitCompoundStatement(CSubsetParser::CompoundStatementContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitForLoopStatement(CSubsetParser::ForLoopStatementContext *ctx) {
    string labelStart = newLabel();
    string labelStep = newLabel();
    string labelEnd = newLabel();
    
    visit(ctx->expression_statement(0)); // Init
    
    emitLabel(labelStart);
    auto condStmt = dynamic_cast<CSubsetParser::ExpressionStatementContext*>(ctx->expression_statement(1));
    if (condStmt && condStmt->expression()) { // check if condition exists
        visit(condStmt->expression());
        emit("TEST EAX, EAX");
        emit("JE " + labelEnd);
    }
    
    visit(ctx->statement());
    
    emitLabel(labelStep);
    visit(ctx->expression());
    
    emit("JMP " + labelStart);
    emitLabel(labelEnd);
    return nullptr;
}
any Visitor::visitIfStatement(CSubsetParser::IfStatementContext *ctx) { 
    string labelEnd = newLabel();
    visit(ctx->expression());
    emit("TEST EAX, EAX");
    emit("JE " + labelEnd);
    visit(ctx->statement());
    emitLabel(labelEnd);
    return nullptr;
}
any Visitor::visitIfElseStatement(CSubsetParser::IfElseStatementContext *ctx) { 
    string labelElse = newLabel();
    string labelEnd = newLabel();
    visit(ctx->expression());
    emit("TEST EAX, EAX");
    emit("JE " + labelElse);
    visit(ctx->statement(0));
    emit("JMP " + labelEnd);
    emitLabel(labelElse);
    visit(ctx->statement(1));
    emitLabel(labelEnd);
    return nullptr;
}
any Visitor::visitWhileLoopStatement(CSubsetParser::WhileLoopStatementContext *ctx) { 
    string labelStart = newLabel();
    string labelEnd = newLabel();
    emitLabel(labelStart);
    visit(ctx->expression());
    emit("TEST EAX, EAX");
    emit("JE " + labelEnd);
    visit(ctx->statement());
    emit("JMP " + labelStart);
    emitLabel(labelEnd);
    return nullptr;
}

any Visitor::visitPrintStatement(CSubsetParser::PrintStatementContext *ctx) {
    string name = ctx->ID()->getText();
    int offset = lookupOffset(name);
    emit("MOV EAX, " + getMemRef(name, offset, ""));
    emit("CALL print_number");
    return nullptr;
}

any Visitor::visitReturnExpressionStatement(CSubsetParser::ReturnExpressionStatementContext *ctx) {
    visit(ctx->expression());
    emit("JMP " + currentFuncName + "_exit");
    return nullptr;
}

// ---- expression_statement ----
any Visitor::visitNoExpression(CSubsetParser::NoExpressionContext *ctx) { return nullptr; }
any Visitor::visitExpressionStatement(CSubsetParser::ExpressionStatementContext *ctx) {
    visit(ctx->expression());
    return nullptr;
}

// ---- variable ----
any Visitor::visitAnID(CSubsetParser::AnIDContext *ctx) {
    string name = ctx->ID()->getText();
    int offset = lookupOffset(name);
    emit("MOV EAX, " + getMemRef(name, offset, ""));
    return nullptr;
}

any Visitor::visitAnArrayIndex(CSubsetParser::AnArrayIndexContext *ctx) {
    string name = ctx->ID()->getText();
    int offset = lookupOffset(name);
    visit(ctx->expression()); 
    emit("IMUL EAX, 4"); // Multiply index by 4
    emit("MOV EBX, EAX");
    
    if (isGlobal(name)) {
        emit("MOV EAX, [" + name + " + EBX]");
    } else {
        emit("MOV EAX, [EBP + " + to_string(offset) + " + EBX]");
    }
    return nullptr;
}

// ---- expression ----
any Visitor::visitLogicalExpression(CSubsetParser::LogicalExpressionContext *ctx) { return visitChildren(ctx); }

any Visitor::visitAssignExpression(CSubsetParser::AssignExpressionContext *ctx) {
    visit(ctx->logic_expression());
    emit("PUSH EAX");
    
    auto varCtx = ctx->variable();
    if (auto idctx = dynamic_cast<CSubsetParser::AnIDContext*>(varCtx)) {
        string name = idctx->ID()->getText();
        int offset = lookupOffset(name);
        emit("POP EBX");
        emit("MOV " + getMemRef(name, offset, "") + ", EBX");
        emit("MOV EAX, EBX");
    } else if (auto arrctx = dynamic_cast<CSubsetParser::AnArrayIndexContext*>(varCtx)) {
        string name = arrctx->ID()->getText();
        int offset = lookupOffset(name);
        
        visit(arrctx->expression());
        emit("IMUL EAX, 4");
        emit("MOV EBX, EAX");
        emit("POP EAX");
        
        if (isGlobal(name)) {
            emit("MOV [" + name + " + EBX], EAX");
        } else {
            emit("MOV [EBP + " + to_string(offset) + " + EBX], EAX");
        }
    }
    return nullptr;
}

// ---- logic_expression ----
any Visitor::visitRelationalExpression(CSubsetParser::RelationalExpressionContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitMultipleRelationalExpression(CSubsetParser::MultipleRelationalExpressionContext *ctx) {
    string op = ctx->LOGICOP()->getText();
    string labelFalse = newLabel();
    string labelEnd = newLabel();

    if (op == "&&") {
        visit(ctx->rel_expression(0)); // Left first
        emit("TEST EAX, EAX");
        emit("JE " + labelFalse);
        visit(ctx->rel_expression(1));
        emit("TEST EAX, EAX");
        emit("JE " + labelFalse);
        emit("MOV EAX, 1");
        emit("JMP " + labelEnd);
        emitLabel(labelFalse);
        emit("MOV EAX, 0");
        emitLabel(labelEnd);
    } else { //or
        string labelTrue = newLabel();
        visit(ctx->rel_expression(0));
        emit("TEST EAX, EAX");
        emit("JNE " + labelTrue);
        visit(ctx->rel_expression(1));
        emit("TEST EAX, EAX");
        emit("JNE " + labelTrue);
        emit("MOV EAX, 0");
        emit("JMP " + labelEnd);
        emitLabel(labelTrue);
        emit("MOV EAX, 1");
        emitLabel(labelEnd);
    }
    return nullptr;
}

// ---- rel_expression ----
any Visitor::visitSimpleExpression(CSubsetParser::SimpleExpressionContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitSimpleCompareSimple(CSubsetParser::SimpleCompareSimpleContext *ctx) {
    string op = ctx->RELOP()->getText();
    visit(ctx->simple_expression(1)); // Right first
    emit("PUSH EAX");
    visit(ctx->simple_expression(0)); 
    emit("POP EBX");
    emit("CMP EAX, EBX");

    string labelTrue = newLabel();
    string labelEnd = newLabel();
    string jumpInstr;
    if (op == "<") jumpInstr = "JL";
    else if (op == "<=") jumpInstr = "JLE";
    else if (op == ">") jumpInstr = "JG";
    else if (op == ">=") jumpInstr = "JGE";
    else if (op == "==") jumpInstr = "JE";
    else if (op == "!=") jumpInstr = "JNE";
    
    emit(jumpInstr + " " + labelTrue);
    emit("MOV EAX, 0");
    emit("JMP " + labelEnd);
    emitLabel(labelTrue);
    emit("MOV EAX, 1");
    emitLabel(labelEnd);
    return nullptr;
}

// ---- simple_expression ----
any Visitor::visitSimpleTerm(CSubsetParser::SimpleTermContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitSimpleExpressionAddTerm(CSubsetParser::SimpleExpressionAddTermContext *ctx) {
    string op = ctx->ADDOP()->getText();
    visit(ctx->term()); // Right first for binary operation (a = b+ b++;)
    emit("PUSH EAX");
    visit(ctx->simple_expression());
    emit("POP EBX");
    
    if (op == "+") emit("ADD EAX, EBX");
    else emit("SUB EAX, EBX");
    return nullptr;
}

// ---- term ----
any Visitor::visitUnaryExpression(CSubsetParser::UnaryExpressionContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitTermMultipliedUnaryExpression(CSubsetParser::TermMultipliedUnaryExpressionContext *ctx) {
    string op = ctx->MULOP()->getText();
    visit(ctx->unary_expression());
    emit("PUSH EAX");
    visit(ctx->term());
    emit("POP EBX");
    
    if (op == "*") {
        emit("IMUL EAX, EBX");
    } else if (op == "/") {
        emit("CDQ");
        emit("IDIV EBX");
    } else if (op == "%") {
        emit("CDQ");
        emit("IDIV EBX");
        emit("MOV EAX, EDX");
    }
    return nullptr;
}

// ---- unary_expression ----
any Visitor::visitAddUnaryExpression(CSubsetParser::AddUnaryExpressionContext *ctx) {
    string op = ctx->ADDOP()->getText();
    visit(ctx->unary_expression());
    if (op == "-") emit("NEG EAX");
    return nullptr;
}

any Visitor::visitNotUnaryExpression(CSubsetParser::NotUnaryExpressionContext *ctx) {
    visit(ctx->unary_expression());
    string labelTrue = newLabel();
    string labelEnd = newLabel();
    emit("TEST EAX, EAX");
    emit("JNE " + labelTrue);
    emit("MOV EAX, 1");
    emit("JMP " + labelEnd);
    emitLabel(labelTrue);
    emit("MOV EAX, 0");
    emitLabel(labelEnd);
    return nullptr;
}

any Visitor::visitSingleFactor(CSubsetParser::SingleFactorContext *ctx) { 
    return visitChildren(ctx); 
}

// ---- factor ----
any Visitor::visitFactorVariable(CSubsetParser::FactorVariableContext *ctx) { 
    return visitChildren(ctx); 
}

any Visitor::visitIDOfArgList(CSubsetParser::IDOfArgListContext *ctx) {
    int savedArgPushCount = currentArgPushCount; //now supports nested function calls
    currentArgPushCount = 0;
    visit(ctx->argument_list());
    string funcName = ctx->ID()->getText();
    emit("CALL " + funcName);
    
    if (currentArgPushCount > 0) {
        emit("ADD ESP, " + to_string(currentArgPushCount * 4));
    }
    currentArgPushCount = savedArgPushCount;
    return nullptr;
}

any Visitor::visitBracketedExpression(CSubsetParser::BracketedExpressionContext *ctx) {
    visit(ctx->expression());
    return nullptr;
}

any Visitor::visitConstInt(CSubsetParser::ConstIntContext *ctx) {
    emit("MOV EAX, " + ctx->CONST_INT()->getText());
    return nullptr;
}

any Visitor::visitConstFloat(CSubsetParser::ConstFloatContext *ctx) { 
    return string("float_const");
}

any Visitor::visitVariableIncrement(CSubsetParser::VariableIncrementContext *ctx) {
    auto varCtx = ctx->variable();
    if (auto idctx = dynamic_cast<CSubsetParser::AnIDContext*>(varCtx)) {
        string name = idctx->ID()->getText();
        int offset = lookupOffset(name);
        string memRef = getMemRef(name, offset, "");
        emit("MOV EAX, " + memRef);
        emit("MOV EBX, EAX");
        emit("ADD EBX, 1");
        emit("MOV " + memRef + ", EBX");
    } else if (auto arrctx = dynamic_cast<CSubsetParser::AnArrayIndexContext*>(varCtx)) {
        string name = arrctx->ID()->getText();
        int offset = lookupOffset(name);
        visit(arrctx->expression());
        emit("IMUL EAX, 4");
        emit("MOV EBX, EAX");
        if (isGlobal(name)) {
            emit("MOV EAX, [" + name + " + EBX]");
            emit("MOV EBX, EAX");
            emit("ADD EBX, 1");
            emit("MOV [" + name + " + EBX], EBX");
        } else {
            emit("MOV EAX, [EBP + " + to_string(offset) + " + EBX]");
            emit("MOV EBX, EAX");
            emit("ADD EBX, 1");
            emit("MOV [EBP + " + to_string(offset) + " + EBX], EBX");
        }
    }
    return nullptr;
}

any Visitor::visitVariableDecrement(CSubsetParser::VariableDecrementContext *ctx) {
    auto varCtx = ctx->variable();
    if (auto idctx = dynamic_cast<CSubsetParser::AnIDContext*>(varCtx)) {
        string name = idctx->ID()->getText();
        int offset = lookupOffset(name);
        string memRef = getMemRef(name, offset, "");
        emit("MOV EAX, " + memRef);
        emit("MOV EBX, EAX");
        emit("SUB EBX, 1");
        emit("MOV " + memRef + ", EBX");
    } else if (auto arrctx = dynamic_cast<CSubsetParser::AnArrayIndexContext*>(varCtx)) {
        string name = arrctx->ID()->getText();
        int offset = lookupOffset(name);
        visit(arrctx->expression());
        emit("IMUL EAX, 4");
        emit("MOV EBX, EAX");
        if (isGlobal(name)) {
            emit("MOV EAX, [" + name + " + EBX]");
            emit("MOV EBX, EAX");
            emit("SUB EBX, 1");
            emit("MOV [" + name + " + EBX], EBX");
        } else {
            emit("MOV EAX, [EBP + " + to_string(offset) + " + EBX]");
            emit("MOV EBX, EAX");
            emit("SUB EBX, 1");
            emit("MOV [EBP + " + to_string(offset) + " + EBX], EBX");
        }
    }
    return nullptr;
}

// ---- argument_list ----
any Visitor::visitNonEmptyArguments(CSubsetParser::NonEmptyArgumentsContext *ctx) { 
    return visit(ctx->arguments());
}
any Visitor::visitBlankArgument(CSubsetParser::BlankArgumentContext *ctx) { 
    return nullptr; 
}

// ---- arguments ----
any Visitor::visitMultipleArguments(CSubsetParser::MultipleArgumentsContext *ctx) { 
    visit(ctx->logic_expression());
    emit("PUSH EAX");
    currentArgPushCount++;
    visit(ctx->arguments());
    return nullptr;
}
any Visitor::visitSingleArgumentLogic(CSubsetParser::SingleArgumentLogicContext *ctx) { 
    visit(ctx->logic_expression());
    emit("PUSH EAX");
    currentArgPushCount++;
    return nullptr;
}