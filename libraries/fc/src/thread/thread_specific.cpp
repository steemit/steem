#include <fc/log/logger.hpp>
#include <fc/thread/thread_specific.hpp>
#include "thread_d.hpp"
#include <boost/atomic.hpp>

namespace fc
{
  namespace detail
  {
    boost::atomic<unsigned> thread_specific_slot_counter;
    unsigned get_next_unused_thread_storage_slot()
    {
      return thread_specific_slot_counter.fetch_add(1);
    }

    void* get_specific_data(std::vector<detail::specific_data_info> *specific_data, unsigned slot)
    {
      return slot < specific_data->size() ?
        (*specific_data)[slot].value : nullptr;
    }
    void set_specific_data(std::vector<detail::specific_data_info> *specific_data, unsigned slot, void* new_value, void(*cleanup)(void*))
    {
      if (slot + 1 > specific_data->size())
        specific_data->resize(slot + 1);
      (*specific_data)[slot] = detail::specific_data_info(new_value, cleanup);
    }

    void* get_thread_specific_data(unsigned slot)
    {
      return get_specific_data(&thread::current().my->thread_specific_data, slot);
    }
    void set_thread_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*))
    {
      return set_specific_data(&thread::current().my->thread_specific_data, slot, new_value, cleanup);
    }

    unsigned get_next_unused_task_storage_slot()
    {
      return thread::current().my->next_unused_task_storage_slot++;
    }
    void* get_task_specific_data(unsigned slot)
    {
      context* current_context = thread::current().my->current;
      if (!current_context ||
          !current_context->cur_task)
        return get_specific_data(&thread::current().my->non_task_specific_data, slot);
      if (current_context->cur_task->_task_specific_data)
        return get_specific_data(current_context->cur_task->_task_specific_data, slot);
      return nullptr;
    }
    void set_task_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*))
    {
      context* current_context = thread::current().my->current;
      if (!current_context ||
          !current_context->cur_task)
        set_specific_data(&thread::current().my->non_task_specific_data, slot, new_value, cleanup);
      else
      {
        if (!current_context->cur_task->_task_specific_data)
            current_context->cur_task->_task_specific_data = new std::vector<detail::specific_data_info>;
        set_specific_data(current_context->cur_task->_task_specific_data, slot, new_value, cleanup);
      }
    }
  }
} // end namespace fc
