#include "Operators.h"

// print table of variables after run each operator in program
#define PRINT_VAR_TABLE false

std::string VTypeToString(VType t) {
    switch (t) {
        case T_INT:
            return "int";
        case T_PTR:
            return "ptr";
        case T_ARR:
            return "arr";
    }
}

bool VTypeIsCompatible(VType t1, VType t2) {
    if (t1 == t2)
        return true;
    else if ((t1 == T_PTR && t2 == T_ARR) || (t1 == T_ARR && t2 == T_PTR))
        return true;
    return false;
}

std::string indentation(unsigned indent) {
    std::string res = "";
    for (unsigned i = 0; i < indent; i++)
        res += "  ";
    return res;
}

// Block Operator
Block::Block() { parentBlock = nullptr; }
Block::Block(std::list<Operator*> newOps) {
    ops = newOps;
    parentBlock = nullptr;
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
            std::cout << "Error:    " << ex.what() << std::endl;
            //std::string skipLine;
            //std::getline(std::cin, skipLine);
            std::cout << "Aborted." << std::endl;
            exit(1);
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
    else {
        delete newVar;
        throw UndefinedVarException("Variable " + id + "is already exists");
    }
}
void Block::printVarTable() const {
    if (parentBlock) {
        std::cout << "**** Variables of nested block ****" << std::endl;
        parentBlock->printVarTable();
    }
    else
        std::cout << "**** Variables ****" << std::endl;
    for (std::map<std::string, Var*>::const_iterator i = vars.begin(); i != vars.end(); i++) {
        std::cout << VTypeToString((*i).second->getType()) << " ";
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
    Var* blockResult = findVar("#RESULT");
    if (blockResult != nullptr) {
        if (VTypeIsCompatible(blockResult->getType(), resultVal.getType()))
            *blockResult = resultVal;
        else
            throw InvalidTypeException("Returned variable must be " + VTypeToString(blockResult->getType()) +
                                        ", not " + VTypeToString(resultVal.getType()));
    }
    else
        throw UndefinedVarException("Block has no #RESULT for return");
}
bool Block::isReturned() {
    Var* blockResult = findVar("#RESULT");
    if (blockResult != nullptr)
        return blockResult->isVarInit();
    return false;
}
std::map<std::string, Var*> Block::getVars() {
    return vars;
}
Block::Block(const Block &block) {
    ops = block.ops;
    parentBlock = block.parentBlock;
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
IfOperator::~IfOperator() {
    delete cond;
    delete thenBlock;
    delete elseBlock;
}
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
WhileOperator::~WhileOperator() { delete cond; delete body; }
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
        initOp(initOp), cond(cond), stepOp(stepOp), body(body) {
    ownBlock = new Block(std::list<Operator*>(1, initOp));
}
ForOperator::~ForOperator() {
    delete cond;
    delete stepOp;
    delete ownBlock;
    delete body;
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
    ownBlock->run(parentBlock);
    int condResult = cond->eval(ownBlock).getIntVal();
    while (condResult == 1) {
        body->run(ownBlock);
        body->clearVarTable();
        stepOp->run(ownBlock);
        condResult = cond->eval(ownBlock).getIntVal();
    }
    ownBlock->clearVarTable();
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
AssignOperator::~AssignOperator() {
    delete value;
    delete index;
}
void AssignOperator::print(unsigned indent) {
    std::cout << indentation(indent) << (isDereferecing ? "*" : "") << ID << " = ";
    value->print();
    std::cout << ";" << std::endl;
}
void AssignOperator::run(Block* parentBlock) {
    Var* targetVar; // variable to left of the '='
    if (isDereferecing)
        targetVar = parentBlock->findVar(ID)->getPtrVal();
    else
        targetVar = parentBlock->findVar(ID);

    if (targetVar == nullptr)
        throw UndefinedVarException("Variable " + ID + " doesn't exist");
    else if (targetVar->getType() == T_INT && index != nullptr)
        throw InvalidTypeException("Int has no index");

    Var valueVar = value->eval(parentBlock);
    size_t assignIndex = 0;
    if (index)
        assignIndex = (size_t) index->eval(parentBlock).getIntVal();

    FunctionCall* valueAsFunCall = dynamic_cast<FunctionCall*>(value);
    bool isMalloc = (valueAsFunCall && valueAsFunCall->getID() == "malloc");
    Program &p = Program::Instance();

    switch (targetVar->getType()) {
        case T_INT:
            targetVar->setIntVal(valueVar.getIntVal());
            break;
        case T_PTR:
            if (index)
                targetVar->setArrAtVal(valueVar.getIntVal(), assignIndex);
            else {
                if (valueVar.getType() == T_PTR) {
                    targetVar->setPtrVal(valueVar.getPtrVal());
                    targetVar->setArrVal(valueVar.getArr(), valueVar.getArrInit());
                }
                else if (valueVar.getType() == T_ARR) {
                    targetVar->setArrVal(valueVar.getArr(), valueVar.getArrInit());
                    if (isMalloc) // add pointer to allocated memory to program storage
                        p.addAllocated(targetVar->getArr().get());
                }
            }
            break;
        case T_ARR:
            if (index)
                targetVar->setArrAtVal(valueVar.getIntVal(), assignIndex);
            else
                targetVar->setArrVal(valueVar.getArr(), valueVar.getArrInit());
            break;
    }
}


// Define Operator
DefOperator::DefOperator(VType T, const std::string& ID, Expression* value) :
        type(T), ID(ID), value(value) {
    size = 0;
    if (value)
        assignOp = new AssignOperator(ID, value);
}
DefOperator::DefOperator(VType T, const std::string& ID, const std::string& size, Expression* value) :
        type(T), ID(ID), size((unsigned) stoi(size)), value(value) {
    if (value)
        assignOp = new AssignOperator(ID, value);
}
DefOperator::~DefOperator() { delete assignOp; }
void DefOperator::print(unsigned indent) {
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
    Program& p = Program::Instance();
    if (ID == "malloc" && args.size() == 1) {
        size_t sizemem = (size_t) (args.front())->eval(parentBlock).getIntVal();
        resVar = Var(T_ARR, sizemem);
    }
    else if (ID == "free" && args.size() == 1) {
        VarExpression* varExpr = dynamic_cast<VarExpression*>(args.front());
        if (!varExpr)
            throw InvalidTypeException("Argument of free function must be variable");
        Var* var = parentBlock->findVar(varExpr->getID());
        if (!var)
            throw UndefinedVarException();
        auto freeMemory = var->getArr().get();
        p.removeAllocated(freeMemory);
        var->freeArr();
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
        std::vector<Var> argsVar;
        for (auto arg : args)
            argsVar.push_back(arg->eval(parentBlock));
        resVar = p.runFunction(ID, argsVar);
    }
    return resVar;
}
std::string FunctionCall::getID() { return ID; }

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
    else if (strcmp(op, "&&") == 0) {
        if (arg1->eval(parentBlock).getIntVal() == 0)
            result = Var(0);
        else
            result = Var((arg1->eval(parentBlock).getIntVal() * arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);
    }
    else if (strcmp(op, "||") == 0)
        result = Var((arg1->eval(parentBlock).getIntVal() + arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);

    return result;
}


// Array at Expression
ArrayAtExpression::ArrayAtExpression(std::string ID, Expression* index) : ID(ID), index(index) {}
ArrayAtExpression::~ArrayAtExpression() { delete index; }
Var ArrayAtExpression::eval(Block* parentBlock) {
    Var* sourceArr = parentBlock->findVar(ID);
    if (sourceArr == nullptr)
        throw UndefinedVarException("Array " + ID + " doesn't exist");
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
Value::~Value() { }
void Value::print() { std::cout << val; }
Var Value::eval(Block* parentBlock) {
    return Var(val);
}


// Variable Expression
VarExpression::VarExpression(const std::string& ID) : ID(ID) {}
VarExpression::~VarExpression() {

}
void VarExpression::print() { std::cout << ID; }
std::string VarExpression::getID() { return ID; }
Var VarExpression::eval(Block* parentBlock) {
    Var* thisVar = parentBlock->findVar(ID);
    if (thisVar == nullptr)
        throw UndefinedVarException(ID);
    return *thisVar;
}

Globals::Globals() {}
Globals::Globals(Block *globalsBlock) : globalsBlock(globalsBlock) {}
void Globals::addToBlock(Block *block) {
    globalsBlock->run(nullptr);
    auto globalVars = globalsBlock->getVars();
    for (auto var : globalVars) {
        block->addVar(var.first, var.second);
    }
}

Parameter::Parameter(VType paramType, const std::string &id) : paramType(paramType), id(id) {}
Parameter::~Parameter() {}
VType Parameter::getParamType() const {
    return paramType;
}
const std::string &Parameter::getId() const {
    return id;
}

Function::Function(const std::string &id, VType returnType, const std::vector<Parameter*> params, Block *body) :
        id(id), returnType(returnType), params(params), body(body) {}
Function::~Function() {
    for (auto param : params)
        delete param;
    params.clear();
    delete body;
}
// MiniValgrind doesn't support call stack, so if you call function inside it,
// the first call will lost
Var Function::eval(const std::vector<Var>& args, Block* globalBlock) {
    body->clearVarTable();
    if (args.size() != params.size())
        throw InvalidFunctionCall("too many (few) arguments");
    body->addVar("#RESULT", new Var(returnType));
    for (size_t i = 0; i < params.size(); i++) {
        if (args[i].getType() == params[i]->getParamType())
            body->addVar(params[i]->getId(), new Var(args[i]));
        else
            throw InvalidFunctionCall("invalid parameter's type (" + VTypeToString(args[i].getType()) +
                                      "instead of " + VTypeToString(params[i]->getParamType()) + ")");
    }
    body->run(globalBlock);
    Var res = *(body->findVar("#RESULT"));
    body->clearVarTable();
    return res;
}
const std::string &Function::getId() const {
    return id;
}

void Program::run() {
    auto defaultArgs = std::vector<Var>();
    runFunction("main", defaultArgs);
}
void Program::setFuncs(std::list<Function *> f) {
    funcs = f;
}
void Program::setGlobals(Globals* globs) {
    globalBlock = new Block();
    globs->addToBlock(globalBlock);
}
void Program::finalize() {
    for (auto func : funcs)
        delete func;
    delete globalBlock;
    int totalLeakSize = 0;
    for (auto ptr : allocated)
        totalLeakSize += ptr->size();
    if (totalLeakSize > 0) {
        std::cout << "Memory leaks: " << allocated.size() << " blocks, ";
        std::cout << "total size = " << totalLeakSize << " int" << std::endl;
    }
}
Var Program::runFunction(std::string id, std::vector<Var>& args) {
    for (auto func : funcs) {
        if (func->getId() == id)
            return func->eval(args, globalBlock);
    }
    throw UndefinedFunctionException(id);
}
void Program::addAllocated(std::vector<int>* allocatedMemory) {
    allocated.push_back(allocatedMemory);
}
void Program::removeAllocated(std::vector<int>* freeMemory) {
    auto it = std::find(allocated.begin(), allocated.end(), freeMemory);
    if (it != allocated.end())
        allocated.erase(it);
    else
        std::cout << "Already free" << std::endl;
}

