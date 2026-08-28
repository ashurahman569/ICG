grammar CSubset;
import Lexer;

start : program ;

program
    : program unit  #ProgramUnit
    | unit          #UnitOnly
    ;

unit
    : var_declaration     #UnitVarDec
    | func_declaration    #FuncDec 
    | func_definition     #FuncDef 
    ;

func_declaration
    : type_specifier ID LPAREN parameter_list RPAREN SEMICOLON  #FuncDecWithParam
    | type_specifier ID LPAREN RPAREN SEMICOLON                 #FuncDecNoParam
    ;

func_definition
    : type_specifier ID LPAREN parameter_list RPAREN compound_statement    #FuncDefWithParam
    | type_specifier ID LPAREN RPAREN compound_statement                   #FuncDefNoParam
    ;

parameter_list
    : parameter_list COMMA type_specifier ID           #ParamListMultipleID
    | parameter_list COMMA type_specifier              #ParamListMultipleWithNoIDLast
    | type_specifier ID                                #ParamListSingleID
    | type_specifier                                   #ParamListNoID  
    ;

compound_statement
    : LCURL statements RCURL         #CodeBlock
    | LCURL RCURL                    #BlankBlock
    ;

var_declaration
    : type_specifier declaration_list SEMICOLON #TypeSpecifiedVarDeclarationList 
    ;    

type_specifier
    : INT      #TypeInt
    | FLOAT    #TypeFloat
    | VOID     #TypeVoid
    ;

declaration_list
    : declaration_list COMMA ID                               #MultipleIDDeclaration
    | declaration_list COMMA ID LTHIRD CONST_INT RTHIRD       #MutlipleIDDeclarationWithArray
    | ID                                                      #SingleIDDeclaration
    | ID LTHIRD CONST_INT RTHIRD                              #SignleIDArrayDeclaration
    ;

statements
    : statement              #SingleStatement
    | statements statement   #MultipleStatement
    ;

statement
    : var_declaration            #StatementVarDec
    | expression_statement       #SingleExpressionStatement
    | compound_statement         #CompoundStatement
    | FOR LPAREN expression_statement expression_statement expression RPAREN statement  #ForLoopStatement
    | IF LPAREN expression RPAREN statement ELSE statement    #IfElseStatement
    | IF LPAREN expression RPAREN statement   #IfStatement    
    | WHILE LPAREN expression RPAREN statement    #WhileLoopStatement
    | PRINTLN LPAREN ID RPAREN SEMICOLON     #PrintStatement
    | RETURN expression SEMICOLON            #ReturnExpressionStatement
    ;

expression_statement
    : SEMICOLON               #NoExpression
    | expression SEMICOLON    #ExpressionStatement
    ;

variable
    : ID                            #AnID
    | ID LTHIRD expression RTHIRD   #AnArrayIndex
    ;

expression
    : logic_expression    #LogicalExpression
    | variable ASSIGNOP logic_expression     #AssignExpression
    ;

logic_expression
    : rel_expression         #RelationalExpression
    | rel_expression LOGICOP rel_expression         #MultipleRelationalExpression
    ;

rel_expression
    : simple_expression         #SimpleExpression
    | simple_expression RELOP simple_expression     #SimpleCompareSimple
    ; 

simple_expression
    : term      #SimpleTerm
    | simple_expression ADDOP term                   #SimpleExpressionAddTerm
    ;

term
    : unary_expression              #UnaryExpression
    | term MULOP unary_expression   #TermMultipliedUnaryExpression
    ;

unary_expression
    : ADDOP unary_expression        #AddUnaryExpression
    | NOT unary_expression          #NotUnaryExpression
    | factor                        #SingleFactor
    ;

factor
    : variable                            #FactorVariable
    | ID LPAREN argument_list RPAREN      #IDOfArgList
    | LPAREN expression RPAREN            #BracketedExpression
    | CONST_INT                           #ConstInt
    | CONST_FLOAT                         #ConstFloat
    | variable INCOP                      #VariableIncrement
    | variable DECOP                      #VariableDecrement
    ;

argument_list
    : arguments      #NonEmptyArguments
    |                #BlankArgument
    ;

arguments
    : arguments COMMA logic_expression   #MultipleArguments
    | logic_expression                   #SingleArgumentLogic
    ;


