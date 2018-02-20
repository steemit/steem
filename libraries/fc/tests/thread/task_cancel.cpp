#include <boost/test/unit_test.hpp>

#include <fc/thread/thread.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/exception/exception.hpp>
#include <fc/thread/non_preemptable_scope_check.hpp>

BOOST_AUTO_TEST_SUITE(fc_thread)

BOOST_AUTO_TEST_CASE( leave_mutex_locked )
{
  {
    fc::mutex test_mutex;
    fc::future<void> test_task = fc::async([&](){ 
      fc::scoped_lock<fc::mutex> test_lock(test_mutex); 
      for (int i = 0; i < 10; ++i) 
        fc::usleep(fc::seconds(1));
    }, "test_task");
    fc::usleep(fc::seconds(3));
    test_task.cancel_and_wait("cancel called directly by test");
  }
  BOOST_TEST_PASSPOINT();
}

BOOST_AUTO_TEST_CASE( cancel_task_blocked_on_mutex)
{
  {
    fc::mutex test_mutex;
    fc::future<void> test_task;
    {
      fc::scoped_lock<fc::mutex> test_lock(test_mutex); 
      test_task = fc::async([&test_mutex](){ 
        BOOST_TEST_MESSAGE("--- In test_task, locking mutex");
        fc::scoped_lock<fc::mutex> async_task_test_lock(test_mutex); 
        BOOST_TEST_MESSAGE("--- In test_task, mutex locked, commencing sleep");
        for (int i = 0; i < 10; ++i) 
          fc::usleep(fc::seconds(1));
        BOOST_TEST_MESSAGE("--- In test_task, sleeps done, exiting");
      }, "test_task");
      fc::usleep(fc::seconds(3));
      {
       fc::scoped_lock<fc::mutex> test_lock2(test_mutex); 
       test_task.cancel();
       try
       {
        test_task.wait(fc::seconds(1));
        BOOST_ERROR("test should have been canceled, not exited cleanly");
       }
       catch (const fc::canceled_exception&)
       {
        BOOST_TEST_PASSPOINT();
       }
       catch (const fc::timeout_exception&)
       {
        BOOST_ERROR("unable to cancel task blocked on mutex");
       }
      }
      BOOST_TEST_MESSAGE("Unlocking mutex locked from the main task so the test task will have the opportunity to lock it and be canceled");
    }
    fc::usleep(fc::seconds(3));

    test_task.cancel_and_wait("cleaning up test");
  }
}


BOOST_AUTO_TEST_CASE( test_non_preemptable_assertion )
{
  return; // this isn't a real test, because the thing it tries to test works by asserting, not by throwing
  fc::usleep(fc::milliseconds(10)); // this should not assert
  {
    ASSERT_TASK_NOT_PREEMPTED();
    fc::usleep(fc::seconds(1)); // this should assert
  }

  {
    ASSERT_TASK_NOT_PREEMPTED();
    {
      ASSERT_TASK_NOT_PREEMPTED();
      fc::usleep(fc::seconds(1)); // this should assert
    }
  }

  {
    ASSERT_TASK_NOT_PREEMPTED();
    {
      ASSERT_TASK_NOT_PREEMPTED();
      int i = 4;
      i += 2;
    }
    fc::usleep(fc::seconds(1)); // this should assert
  }

  fc::usleep(fc::seconds(1));  // this should not assert
  BOOST_TEST_PASSPOINT();
}

BOOST_AUTO_TEST_CASE( cancel_an_sleeping_task )
{
  enum task_result{sleep_completed, sleep_aborted};
  fc::future<task_result> task = fc::async([]() {
    BOOST_TEST_MESSAGE("Starting async task");
    try
    {
      fc::usleep(fc::seconds(5));
      return sleep_completed;
    }
    catch (const fc::exception&)
    {
      return sleep_aborted;
    }
  }, "test_task");

  fc::time_point start_time = fc::time_point::now();

  // wait a bit for the task to start running
  fc::usleep(fc::milliseconds(100));

  BOOST_TEST_MESSAGE("Canceling task");
  task.cancel("canceling to test if cancel works");
  try
  {
    task_result result = task.wait();
    BOOST_CHECK_MESSAGE(result != sleep_completed, "sleep should have been canceled");
  }
  catch (fc::exception& e)
  {
    BOOST_TEST_MESSAGE("Caught exception from canceled task: " << e.what());
    BOOST_CHECK_MESSAGE(fc::time_point::now() - start_time < fc::seconds(4), "Task was not canceled quickly");
  }
}

