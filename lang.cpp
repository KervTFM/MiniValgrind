#include <cstdlib>
#include <list>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

enum Type {T_INT, T_PTR, T_ARR};


class Operator {
protected: Operator() {}
public: virtual ~Operator() {}
    virtual void print(int indent = 0) = 0;
    virtual void run(Block* parentBlock = nullptr) = 0;
};

class Expression {
protected: Expression() {}
public: virtual ~Expression() {}
    virtual void print() = 0;
};

class Var {
    int intVal;
    int* ptrVal;
    std::vector<int> arrVal;
    Type type;
    bool isInit;
public:
    Var(int int_val) {
        type = Type::T_INT;
        intVal = int_val;
        ptrVal = nullptr;
        arrVal = nullptr;
        isInit = false;
    }
    Var(int* ptr_val) {
        type = Type::T_PTR;
        intVal = 0;
        ptrVal = ptr_val;
        arrVal = nullptr;
        isInit = false;
    }
    Var(std::vector<int> arrVal) {
        type = Type::T_ARR;
        intVal = 0;
        ptrVal = nullptr;
        arrVal = std::vector<int>(arrVal);
        isInit = false; // сделать isInit для каждого элемента массива?
        // например, с помощью vector<pair<int, bool> >, или иначе...
    }
    Type getType() {
        return type;
    }
    int getIntVal() {
        if (type != Type::T_INT)
            throw std::logic_error("Invalid value's type");
        else
            return intVal;
    }
    int* getPtrVal() {
        if (type != Type::T_PTR)
            throw std::logic_error("Invalid value's type");
        else
            return ptrVal;
    }
    int getArrAtVal(size_t i) {
        if (type != Type::T_ARR)
            throw std::logic_error("Invalid value's type");
        else {
            if (i <= arrVal.size())
                return arrVal[i];
            else
                throw std::logic_error("Escape from the bounds of array");
        }
    }

    void setIntVal(int newVal) {
        if (type != Type::T_INT)
            throw std::logic_error("Invalid value's type");
        else
            intVal = newVal;
    }
    void setPtrVal(int* newVal) {
        if (type != Type::T_PTR)
            throw std::logic_error("Invalid value's type");
        else
            ptrVal = newVal;
    }
    void setArrAtVal(int newVal, size_t i) {
        if (type != Type::T_ARR)
            throw std::logic_error("Invalid value's type");
        else {
            if (i <= arrVal.size())
                arrVal[i] = newVal;
            else
                throw std::logic_error("Escape from the bounds of array");
        }
    }
};

class Block : public Operator {
private:
    std::list<Operator*> ops;
    std::map<std::string, Var> vars;
    Block* parentBlock;

    void addOperator(Operator* op) {
        Block* newBlock = dynamic_cast<Block*>(op);

        if (newBlock) {
            ops.splice(ops.end(), newBlock->ops, newBlock->ops.begin(),
                       newBlock->ops.end());
        }
        else
            ops.push_back(op);
    }

public:
    Block() {}
    Block(Operator* op) { addOperator(op); }
    Block(Operator* op1, Operator* op2) {
        addOperator(op1);
        addOperator(op2);
    }
    size_t size() { return ops.size(); }
    virtual void print(unsigned indent = 0) {
        for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); ++i) {
            std::cout << std::string(indent, '\t');
            (*i)->print(indent);
        }
    }
    virtual ~Block() {
        for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); ++i) {
            delete *i;
        }
    }
    virtual void run(Block* parentBlock = nullptr) {
        this->parentBlock = parentBlock;
        for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); ++i) {
            (*i)->run(this);
        }
    }
    std::map<std::string, Var>::iterator findVar(std::string id) {
        Block* currBlock = this;
        // будем возвращать intVars.end() (для блока, из которого вызывается поиск),
        // если указанная переменная не найдена
        std::map<std::string, Var>::iterator result = this->vars.end();
        while (result == vars.end() && currBlock != nullptr) {
            if (currBlock->vars.find(id) != currBlock->vars.end())
                result = currBlock->vars.find(id); // win!
            else
                currBlock = currBlock->parentBlock;
        }
        return result;
    }
    std::map<std::string, Var>::iterator getVarsEnd() {
        return vars.end();
    }
};

class ExprOperator : public Operator {
private:
    Expression* expr;

public:
    ExprOperator(Expression* expr) : expr(expr) {}
    virtual void print(int indent = 0) {
        expr->print();
        std::cout << ";" << std::endl;
    }
    virtual ~ExprOperator() { delete expr; }
    virtual void run(Block* parentBlock = nullptr) {
        AssignExpression* probAssign = dynamic_cast<AssignExpression*>(expr);
        FunctionCall* probFunCall = dynamic_cast<FunctionCall*>(expr);
        if (probAssign) {
            probAssign->run(parentBlock);
        }
        if (probFunCall) {
            probFunCall->run(parentBlock);
        }
    }
};

class IfOperator : public Operator {
private:
    Expression* cond;
    Block thenBlock, elseBlock;
public:
    IfOperator(Expression* cond, Operator* thenBlock, Operator* elseBlock) :
            cond(cond), thenBlock(thenBlock), elseBlock(elseBlock) {}
    virtual void print(unsigned indent = 0) {
        std::cout << "if "; cond->print();  std::cout << " {" << std::endl;
        thenBlock.print(indent+1);
        if (elseBlock.size()) {
            std::cout << std::string(indent, '\t') << "}" << std::string(indent, '\t') <<
                      "else {" << std::endl;
            elseBlock.print(indent+1);
        }
        std::cout << std::string(indent, '\t') << "}" << std::endl;
    }
    virtual ~IfOperator() { delete cond; }

};

