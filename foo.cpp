#if !defined(__clang__) && defined(__GNUC__)
#include <malloc.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <vector>
#include <memory>
#include <string>
#include <exception>
#include <functional>
#include <iostream>
#include <chrono>
#include <locale>

int32_t g_malloc_cnt = 0;
int32_t g_free_cnt = 0;
int32_t g_realloc_cnt = 0;

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
    g_malloc_cnt++;

    setupHooks();

    return result;
}

static void my_free_hook (void *ptr, const void *caller)
{
    restoreHooks();

    free (ptr);
    g_free_cnt++;

    setupHooks();
}

static void *my_realloc_hook (void *ptr, size_t size, const void *caller)
{
    void *result;

    restoreHooks();

    result = realloc(ptr, size);
    g_realloc_cnt++;

    setupHooks();

    return result;
}
#endif

void init_cnt(void)
{
    g_malloc_cnt = 0;
    g_free_cnt = 0;
    g_realloc_cnt = 0;
}

void my_init(void)
{
#if !defined(__clang__) && defined(__GNUC__)
    original_malloc_hook = __malloc_hook;
    original_free_hook = __free_hook;
    original_realloc_hook = __realloc_hook;
#endif
}

static const char* get_cache_type_str(const int32_t type)
{
    switch (type)
    {
        case 1:
            return "Data Cache";
        case 2:
            return "Instruction Cache";
        case 3:
            return "Unified Cache";
        default:
            return "Unknown Type Cache";
    }
}

void i386_cpuid_caches (void)
{
    int i;
    for (i = 0; i < 32; i++) {

        // Variables to hold the contents of the 4 i386 legacy registers
        uint32_t eax, ebx, ecx, edx;

        eax = 4; // get cache info
        ecx = i; // cache id

        __asm__ (
            "cpuid" // call i386 cpuid instruction
            : "+a" (eax) // contains the cpuid command code, 4 for cache query
            , "=b" (ebx)
            , "+c" (ecx) // contains the cache id
            , "=d" (edx)
            ); // generates output in 4 registers eax, ebx, ecx and edx

        // See the page 3-191 of the manual.
        int cache_type = eax & 0x1F;

        if (cache_type == 0) // end of valid cache identifiers
            break;

        int cache_level = (eax >>= 5) & 0x7;

        int cache_is_self_initializing = (eax >>= 3) & 0x1; // does not need SW initialization
        int cache_is_fully_associative = (eax >>= 1) & 0x1;

        // See the page 3-192 of the manual.
        // ebx contains 3 integers of 10, 10 and 12 bits respectively
        unsigned int cache_sets = ecx + 1;
        unsigned int cache_coherency_line_size = (ebx & 0xFFF) + 1;
        unsigned int cache_physical_line_partitions = ((ebx >>= 12) & 0x3FF) + 1;
        unsigned int cache_ways_of_associativity = ((ebx >>= 10) & 0x3FF) + 1;

        // Total cache size is the product
        size_t cache_total_size = cache_ways_of_associativity * cache_physical_line_partitions * cache_coherency_line_size * cache_sets;

        printf(
            "Cache ID %d:\n"
            "- Level: %d\n"
            "- Type: %s\n"
            "- Sets: %d\n"
            "- System Coherency Line Size: %d bytes\n"
            "- Physical Line partitions: %d\n"
            "- Ways of associativity: %d\n"
            "- Total Size: %zu bytes (%zu kb)\n"
            "- Is fully associative: %s\n"
            "- Is Self Initializing: %s\n"
            "\n"
            , i
            , cache_level
            , get_cache_type_str(cache_type)
            , cache_sets
            , cache_coherency_line_size
            , cache_physical_line_partitions
            , cache_ways_of_associativity
            , cache_total_size, cache_total_size >> 10
            , cache_is_fully_associative ? "true" : "false"
            , cache_is_self_initializing ? "true" : "false"
            );
    }
}

static std::string makeString_vasprintf(const char *aFormat, ...)
{
    int32_t sLen;
    va_list sAp;

    if (aFormat == nullptr)
    {
        return std::string();
    }

    char* sBuffer;

    setupHooks();
    va_start(sAp, aFormat);
    sLen = vasprintf(&sBuffer, aFormat, sAp);
    va_end(sAp);

    if (sLen >= 0)
    {
        std::unique_ptr<char, std::function<void(void*)>> sOwner(sBuffer, [](void* aPtr){ free(aPtr); });
        restoreHooks();

        return std::string(sBuffer, sLen);
    }
    else
    {
        std::cout << aFormat << "\n";
        throw std::runtime_error("makeString_vasprintf(): vasprintf() returned subzero value. Possible memory deficit");
    }
}

static std::string makeString_vsnprintf(const char *aFormat, ...)
{
    int32_t sLen;
    va_list sAp;

    if (aFormat == nullptr)
    {
        return std::string();
    }

    setupHooks();
    va_start(sAp, aFormat);
    sLen = std::vsnprintf(nullptr, 0, aFormat, sAp);
    va_end(sAp);

    std::unique_ptr<char[]> sBuffer(new char[sLen + 1]);

    va_start(sAp, aFormat);
    (void)std::vsnprintf(sBuffer.get(), sLen + 1, aFormat, sAp);
    va_end(sAp);
    restoreHooks();

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

typedef std::string method_func(const char *aFormat, ...);

static void test(method_func* method, const char* name, const std::vector<std::string>& vec)
{
    init_cnt();

    std::cout << "Testing with method " << name;

    auto start_time = std::chrono::steady_clock::now();

    for (const auto& str : vec)
    {
        std::cerr << str.length() << "  ";
        std::string s = method(str.c_str());
        std::cerr << s.length() << "\n";
    }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    uint64_t elapsed_int = elapsed.count();
    std::cout << "    --> Took " << std::fixed << elapsed_int << "us ";
    std::cout << "(" << elapsed_int / 1000 << "ms)";
    std::cout << "    malloc: " << g_malloc_cnt << "    free: " << g_free_cnt << "    realloc: " << g_realloc_cnt << "\n";
}

int32_t main(int32_t argc, char* argv[])
{
    my_init();

    std::cout.imbue(std::locale(""));

    std::srand(std::time(0));

    if (argc != 4 && argc != 2)
    {
        std::cerr << "Usage: ./foo count min_lenght max_length\n";
        std::cerr << "       ./foo cpuinfo\n";
        return -1;
    }

    if (strcmp(argv[1], "cpuinfo") == 0)
    {
        i386_cpuid_caches();
        return 0;
    }

    int32_t count = std::atoi(argv[1]);
    int32_t min_len = std::atoi(argv[2]);
    int32_t max_len = std::atoi(argv[3]);

    if (min_len > max_len)
    {
        std::swap(min_len, max_len);
    }

    std::cout << "Generating " << count << " strings of length " << min_len << " ~ " << max_len << "\n";
    std::vector<std::string> vec = gen_strings(count, min_len, max_len);

    for (int32_t i = 0; i < 4; i++)
    {
        test(makeString_vasprintf, "vasprintf", vec);
        test(makeString_vsnprintf, "vsnprintf", vec);
    }

    return 0;
}
