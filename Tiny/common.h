#pragma once
#include <Windows.h>
#include <exception>
#include <string>
#include <format>
#include <iostream>
#include <chrono>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;
using f32 = float;
using f64 = double;

#ifdef TINY_BUILD_FX
#define TINYFX_API __declspec(dllexport)
#else
#define TINYFX_API __declspec(dllimport)
#endif

#ifdef TINY_BUILD_ENGINE
#define TINY_API __declspec(dllexport)
#else
#define TINY_API __declspec(dllimport)
#endif

#ifdef TINY_REVEAL_INTERNALS
#define _internal public
#else
#define _internal private
#endif

#define DISABLE_COPY(ClassName) \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

#define DISABLE_MOVE(ClassName) \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

#define DEFAULT_COPY(ClassName) \
    ClassName(const ClassName&) = default; \
    ClassName& operator=(const ClassName&) = default;

#define DEFAULT_MOVE(ClassName) \
    ClassName(ClassName&&) = default; \
    ClassName& operator=(ClassName&&) = default;

namespace tiny
{
    class EngineException : public std::exception {
    public:
        EngineException(int line, const char* file, const std::string& message = "Engine exception occurred.")
            : line_(line), file_(file), message_(message)
        {
            fullMessage_ = std::format("Error: {}\nFile: {}\nLine: {}", message_, file_, line_);
        }
        virtual const char* what() const noexcept override { return fullMessage_.c_str(); }
        int GetLine() const noexcept { return line_; }
        const std::string& GetFile() const noexcept { return file_; }
        const std::string& GetErrorMessage() const noexcept { return message_; }
    private:
        int line_;
        std::string file_;
        std::string message_;
        std::string fullMessage_;
    };

    class DxException : public EngineException {
    public:
        DxException(HRESULT hr, int line, const char* file, const std::string& message = "DirectX exception occurred.")
            : EngineException(line, file, message), hr_(hr)
        {
            fullDxMessage_ = std::format("DirectX Error: 0x{:X}\nMessage: {}\nFile: {}\nLine: {}", hr_, GetErrorMessage(), GetFile(), GetLine());
        }
        virtual const char* what() const noexcept override { return fullDxMessage_.c_str(); }
        HRESULT ErrorCode() const noexcept { return hr_; }
    private:
        HRESULT hr_;
        std::string fullDxMessage_;
    };
}

#define THROW_IF_FAILED_FMT(hr, formatStr, ...) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            throw DxException(_hr, __LINE__, __FILE__, std::format((formatStr), __VA_ARGS__)); \
        } \
    } while(0)

#define THROW_IF_FAILED(hr) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            throw DxException(_hr, __LINE__, __FILE__); \
        } \
    } while(0)

#define THROW_ENGINE_EXCEPTION(formatStr, ...) \
    throw tiny::EngineException(__LINE__, __FILE__, std::format((formatStr), __VA_ARGS__))


#define SAFE_RELEASE(p) \
    do { \
        if (p) { \
            p->Release(); \
            p = nullptr; \
        } \
    } while(0)

#define SAFE_DELETE(p) \
    do { \
        if (p) { \
            delete p; \
            p = nullptr; \
        } \
    } while(0)

#pragma region Engine Defaults

#define GFX_DEFAULT_SRV_HEAP_SIZE 1024
#define GFX_DEFAULT_SAMPLER_HEAP_SIZE 64
#define GFX_DEFAULT_RTV_HEAP_SIZE 512
#define GFX_DEFAULT_DSV_HEAP_SIZE 512

#define GFX_DEFAULT_COPY_QUEUE_SIZE 3

#define TINY_WINDOW_CLASS "TinyWindow"

#define TINY_DEFAULT_BUFFERING 3
#define TINY_MAX_SWAPCHAIN_BUFFERS 3
#define TINY_MAX_ENTRYPOINT_NAME 64

#define MAX_CONDITIONAL_COMPILATIONS 8
#define TINY_MAX_SHADER_RESOURCE_NAME 64

#pragma endregion

#undef min
#undef max