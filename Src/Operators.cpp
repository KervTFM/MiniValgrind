#include "Operators.h"
// print table of variables after run each operator in program
#define PRINT_VAR_TABLE false


std::string indentation(unsigned indent) {
    std::string res = "";
    for (unsigned i = 0; i < indent; i++)
        res += "  ";
    return res;
}

// Block Operator
Block::Block(std::list<Operator*> newOps) {
    ops = newOps;
}
size_t Block::size() {
    return ops.size();
}
void Block::print(unsigned indent) {
    std::cout << indentation(indent) << "{" << std::endl;
    for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); i++) {
        (*i)->print(indent + 1);
    }
    std::cout << indentation(indent) << "}" << std::endl;
}
Block::~Block() {
    for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); i++) {
        delete *i;
    }
    clearVarTable();
}
void Block::run(Block* parentBlock) {
    this->parentBlock = parentBlock;
    // run each operator, and output exception if there is one
    for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end() && !isReturned(); i++) {
        try {
            (*i)->run(this);
        }
        catch (const std::exception & ex) {
            std::cerr << "!!! ERROR !!!  " << ex.what() << std::endl;
            std::string skipLine;
            std::getline(std::cin, skipLine);
        }
        if (PRINT_VAR_TABLE) {
            printVarTable();
            std::cout << std::endl;
        }
    }
}
Var* Block::findVar(const std::string& id) {
    Block* currBlock = this;
    std::map<std::string, Var*>::iterator result = vars.end();
    // search var with given id in this block, then in parent block, ...
    while (result == vars.end() && currBlock != nullptr) {
        if (currBlock->vars.find(id) != currBlock->vars.end())
            result = currBlock->vars.find(id);
        else
            currBlock = currBlock->parentBlock;
    }
    if (result != vars.end())
        return result->second;
    else
        return nullptr;
}
void Block::addVar(const std::string& id, Var* newVar) {
    if (findVar(id) == nullptr) {
        vars.insert(std::pair<std::string, Var*>(id, newVar));
    }
    else
        throw UndefinedVarException("VarExpression with the same ID already exists");
}
void Block::printVarTable() const {
    if (parentBlock) {
        std::cout << "**** Variables of nested block ****" << std::endl;
        parentBlock->printVarTable();
    }
    else
        std::cout << "**** Variables ****" << std::endl;
    for (std::map<std::string, Var*>::const_iterator i = vars.begin(); i != vars.end(); i++) {
        switch ((*i).second->getType()) {
        case T_INT:
            std::cout << "int ";
            break;
        case T_PTR:
            std::cout << "ptr ";
            break;
        case T_ARR:
            std::cout << "arr ";
            break;
        }
        std::cout << (*i).first << " = " << *((*i).second) << std::endl;
    }

    if (parentBlock)
        std::cout << "***********************************" << std::endl;
    else
        std::cout << "*******************" << std::endl;
}
void Block::clearVarTable() {
    for (auto iter : vars) {
        delete iter.second;
    }
    vars.clear();
}
void Block::changeReturnValue(Var resultVal) {
    // to do: stop running parentBlock !!!
    Var* blockResult = findVar("#RESULT");
    if (blockResult != nullptr)
        *blockResult = resultVal;
    else
        throw UndefinedVarException("Block has no #RESULT for return");
}
bool Block::isReturned() {
    Var* blockResult = findVar("#RESULT");
    if (blockResult != nullptr) {
        if (blockResult->isVarInit())
            return true;
        else
            return false;
    }
    else
        throw UndefinedVarException("Block has no #RESULT for return");
}


// Expression Operator
ExprOperator::ExprOperator(Expression* expr) : expr(expr) {}
void ExprOperator::print(unsigned indent) {
    std::cout << indentation(indent);
    expr->print();
    std::cout << ";" << std::endl;
}
ExprOperator::~ExprOperator() { delete expr; }
void ExprOperator::run(Block* parentBlock) {
    expr->eval(parentBlock);
    // do nothing
}


// If Operator
IfOperator::IfOperator(Expression* cond, Block* thenBlock, Block* elseBlock) :
    cond(cond), thenBlock(thenBlock), elseBlock(elseBlock) {}
void IfOperator::print(unsigned indent) {
    std::cout << indentation(indent);
    std::cout << "if ";
    cond->print();
    std::cout << std::endl;
    thenBlock->print(indent);
    if (elseBlock != nullptr) {
        std::cout << indentation(indent) << "else" << std::endl;
        elseBlock->print(indent);
    }
}
IfOperator::~IfOperator() { delete cond; }
void IfOperator::run(Block* parentBlock) {
    int condResult = cond->eval(parentBlock).getIntVal();
    if (condResult == 1) {
        thenBlock->run(parentBlock);
        thenBlock->clearVarTable();
    }
    else {
        if (elseBlock) {
            elseBlock->run(parentBlock);
            elseBlock->clearVarTable();
        }
    }
}