class WhileOperator: public Operator {
private:
    Expression* cond;
    Block block;
public:
    WhileOperator(Expression* cond, Operator* block) :
            cond(cond), block(block) {}
    virtual void print(unsigned indent = 0) {
        std::cout << "while ";
        cond->print();
        std::cout << " {" << std::endl;
        block.print(indent + 1);
        std::cout << std::string(indent, '\t') << "}" << std::endl;
    }
    virtual ~WhileOperator() { delete cond; }

};

class UnaryExpression : public Expression {
private:
    const char* op;
    Expression *arg;
public:
    UnaryExpression(const char* op, Expression* arg) :
            op(op), arg(arg) {}
    virtual void print() {
        std::cout << op;
        arg->print();
    }
    virtual ~UnaryExpression() { delete arg; }
    int calculate() {
        // на самом деле может быть &x, поэтому не int, а ???
    }
};

class BinaryExpression : public Expression {
private:
    const char* op;
    Expression *arg1, *arg2;
public:
    BinaryExpression(const char* op, Expression* arg1, Expression* arg2) :
            op(op), arg1(arg1), arg2(arg2) {}
    virtual void print() {
        std::cout << "(";
        arg1->print();
        std::cout << " " << op << " ";
        arg2->print();
        std::cout << ")";
    }
    virtual ~BinaryExpression() { delete arg1; delete arg2; }
    int calculate() {
        // на самом деле может быть &x, поэтому не int, а ???
    }
};

class AssignExpression : public Expression {
private:
    std::string ID;
    Expression* value;
public:
    AssignExpression(const std::string& ID, Expression* value) :
            ID(ID), value(value) {}
    virtual void print() {
        std::cout << ID << " = ";
        value->print();
    }
    virtual ~AssignExpression() { delete value; }
    void run(Block* parentBlock = nullptr) {
        int arrIndex = -1; // only for array
        if (ID.find("[") != -1) {
            arrIndex = std::stoi(ID.substr(ID.find("["), ID.find("]"))); // или find("]") - 1?
        }
        std::string clearID = ID.erase(ID.find("["), ID.find("]")); // a[12] --> a

        // value may be: unaryexpr, binaryexpr, value, variable, funccal
        Value* probValue = dynamic_cast<Value*>(value);
        Variable* probVar = dynamic_cast<Variable*>(value);
        FunctionCall* probFunCall = dynamic_cast<FunctionCall*>(value);
        UnaryExpression* probUnaryExpr = dynamic_cast<UnaryExpression*>(value);
        BinaryExpression* probBinExpr = dynamic_cast<BinaryExpression*>(value);

        std::map<std::string, Var>::iterator iterVar = parentBlock->findVar(clearID);
        if (iterVar == parentBlock->getVarsEnd())
            throw std::logic_error("Unknown variable");

        if (probValue) {
            switch (iterVar->second.getType()) {
                case Type::T_INT : {
                    iterVar->second.setIntVal(probValue->getVal());
                }
                case Type::T_PTR : {
                    throw std::logic_error("Assign int to ptr");
                }
                case Type::T_ARR : {
                    if (arrIndex != -1)
                        iterVar->second.setArrAtVal(probValue->getVal(), arrIndex);
                    else
                        throw std::logic_error("Assign int to arr without index");
                }
            }
        }
        if (probVar) {
            switch (iterVar->second.getType()) {
                case Type::T_INT : {

                }
                case Type::T_PTR : {

                }
                case Type::T_ARR : {

                }
            }
        }

    }
};

class FunctionCall : public Expression {
private:
    std::string ID;
    std::list<Expression*> args;
public:
    FunctionCall(const std::string& ID, const std::list<Expression*>& args) :
            ID(ID), args(args) {}
    virtual void print() {
        std::cout << ID << "(";
        for (std::list<Expression*>::iterator i = args.begin(); i != args.end(); ++i) {
            if (i != args.begin())
                std::cout<<", ";
            (*i)->print();
        }
        std::cout << ")";
    }
    virtual ~FunctionCall() {
        for (std::list<Expression*>::iterator i = args.begin(); i != args.end(); ++i) {
            delete *i;
        }
    }
    void run(Block* parentBlock = nullptr) {
        // нужно ли?
    }
};


class DefExpression : public Expression {
private:
    Type T;
    std::string ID;
    unsigned size;
public:
    DefExpression(Type T, const std::string& ID) :
            T(T), ID(ID) { size = 1; }
    DefExpression(Type T, const std::string& ID, const std::string& size) :
            T(T), ID(ID), size(atoi(size.c_str())) {} // to do: stoi
    virtual void print() {
        if (T == T_INT)
            std::cout << "int " << ID;
        else if (T == T_PTR)
            std::cout << "ptr " << ID;
        else
            std::cout << "arr " << ID << "[" << size << "]";
    }
};

class Value : public Expression {
private:
    int val;
public:
    Value(const std::string& val) : val(atoi(val.c_str())) {}
    virtual void print() { std::cout << val; }
    int getVal() {
        return val;
    }
};

class Variable : public Expression {
private:
    std::string ID;
public:
    Variable(const std::string& ID, Type type) : ID(ID) {}
    virtual void print() { std::cout << ID; }
};
