/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN-Check contributors:
 *   - Ievgen Khvedchenia <https://github.com/BloodAxe>
 *
 * MIT License <https://github.com/BloodAxe/nan-check/blob/master/LICENSE.md>
 *
 * Version 0.0.1: current Node 4.0.0, Node 12: 0.12.7, Node 10: 0.10.40, iojs: 3.2.0
 *
 * See https://github.com/BloodAxe/nan-check for the latest update to this file
 **********************************************************************************/

#pragma once 

#include <nan.h>
#include <node_buffer.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <array>
#include <map>

namespace Nan
{

    class CheckException : std::runtime_error
    {
    public:

        CheckException(const std::string& msg);
        CheckException(int actual, int expected);
        CheckException(int actual, const std::initializer_list<int>& expected);

        virtual const char * what() const override
        {
            return mMessage.c_str();
        }

        virtual ~CheckException()
        {
        }

    private:
        std::string mMessage;
    };

    typedef std::function<bool(Nan::NAN_METHOD_ARGS_TYPE args) > InitFunction;

    class MethodArgBinding;
    class CheckArguments;

    template <typename EnumType>
    class ArgStringEnum;

    //////////////////////////////////////////////////////////////////////////

    class CheckArguments
    {
    public:
        CheckArguments(Nan::NAN_METHOD_ARGS_TYPE args);

        CheckArguments& ArgumentsCount(int count);
        CheckArguments& ArgumentsCount(int argsCount1, int argsCount2);

        MethodArgBinding Argument(int index);

        /**
         * Unwind all fluent calls
         */
        operator bool() const;

        CheckArguments& AddAndClause(InitFunction rightCondition);
        CheckArguments& Error(std::string * error);

    private:
        Nan::NAN_METHOD_ARGS_TYPE m_args;
        InitFunction          m_init;
        std::string         * m_error;
    };

    //////////////////////////////////////////////////////////////////////////

    template <typename EnumType>
    class ArgStringEnum
    {
    public:
        explicit ArgStringEnum(
            std::initializer_list< std::pair<const char*, EnumType> > possibleValues,
            MethodArgBinding& owner,
            int argIndex);

        CheckArguments& Bind(EnumType& value);
    protected:

        bool TryMatchStringEnum(const std::string& key, EnumType& outValue) const;

    private:
        std::map<std::string, EnumType>     mPossibleValues;
        MethodArgBinding&                mOwner;
        int                                 mArgIndex;
    };

    //////////////////////////////////////////////////////////////////////////

    /**
     * @brief This class wraps particular positional argument
     */
    class MethodArgBinding
    {
    public:

        template <typename EnumType>
        friend class ArgStringEnum;

        MethodArgBinding(int index, CheckArguments& parent);

        MethodArgBinding& IsBuffer();
        MethodArgBinding& IsFunction();
        MethodArgBinding& IsString();
        MethodArgBinding& NotNull();
        MethodArgBinding& IsArray();
        MethodArgBinding& IsObject();

        template <typename T>
        ArgStringEnum<T> StringEnum(std::initializer_list< std::pair<const char*, T> > possibleValues);

        template <typename T>
        CheckArguments& Bind(v8::Local<T>& value);

        template <typename T>
        CheckArguments& Bind(T& value);

        template <typename T1, typename T2>
        CheckArguments& BindAny(T1& value1, T2& value2);

    private:
        int              mArgIndex;
        CheckArguments&  mParent;
    };

    //////////////////////////////////////////////////////////////////////////

    CheckArguments NanCheck(Nan::NAN_METHOD_ARGS_TYPE args);

    //////////////////////////////////////////////////////////////////////////
    // Template functions implementation

    template <typename T>
    CheckArguments& MethodArgBinding::Bind(v8::Local<T>& value)
    {
        return mParent.AddAndClause([this, &value](Nan::NAN_METHOD_ARGS_TYPE args) {
            value = args[mArgIndex].As<T>();
            return true;
        });
    }


    template <typename T>
    CheckArguments& MethodArgBinding::Bind(T& value)
    {
        return mParent.AddAndClause([this, &value](Nan::NAN_METHOD_ARGS_TYPE args) {
            try
            {
                value = To<T>(args[mArgIndex]).FromJust();
                return true;
            }
            catch (...)
            {
                return false;
            }
        });
    }

    template <typename T1, typename T2>
    CheckArguments& MethodArgBinding::BindAny(T1& value1, T2& value2)
    {
        return mParent.AddAndClause([this, &value1, &value2](Nan::NAN_METHOD_ARGS_TYPE args) {
            value1 = To<T1>(args[mArgIndex]).FromJust();
            value2 = To<T2>(args[mArgIndex]).FromJust();
            return true;
        });
    }