BOOST_AUTO_TEST_CASE( cancel_a_task_waiting_on_promise )
{
  enum task_result{task_completed, task_aborted};

  fc::promise<void>::ptr promise_to_wait_on(new fc::promise<void>());

  fc::future<task_result> task = fc::async([promise_to_wait_on]() {
    BOOST_TEST_MESSAGE("Starting async task");
    try
    {
      promise_to_wait_on->wait_until(fc::time_point::now() + fc::seconds(5));
      return task_completed;
    }
    catch (const fc::canceled_exception&)
    {
      BOOST_TEST_MESSAGE("Caught canceled_exception inside task-to-be-canceled");
      throw;
    }
    catch (const fc::timeout_exception&)
    {
      BOOST_TEST_MESSAGE("Caught timeout_exception inside task-to-be-canceled");
      throw;
    }
    catch (const fc::exception& e)
    {
      BOOST_TEST_MESSAGE("Caught unexpected exception inside task-to-be-canceled: " << e.to_detail_string());
      return task_aborted;
    }
  }, "test_task");

  fc::time_point start_time = fc::time_point::now();

  // wait a bit for the task to start running
  fc::usleep(fc::milliseconds(100));

  BOOST_TEST_MESSAGE("Canceling task");
  task.cancel("canceling to test if cancel works");
  //promise_to_wait_on->set_value();
  try
  {
    task_result result = task.wait();
    BOOST_CHECK_MESSAGE(result != task_completed, "task should have been canceled");
  }
  catch (fc::exception& e)
  {
    BOOST_TEST_MESSAGE("Caught exception from canceled task: " << e.what());
    BOOST_CHECK_MESSAGE(fc::time_point::now() - start_time < fc::seconds(4), "Task was not canceled quickly");
  }
}

BOOST_AUTO_TEST_CASE( cleanup_cancelled_task )
{
  std::shared_ptr<std::string> some_string(std::make_shared<std::string>("some string"));
  fc::future<void> task = fc::async([some_string]() {
    BOOST_TEST_MESSAGE("Starting async task, bound string is " << *some_string);
    try
    {
      fc::usleep(fc::seconds(5));
      BOOST_TEST_MESSAGE("Finsihed usleep in async task, leaving the task's functor");
    }
    catch (...)
    {
      BOOST_TEST_MESSAGE("Caught exception in async task, leaving the task's functor");
    }
  }, "test_task");
  std::weak_ptr<std::string> weak_string_ptr(some_string);
  some_string.reset();
  BOOST_CHECK_MESSAGE(!weak_string_ptr.expired(), "Weak pointer should still be valid because async task should be holding the strong pointer");
  fc::usleep(fc::milliseconds(100));
  BOOST_TEST_MESSAGE("Canceling task");
  task.cancel("canceling to test if cancel works");
  try
  {
    task.wait();
  }
  catch (fc::exception& e)
  {
    BOOST_TEST_MESSAGE("Caught exception from canceled task: " << e.what());
  }
//  BOOST_CHECK_MESSAGE(weak_string_ptr.expired(), "Weak pointer should now be invalid because async task should be done with it");
  task = fc::future<void>();
  BOOST_CHECK_MESSAGE(weak_string_ptr.expired(), "Weak pointer should now be invalid because async task should have been destroyed");
}

int task_execute_count = 0;
fc::future<void> simple_task_done;
void simple_task()
{
  task_execute_count++;
  simple_task_done = fc::schedule([](){ simple_task(); }, 
                                  fc::time_point::now() + fc::seconds(3),
                                  "simple_task");
}

BOOST_AUTO_TEST_CASE( cancel_scheduled_task )
{
  //bool task_executed = false;
  try 
  {
//    simple_task();
    simple_task();
    fc::usleep(fc::seconds(4));
    simple_task_done.cancel("canceling scheduled task to test if cancel works");
    simple_task_done.wait();
    BOOST_CHECK_EQUAL(task_execute_count, 2);
    fc::usleep(fc::seconds(3));
    BOOST_CHECK_EQUAL(task_execute_count, 2);
  } 
  catch ( const fc::exception& e )
  {
    wlog( "${e}", ("e",e.to_detail_string() ) );
  }
}

BOOST_AUTO_TEST_SUITE_END()
