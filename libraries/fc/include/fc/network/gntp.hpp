#pragma once
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/optional.hpp>
#include <fc/network/ip.hpp>

#include <memory>
#include <vector>

namespace fc
{
  namespace detail {
    class gntp_icon_impl;
  }
  class gntp_notifier;

  class gntp_icon {
  public:
    gntp_icon(const char* buffer, size_t length);
    ~gntp_icon();
  private:
    std::unique_ptr<detail::gntp_icon_impl> my;
    friend class gntp_notifier;
  };
  typedef std::shared_ptr<gntp_icon> gntp_icon_ptr;

  class gntp_notification_type {
  public:
    std::string   name;
    std::string   display_name;
    bool          enabled;
    gntp_icon_ptr icon;
  };
  typedef std::vector<gntp_notification_type> gntp_notification_type_list;

  namespace detail {
    class gntp_notifier_impl;
  }

  typedef uint160_t gntp_guid;

  class gntp_notifier {
  public:
    gntp_notifier(const std::string& host_to_notify = "127.0.0.1", uint16_t port = 23053,
                  const optional<std::string>& password = optional<std::string>());
    ~gntp_notifier();
    void set_application_name(std::string application_name);
    void set_application_icon(const gntp_icon_ptr& icon);
    void register_notifications();
    gntp_guid send_notification(std::string name, std::string title, std::string text, const gntp_icon_ptr& icon = gntp_icon_ptr(), optional<gntp_guid> coalescingId = optional<gntp_guid>());
    void add_notification_type(const gntp_notification_type& notificationType);
  private:
    std::unique_ptr<detail::gntp_notifier_impl> my;
  };


} // namespace fc