// While Operator
WhileOperator::WhileOperator(Expression* cond, Block* body) :
    cond(cond), body(body) {}
void WhileOperator::print(unsigned indent) {
    std::cout << indentation(indent) << "while ";
    cond->print();
    std::cout << std::endl;
    body->print(indent);
}
WhileOperator::~WhileOperator() { delete cond; }
void WhileOperator::run(Block* parentBlock) {
    int condResult = cond->eval(parentBlock).getIntVal();
    while (condResult == 1) {
        body->run(parentBlock);
        body->clearVarTable();
        condResult = cond->eval(parentBlock).getIntVal();
    }
}


// For operator
ForOperator::ForOperator(Operator *initOp, Expression *cond, Operator *stepOp, Block *body) :
        initOp(initOp), cond(cond), stepOp(stepOp), body(body) {}
ForOperator::~ForOperator() {
    delete initOp;
    delete cond;
    delete stepOp;
}
void ForOperator::print(unsigned int indent) {
    std::cout << indentation(indent) << "for ";
    std::cout << "(";
    initOp->print();
    std::cout << "; ";
    cond->print();
    std::cout << "; ";
    stepOp->print();
    std::cout << ")";
    std::cout << std::endl;
    body->print(indent);
}
void ForOperator::run(Block *parentBlock) {
    Block* forBlock = new Block(std::list<Operator*>(1, initOp));
    //initOp->run(parentBlock);
    forBlock->run(parentBlock);
    int condResult = cond->eval(forBlock).getIntVal();
    while (condResult == 1) {
        body->run(forBlock);
        body->clearVarTable();
        stepOp->run(forBlock);
        condResult = cond->eval(forBlock).getIntVal();
    }
    forBlock->clearVarTable();
}



// Assign Operator
AssignOperator::AssignOperator(const std::string& ID, Expression* value, bool isDereferecing) :
    ID(ID), value(value), isDereferecing(isDereferecing) {
    index = nullptr;
}
AssignOperator::AssignOperator(const std::string& ID, Expression* value, Expression* index) :
    ID(ID), value(value), index(index) {
    isDereferecing = false;
}
void AssignOperator::print(unsigned indent) {
    std::cout << indentation(indent) << (isDereferecing ? "*" : "") << ID << " = ";
    value->print();
    std::cout << ";" << std::endl;
}
AssignOperator::~AssignOperator() { delete value; delete index; }
void AssignOperator::run(Block* parentBlock) {
    Var* targetVar; // variable to left of the '='
    if (isDereferecing)
        targetVar = parentBlock->findVar(ID)->getPtrVal();
    else
        targetVar = parentBlock->findVar(ID);

    if (targetVar == nullptr)
        throw UndefinedVarException(); // variable with given id doesn't exist
    else if (targetVar->getType() == T_INT && index != nullptr)
        throw InvalidTypeException("Int has no index");

    Var valueVar = value->eval(parentBlock);
    size_t assignIndex;
    if (index != nullptr)
        assignIndex = (size_t) index->eval(parentBlock).getIntVal();

    switch (targetVar->getType()) {
        case T_INT:
            targetVar->setIntVal(valueVar.getIntVal());
            break;
        case T_PTR:
            if (index)
                targetVar->setArrAtVal(valueVar.getIntVal(), assignIndex);
            else {
                if (valueVar.getType() == T_PTR)
                    targetVar->setPtrVal(valueVar.getPtrVal());
                else if (valueVar.getType() == T_ARR)
                    targetVar->setArrVal(valueVar.getArr(), valueVar.getArrSize(), valueVar.getArrInit());
            }
            break;
        case T_ARR:
            if (index)
                targetVar->setArrAtVal(valueVar.getIntVal(), assignIndex);
            else
                targetVar->setArrVal(valueVar.getArr(), valueVar.getArrSize(), valueVar.getArrInit());
            break;
    }
}


// Define Operator
DefOperator::DefOperator(VType T, const std::string& ID, Expression* value) :
    type(T), ID(ID), value(value) {
    size = 0;
}
DefOperator::DefOperator(VType T, const std::string& ID, const std::string& size, Expression* value) :
    type(T), ID(ID), size((unsigned) stoi(size)), value(value) {}