    template <typename T>
    ArgStringEnum<T> MethodArgBinding::StringEnum(std::initializer_list< std::pair<const char*, T> > possibleValues)
    {
        return std::move(ArgStringEnum<T>(possibleValues, IsString(), mArgIndex));
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename T>
    ArgStringEnum<T>::ArgStringEnum(
        std::initializer_list< std::pair<const char*, T> > possibleValues,
        MethodArgBinding& owner, int argIndex)
        : mPossibleValues(possibleValues.begin(), possibleValues.end())
        , mOwner(owner)
        , mArgIndex(argIndex)
    {
    }

    template <typename T>
    CheckArguments& ArgStringEnum<T>::Bind(T& value)
    {
        return mOwner.mParent.AddAndClause([this, &value](Nan::NAN_METHOD_ARGS_TYPE args) {
            std::string key = To<std::string>(args[mArgIndex]).FromJust();
            return TryMatchStringEnum(key, value);
        });
    }

    template <typename T>
    bool ArgStringEnum<T>::TryMatchStringEnum(const std::string& key, T& outValue) const
    {
        auto it = mPossibleValues.find(key);
        if (it != mPossibleValues.end())
        {
            outValue = it->second;
            return true;
        }

        return false;
    }

}

namespace Nan
{

    CheckException::CheckException(const std::string& msg)
        : std::runtime_error(nullptr)
        , mMessage(msg)
    {
    }

    CheckException::CheckException(int actual, int expected)
        : std::runtime_error(nullptr)
        , mMessage("Invalid number of arguments passed to a function")
    {
    }

    CheckException::CheckException(int actual, const std::initializer_list<int>& expected)
        : std::runtime_error(nullptr)
        , mMessage("Invalid number of arguments passed to a function")
    {
    }

    typedef std::function<bool(Nan::NAN_METHOD_ARGS_TYPE) > InitFunction;

    class MethodArgBinding;
    class CheckArguments;

    MethodArgBinding::MethodArgBinding(int index, CheckArguments& parent)
        : mArgIndex(index)
        , mParent(parent)
    {
    }

    MethodArgBinding& MethodArgBinding::IsBuffer()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            bool isBuf = node::Buffer::HasInstance(args[mArgIndex]);

            if (!isBuf)
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates IsBuffer check");
            return true;
        };

        mParent.AddAndClause(bind);
        return *this;
    }

    MethodArgBinding& MethodArgBinding::IsFunction()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            bool isFn = args[mArgIndex]->IsFunction();

            if (!isFn)
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates IsFunction check");

            return true;
        };
        mParent.AddAndClause(bind);
        return *this;
    }

    MethodArgBinding& MethodArgBinding::IsArray()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            bool isArr = args[mArgIndex]->IsArray();
            if (!isArr)
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates IsArray check");

            return true;
        };
        mParent.AddAndClause(bind);
        return *this;
    }

    MethodArgBinding& MethodArgBinding::IsObject()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            bool isArr = args[mArgIndex]->IsObject();
            if (!isArr)
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates IsObject check");

            return true;
        };
        mParent.AddAndClause(bind);
        return *this;
    }

    MethodArgBinding& MethodArgBinding::IsString()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            bool isStr = args[mArgIndex]->IsString() || args[mArgIndex]->IsStringObject();

            if (!isStr)
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates IsString check");

            return true;
        };
        mParent.AddAndClause(bind);
        return *this;
    }

    MethodArgBinding& MethodArgBinding::NotNull()
    {
        auto bind = [this](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            if (args[mArgIndex]->IsNull())
            {
                throw CheckException(std::string("Argument ") + std::to_string(mArgIndex) + " violates NotNull check");
            }
            return true;
        };
        mParent.AddAndClause(bind);
        return *this;
    }

    CheckArguments::CheckArguments(Nan::NAN_METHOD_ARGS_TYPE args)
        : m_args(args)
        , m_init([](Nan::NAN_METHOD_ARGS_TYPE args) { return true; })
        , m_error(0)
    {
    }


    CheckArguments& CheckArguments::ArgumentsCount(int count)
    {
        return AddAndClause([count](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            if (args.Length() != count)
                throw CheckException(args.Length(), count);

            return true;
        });
    }

    CheckArguments& CheckArguments::ArgumentsCount(int argsCount1, int argsCount2)
    {
        return AddAndClause([argsCount1, argsCount2](Nan::NAN_METHOD_ARGS_TYPE args)
        {
            if (args.Length() != argsCount1 || args.Length() != argsCount2)
                throw CheckException(args.Length(), { argsCount1, argsCount2 });

            return true;
        });
    }

    MethodArgBinding CheckArguments::Argument(int index)
    {
        return MethodArgBinding(index, *this);
    }

    CheckArguments& CheckArguments::Error(std::string * error)
    {
        m_error = error;
        return *this;
    }

    /**
     * Unwind all fluent calls
     */
    CheckArguments::operator bool() const
    {
        try
        {
            return m_init(m_args);
        }
        catch (CheckException& exc)
        {
            if (m_error)
            {
                *m_error = exc.what();
            }
            return false;
        }
        catch (...)
        {
            if (m_error)
            {
                *m_error = "Unknown error";
            }
            return false;
        }
    }

    CheckArguments& CheckArguments::AddAndClause(InitFunction rightCondition)
    {
        InitFunction prevInit = m_init;
        InitFunction newInit = [prevInit, rightCondition](Nan::NAN_METHOD_ARGS_TYPE args) {
            return prevInit(args) && rightCondition(args);
        };
        m_init = newInit;
        return *this;
    }

    CheckArguments Check(Nan::NAN_METHOD_ARGS_TYPE args)
    {
        return std::move(CheckArguments(args));
    }
}