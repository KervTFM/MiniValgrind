#pragma once

#include "Types.h"
#include "Exceptions.h"

enum VType;
enum EType;

class Var {
	VType type;
	int intVal;
	const Var* ptrVal;
	int* arrVal;
	size_t arrSize;
	bool isInit;
	bool* isArrInit;
public:
	Var();
	Var(VType t);
	Var(VType t, unsigned size);
	Var(int int_val);
	Var(Var* ptr_val);
	~Var();
	Var(const Var& other);
	Var& operator=(const Var& other);
	VType getType();
	int getIntVal();
	const Var* getPtrVal();
	int getArrAtVal(size_t i);
	size_t getArrSize();
	void setIntVal(int newVal);
	void setPtrVal(const Var* newVal);
	void setArrAtVal(int newVal, size_t i);
};