void DefOperator::print(unsigned indent) {
    //std::cout << "print def\n";
    std::cout << indentation(indent);
    if (type == T_INT)
        std::cout << "int " << ID;
    else if (type == T_PTR)
        std::cout << "ptr " << ID;
    else if (type == T_ARR)
        std::cout << "arr " << ID << "[" << size << "]";
    if (value != nullptr)
        std::cout << " = " << "expr";
    std::cout << ";" << std::endl;
}
void DefOperator::run(Block* parentBlock) {
    if (type == T_ARR && size != 0) {
        Var* newVar = new Var(T_ARR, size);
        parentBlock->addVar(ID, newVar);
    }
    else {
        Var* newVar = new Var(type);
        parentBlock->addVar(ID, newVar);
        if (value != nullptr) {
            AssignOperator* assignOp = new AssignOperator(ID, value);
            assignOp->run(parentBlock);
        }
    }
}


// Function call
FunctionCall::FunctionCall(const std::string& ID, const std::vector<Expression*>& args) :
    ID(ID), args(args) {}
void FunctionCall::print() {
    std::cout << ID << "(";
    for (auto i = args.begin(); i != args.end(); i++) {
        if (i != args.begin())
            std::cout << ", ";
        (*i)->print();
    }
    std::cout << ")";
}
FunctionCall::~FunctionCall() {
    for (auto i = args.begin(); i != args.end(); i++) {
        delete *i;
    }
}
Var FunctionCall::eval(Block* parentBlock) {
    Var resVar;
    if (ID == "malloc" && args.size() == 1) {
        size_t sizemem = (size_t) (args.front())->eval(parentBlock).getIntVal();
        resVar = Var(new int[sizemem], sizemem);
    }
    else if (ID == "print" && args.size() == 1) {
        if (PRINT_VAR_TABLE)
            std::cout << "PRINT: ";
        std::cout << args.front()->eval(parentBlock) << std::endl;
    }
    else if (ID == "printVarTable" && args.size() == 0) {
        parentBlock->printVarTable();
    }
    else {
        try {
            Program& p = Program::Instance();
            std::vector<Var> argsVar;
            for (auto arg : args)
                argsVar.push_back(arg->eval(parentBlock));
            resVar = p.runFunction(ID, argsVar);
        }
        catch (UndefinedFunctionException) {
            std::cout << "UndefinedFunctionException!!!" << std::endl;
        }
    }
    return resVar;
}

ReturnOperator::ReturnOperator(Expression* value) : value(value) {}
ReturnOperator::~ReturnOperator() {
    delete value;
}
void ReturnOperator::print(unsigned int indent) {
    std::cout << indentation(indent) << "return ";
    value->print();
    std::cout << ";" << std::endl;
}
void ReturnOperator::run(Block* parentBlock) {
    parentBlock->changeReturnValue( value->eval(parentBlock) );
}



// Unary Expression
UnaryExpression::UnaryExpression(const char* op, Expression* arg) :
    op(op), arg(arg) {}
void UnaryExpression::print() {
    std::cout << op;
    arg->print();
}
UnaryExpression::~UnaryExpression() { delete arg; }
Var UnaryExpression::eval(Block* parentBlock) {
    Var result;
    VarExpression* argVarExpr = dynamic_cast<VarExpression *>(arg);
    Var* argVar = nullptr;
    if (argVarExpr)
        argVar = parentBlock->findVar(argVarExpr->getID());

    switch (*op) {
        case '-':
            result = Var((-1) * arg->eval(parentBlock).getIntVal());
            break;
        case '!':
            result = ((arg->eval(parentBlock).getIntVal()) != 0 ? 0 : 1);
            break;
        case '&':
            if (argVar)
                result = Var(argVar);
            break;
        case '*':
            if (argVar)
                result = *(argVar->getPtrVal());
            break;
        default:
            if (strcmp(op, ".++") == 0) {
                if (argVar) {
                    result = Var(*(argVar));
                    argVar->setIntVal(argVar->getIntVal() + 1);
                }
                else
                    throw InvalidTypeException();
            }
            else if (strcmp(op, "++.") == 0) {
                if (argVar) {
                    argVar->setIntVal(argVar->getIntVal() + 1);
                    result = Var(*(argVar));
                }
                else
                    throw InvalidTypeException();
            }
            else if (strcmp(op, ".--") == 0) {
                if (argVar) {
                    result = Var(*(argVar));
                    argVar->setIntVal(argVar->getIntVal() - 1);
                }
                else
                    throw InvalidTypeException();
            }
            else if (strcmp(op, "--.") == 0) {
                if (argVar) {
                    argVar->setIntVal(argVar->getIntVal() - 1);
                    result = Var(*(argVar));
                }
                else
                    throw InvalidTypeException();
            }
            break;
    }
    return result;
}


