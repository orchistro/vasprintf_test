#if !defined(__clang__) && defined(__GNUC__)
#include <malloc.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <exception>
#include <functional>
#include <iostream>

#if !defined(__clang__) && defined(__GNUC__)
/* Prototypes for our hooks.  */
static void *my_malloc_hook (size_t, const void *);
static void my_free_hook (void*, const void *);
static void *my_realloc_hook (void *ptr, size_t size, const void *caller);

static decltype(__malloc_hook) original_malloc_hook;
static decltype(__free_hook) original_free_hook;
static decltype(__realloc_hook) original_realloc_hook;

static void setupHooks(void)
{
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
    __realloc_hook = my_realloc_hook;
}

static void restoreHooks(void)
{
    __malloc_hook = original_malloc_hook;
    __free_hook = original_free_hook;
    __realloc_hook = original_realloc_hook;
}

static void* my_malloc_hook(size_t size, const void *caller)
{
    void *result;

    restoreHooks();

    result = malloc (size);
    printf ("malloc (%u) returns %p\n", (unsigned int) size, result);

    setupHooks();

    return result;
}

static void my_free_hook (void *ptr, const void *caller)
{
    restoreHooks();

    free (ptr);
    printf ("freed pointer %p\n", ptr);

    setupHooks();
}

static void *my_realloc_hook (void *ptr, size_t size, const void *caller)
{
    void *result;

    restoreHooks();

    result = realloc(ptr, size);
    printf ("realloc pointer %p --> %p (size:%zu)\n", ptr, result, size);

    setupHooks();

    return result;
}
#endif

void my_init(void)
{
#if !defined(__clang__) && defined(__GNUC__)
    original_malloc_hook = __malloc_hook;
    original_free_hook = __free_hook;
    original_realloc_hook = __realloc_hook;

    setupHooks();
#endif
}

void my_final(void)
{
#if !defined(__clang__) && defined(__GNUC__)
    restoreHooks();
#endif
}

std::string makeString_vasprintf(const char *aFormat, ...)
{
    int32_t sLen;
    va_list sAp;

    if (aFormat == nullptr)
    {
        return std::string();
    }

    char* sBuffer;

    va_start(sAp, aFormat);
    sLen = vasprintf(&sBuffer, aFormat, sAp);
    va_end(sAp);

    if (sLen >= 0)
    {
        std::unique_ptr<char, std::function<void(void*)>> sOwner(sBuffer, [](void* aPtr){ free(aPtr); });
        return std::string(sBuffer, sLen);
    }
    else
    {
        std::cout << aFormat << "\n";
        throw std::runtime_error("makeString_vasprintf(): vasprintf() returned subzero value. Possible memory deficit");
    }
}

std::string makeString_vsnprintf(const char *aFormat, ...)
{
    int32_t sLen;
    va_list sAp;

    if (aFormat == nullptr)
    {
        return std::string();
    }

    va_start(sAp, aFormat);
    sLen = std::vsnprintf(nullptr, 0, aFormat, sAp);
    va_end(sAp);

    std::unique_ptr<char[]> sBuffer(new char[sLen + 1]);

    va_start(sAp, aFormat);
    (void)std::vsnprintf(sBuffer.get(), sLen + 1, aFormat, sAp);
    va_end(sAp);

    return std::string(sBuffer.get(), sLen);
}

static std::string gen_string(const int32_t length)
{
    std::string result;
    result.reserve(length);

    for (int32_t i = 0; i < length; i++)
    {
        char c = std::rand() % (126 - 32); // 126:~ 32:space
        c += 32;
        if (c == '%' || c == '\\')
        {
            c = '_';
        }
        result.push_back(c);
    }

    return result;
}

static std::vector<std::string> gen_strings(const int32_t count, const int32_t min_len, const int32_t max_len)
{
    std::vector<std::string> result;
    result.reserve(count);

    for (int32_t i = 0; i < count; i++)
    {
        int32_t length = std::rand() % (max_len - min_len);
        length += min_len;

        result.push_back(gen_string(length));
    }

    return result;
}

std::int32_t main(std::int32_t argc, char* argv[])
{
    std::srand(std::time(0));

    // if (argc != 4)
    // {
    //     std::cerr << "Usage: ./foo count min_lenght max_length\n";
    //     return 1;
    // }

    std::int32_t count = 1000;
    std::int32_t min_len = 100;
    std::int32_t max_len = 1000;

    std::vector<std::string> vec = gen_strings(count, min_len, max_len);

    for (const auto& str : vec)
    {
        std::cout << str.length() << "  ";
        std::string s = makeString_vsnprintf(str.c_str());
        std::cout << s.length() << "\n";
        // std::cout << s << "\n";
    }

    return 0;
}
