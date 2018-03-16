#pragma once
#include "thread.hpp"

namespace fc
{
  namespace detail
  {
    unsigned get_next_unused_thread_storage_slot();
    unsigned get_next_unused_task_storage_slot();
  }

  template <typename T>
  class thread_specific_ptr
  {
  private:
    unsigned slot;
  public:
    thread_specific_ptr() :
      slot(detail::get_next_unused_thread_storage_slot())
      {}

    T* get() const
    {
      return static_cast<T*>(detail::get_thread_specific_data(slot));
    }
    T* operator->() const
    {
      return get();
    }
    T& operator*() const
    {
      return *get();
    }
    operator bool() const
    {
      return get() != static_cast<T*>(0);
    }
    static void cleanup_function(void* obj) 
    {
      delete static_cast<T*>(obj);
    }
    void reset(T* new_value = 0)
    {
      detail::set_thread_specific_data(slot, new_value, cleanup_function);
    }
  };

  template <typename T>
  class task_specific_ptr
  {
  private:
    unsigned slot;
  public:
    task_specific_ptr() :
      slot(detail::get_next_unused_task_storage_slot())
      {}

    T* get() const
    {
      return static_cast<T*>(detail::get_task_specific_data(slot));
    }
    T* operator->() const
    {
      return get();
    }
    T& operator*() const
    {
      return *get();
    }
    operator bool() const
    {
      return get() != static_cast<T*>(0);
    }
    static void cleanup_function(void* obj) 
    {
      delete static_cast<T*>(obj);
    }
    void reset(T* new_value = 0)
    {
      detail::set_task_specific_data(slot, new_value, cleanup_function);
    }
  };

}
