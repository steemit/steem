#include <boost/test/unit_test.hpp>

#include <fc/thread/thread.hpp>

using namespace fc;

BOOST_AUTO_TEST_SUITE(thread_tests)

BOOST_AUTO_TEST_CASE(executes_task)
{
    bool called = false;
    fc::thread thread("my");
    thread.async([&called]{called = true;}).wait();
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(returns_value_from_function)
{
    fc::thread thread("my");
    BOOST_CHECK_EQUAL(10, thread.async([]{return 10;}).wait());
}

BOOST_AUTO_TEST_CASE(executes_multiple_tasks)
{
    bool called1 = false;
    bool called2 = false;

    fc::thread thread("my");
    auto future1 = thread.async([&called1]{called1 = true;});
    auto future2 = thread.async([&called2]{called2 = true;});

    future2.wait();
    future1.wait();

    BOOST_CHECK(called1);
    BOOST_CHECK(called2);
}

BOOST_AUTO_TEST_CASE(calls_tasks_in_order)
{
    std::string result;

    fc::thread thread("my");
    auto future1 = thread.async([&result]{result += "hello ";});
    auto future2 = thread.async([&result]{result += "world";});

    future2.wait();
    future1.wait();

    BOOST_CHECK_EQUAL("hello world", result);
}

BOOST_AUTO_TEST_CASE(yields_execution)
{
    std::string result;

    fc::thread thread("my");
    auto future1 = thread.async([&result]{fc::yield(); result += "world";});
    auto future2 = thread.async([&result]{result += "hello ";});

    future2.wait();
    future1.wait();

    BOOST_CHECK_EQUAL("hello world", result);
}

BOOST_AUTO_TEST_CASE(quits_infinite_loop)
{
    fc::thread thread("my");
    auto f = thread.async([]{while (true) fc::yield();});

    thread.quit();
    BOOST_CHECK_THROW(f.wait(), fc::canceled_exception);
}

BOOST_AUTO_TEST_CASE(reschedules_yielded_task)
{
    int reschedule_count = 0;

    fc::thread thread("my");
    auto future = thread.async([&reschedule_count]
            {
                while (reschedule_count < 10)
                {
                    fc::yield();
                    reschedule_count++;
                }
            });

    future.wait();
    BOOST_CHECK_EQUAL(10, reschedule_count);
}

BOOST_AUTO_TEST_SUITE_END()
