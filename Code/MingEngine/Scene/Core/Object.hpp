#pragma once

#include <string>

#define ADD_PROPERTY(propertyName, setterName, getterName)                                                             \
	ClassDatabase::AddProperty(GetStaticClassName(), propertyName, setterName, getterName)

#define MCLASS(className, inheritName)                                                                                 \
public:                                                                                                                \
	using Super = inheritName;                                                                                         \
	static std::string     GetStaticClassName() { return #className; }                                                 \
	virtual std::string    GetClassName() const { return #className; }                                                 \
	static BindMethodsFunc GetBindMethodsFunc() { return &className::BindMethods; }                                    \
	static void            InitializeClass()                                                                           \
	{                                                                                                                  \
		static bool isClassInitialized = false;                                                                        \
		if (isClassInitialized)                                                                                        \
		{                                                                                                              \
			return;                                                                                                    \
		}                                                                                                              \
		Super::InitializeClass();                                                                                      \
		if (className::GetBindMethodsFunc() != Super::GetBindMethodsFunc())                                            \
		{                                                                                                              \
			className::BindMethods();                                                                                  \
		}                                                                                                              \
		isClassInitialized = true;                                                                                     \
	}

class Object
{
public:
	using BindMethodsFunc = void (*)();

public:
	Object()          = default;
	virtual ~Object() = default;

	static void            BindMethods() {}
	static BindMethodsFunc GetBindMethodsFunc();

	static std::string  GetStaticClassName();
	virtual std::string GetClassName() const;
	static void         InitializeClass();
};
