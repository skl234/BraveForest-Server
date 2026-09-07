#include <iostream>
#include <functional>
#include <chrono>
#include <cstdint>
#include <iomanip>

#pragma warning(disable : 4324)

volatile std::uint64_t g_result = 0;

//----------------------------------------------------------
// Virtual 함수 테스트
//----------------------------------------------------------

class CVirtualBase
{
public:
    virtual ~CVirtualBase() = default;

    virtual std::uint64_t Execute(std::uint64_t _value) = 0;
};

class CVirtualDerived : public CVirtualBase
{
private:
    std::uint64_t m_value;

public:
    CVirtualDerived() :
        m_value(1)
    {
    }

    std::uint64_t Execute(std::uint64_t _value) override
    {
        m_value += _value;
        return m_value;
    }
};

//----------------------------------------------------------
// std::function 테스트
//----------------------------------------------------------

class CFunctionObject
{
private:
    std::uint64_t m_value;
    std::function<std::uint64_t(std::uint64_t)> m_function;

public:
    CFunctionObject() :
        m_value(1)
    {
        m_function =
            [this](std::uint64_t _value) -> std::uint64_t
        {
            m_value += _value;
            return m_value;
        };
    }

    std::uint64_t Execute(std::uint64_t _value)
    {
        return m_function(_value);
    }
};

//----------------------------------------------------------
// 측정 결과
//----------------------------------------------------------

struct BENCHMARK_RESULT
{
    double totalNanoseconds;
    double nanosecondsPerCall;
    std::uint64_t result;
};

//----------------------------------------------------------
// Virtual 측정
//----------------------------------------------------------

BENCHMARK_RESULT BenchmarkVirtual(std::uint64_t _loopCount)
{
    CVirtualDerived derived;

    // 반드시 부모 포인터를 통해 호출해야 실제 virtual dispatch가 발생한다.
    CVirtualBase* object = &derived;

    std::uint64_t result = 0;

    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (std::uint64_t i = 0; i < _loopCount; ++i)
    {
        result += object->Execute(i & 1);
    }

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();

    double elapsed =
        std::chrono::duration<double, std::nano>(
            end - start).count();

    g_result = result;

    BENCHMARK_RESULT benchmarkResult;
    benchmarkResult.totalNanoseconds = elapsed;
    benchmarkResult.nanosecondsPerCall =
        elapsed / static_cast<double>(_loopCount);
    benchmarkResult.result = result;

    return benchmarkResult;
}

//----------------------------------------------------------
// std::function 측정
//----------------------------------------------------------

BENCHMARK_RESULT BenchmarkStdFunction(std::uint64_t _loopCount)
{
    CFunctionObject object;

    std::uint64_t result = 0;

    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (std::uint64_t i = 0; i < _loopCount; ++i)
    {
        result += object.Execute(i & 1);
    }

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();

    double elapsed =
        std::chrono::duration<double, std::nano>(
            end - start).count();

    g_result = result;

    BENCHMARK_RESULT benchmarkResult;
    benchmarkResult.totalNanoseconds = elapsed;
    benchmarkResult.nanosecondsPerCall =
        elapsed / static_cast<double>(_loopCount);
    benchmarkResult.result = result;

    return benchmarkResult;
}

//----------------------------------------------------------
// 출력
//----------------------------------------------------------

void PrintResult(
    const char* _name,
    const BENCHMARK_RESULT& _result)
{
    double milliseconds =
        _result.totalNanoseconds / 1'000'000.0;

    std::cout
        << std::left
        << std::setw(20)
        << _name
        << " : "
        << std::right
        << std::fixed
        << std::setprecision(3)
        << std::setw(12)
        << milliseconds
        << " ms"
        << " | "
        << std::setw(8)
        << _result.nanosecondsPerCall
        << " ns/call"
        << '\n';
}

//----------------------------------------------------------
// main
//----------------------------------------------------------

int main()
{
    constexpr std::uint64_t LOOP_COUNT = 100'000'000;
    constexpr std::uint32_t TEST_COUNT = 10;

    std::cout << "Loop Count : "
        << LOOP_COUNT
        << '\n';

    std::cout << "Test Count : "
        << TEST_COUNT
        << "\n\n";

    double virtualTotal = 0.0;
    double functionTotal = 0.0;

    // 최초 실행 시 코드와 데이터가 캐시에 올라가도록 워밍업한다.
    BenchmarkVirtual(1'000'000);
    BenchmarkStdFunction(1'000'000);

    for (std::uint32_t i = 0; i < TEST_COUNT; ++i)
    {
        BENCHMARK_RESULT virtualResult;
        BENCHMARK_RESULT functionResult;

        /*
         * 실행 순서로 인한 영향을 조금 줄이기 위해
         * 홀수/짝수 테스트마다 순서를 뒤집는다.
         */
        if ((i & 1) == 0)
        {
            virtualResult =
                BenchmarkVirtual(LOOP_COUNT);

            functionResult =
                BenchmarkStdFunction(LOOP_COUNT);
        }
        else
        {
            functionResult =
                BenchmarkStdFunction(LOOP_COUNT);

            virtualResult =
                BenchmarkVirtual(LOOP_COUNT);
        }

        virtualTotal += virtualResult.nanosecondsPerCall;
        functionTotal += functionResult.nanosecondsPerCall;

        std::cout << "Test " << (i + 1) << '\n';

        PrintResult(
            "Virtual",
            virtualResult);

        PrintResult(
            "std::function",
            functionResult);

        std::cout << '\n';
    }

    double virtualAverage =
        virtualTotal / static_cast<double>(TEST_COUNT);

    double functionAverage =
        functionTotal / static_cast<double>(TEST_COUNT);

    std::cout << "---------------- Average ----------------\n";

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "Virtual      : "
        << virtualAverage
        << " ns/call\n";

    std::cout
        << "std::function: "
        << functionAverage
        << " ns/call\n";

    if (virtualAverage < functionAverage)
    {
        double ratio =
            functionAverage / virtualAverage;

        std::cout
            << "\nVirtual is approximately "
            << ratio
            << " times faster.\n";
    }
    else
    {
        double ratio =
            virtualAverage / functionAverage;

        std::cout
            << "\nstd::function is approximately "
            << ratio
            << " times faster.\n";
    }

    // 최적화 방지용 결과 출력
    std::cout << "Dummy Result : "
        << g_result
        << '\n';

    return 0;
}