// Binary Expression
BinaryExpression::BinaryExpression(const char* op, Expression* arg1, Expression* arg2) :
    op(op), arg1(arg1), arg2(arg2) {}
void BinaryExpression::print() {
    std::cout << "(";
    arg1->print();
    std::cout << " " << op << " ";
    arg2->print();
    std::cout << ")";
}
BinaryExpression::~BinaryExpression() { delete arg1; delete arg2; }
Var BinaryExpression::eval(Block* parentBlock) {
    Var result;
    if (*op == '+')
        result = Var(arg1->eval(parentBlock).getIntVal() + arg2->eval(parentBlock).getIntVal());
    else if (*op == '-')
        result = Var(arg1->eval(parentBlock).getIntVal() - arg2->eval(parentBlock).getIntVal());
    else if (*op == '*')
        result = Var(arg1->eval(parentBlock).getIntVal() * arg2->eval(parentBlock).getIntVal());
    else if (*op == '/')
        result = Var(arg1->eval(parentBlock).getIntVal() / arg2->eval(parentBlock).getIntVal());
    else if (*op == '%')
        result = Var(arg1->eval(parentBlock).getIntVal() % arg2->eval(parentBlock).getIntVal());
    else if (strcmp(op, "==") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() == arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (strcmp(op, "<=") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() <= arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (strcmp(op, ">=") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() >= arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (strcmp(op, "!=") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() != arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (*op == '>')
        result = Var((arg1->eval(parentBlock).getIntVal() > arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (*op == '<')
        result = Var((arg1->eval(parentBlock).getIntVal() < arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
    else if (strcmp(op, "&&") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() * arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);
    else if (strcmp(op, "||") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() + arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);

    return result;
}


// Array at Expression
ArrayAtExpression::ArrayAtExpression(std::string ID, Expression* index) : ID(ID), index(index) {}
Var ArrayAtExpression::eval(Block* parentBlock) {
    Var* sourceArr = parentBlock->findVar(ID);
    if (sourceArr == nullptr)
        throw UndefinedVarException();
    else if (sourceArr->getType() == T_INT && index != nullptr) {
        throw InvalidTypeException("Int has no index");
    }
    return Var(sourceArr->getArrAtVal((unsigned) index->eval(parentBlock).getIntVal()));
}
void ArrayAtExpression::print() {
    std::cout << ID << "[" << "index" << "]";
}


// Value Expression
Value::Value(const std::string& val) : val(atoi(val.c_str())) {}
void Value::print() { std::cout << val; }
Var Value::eval(Block* parentBlock) {
    return Var(val);
}


// Variable Expression
VarExpression::VarExpression(const std::string& ID) : ID(ID) {}
void VarExpression::print() { std::cout << ID; }
std::string VarExpression::getID() { return ID; }
Var VarExpression::eval(Block* parentBlock) {
    Var* thisVar = parentBlock->findVar(ID);
    if (thisVar == nullptr)
        throw UndefinedVarException();
    return *thisVar;
}


Parameter::Parameter(VType paramType, const std::string &id) : paramType(paramType), id(id) {}
Parameter::~Parameter() {}
VType Parameter::getParamType() const {
    return paramType;
}
const std::string &Parameter::getId() const {
    return id;
}

Function::Function(const std::string &id, VType returnType, const std::vector<Parameter *> &params, Block *body) :
        id(id), returnType(returnType), params(params), body(body) {}
Function::~Function() {
    for (auto param : params) {
        delete param;
    }
    params.clear();
}
Var Function::eval(const std::vector<Var>& args) {
    body->clearVarTable();
    if (args.size() != params.size())
        throw InvalidFunctionCall();
    body->addVar("#RESULT", new Var(returnType));
    for (size_t i = 0; i < params.size(); i++) {
        if (args[i].getType() == params[i]->getParamType())
            body->addVar(params[i]->getId(), new Var(args[i]));
        else
            throw InvalidFunctionCall();
    }
    body->run(nullptr);
    return *(body->findVar("#RESULT"));
}
const std::string &Function::getId() const {
    return id;
}

void Program::run() {
    for (auto func : funcs) {
        if (func->getId() == "main")
            func->eval(std::vector<Var>());
    }
}
void Program::setFuncs(std::list<Function *> f) {
    funcs = f;
}
Var Program::runFunction(std::string &id, std::vector<Var>& args) {
    for (auto func : funcs) {
        if (func->getId() == id)
            return func->eval(args);
    }
    throw UndefinedFunctionException();
}
