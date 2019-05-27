#ifndef _INC_SPDLOG_H
#define _INC_SPDLOG_H
#include <spdlog/spdlog.h>

class CSpdlog
{
public:
  CSpdlog(std::string const &name)
  {
    __logger = spdlog::get(name);
    if (!__logger)
    {
      __logger = spdlog::default_logger();
    }
  }

protected:
  template <typename... Args>
  void debug(Args &&... args)
  {
    if (__logger)
    {
      __logger->debug(std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void info(Args &&... args)
  {
    if (__logger)
    {
      __logger->info(std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void warn(Args &&... args)
  {
    if (__logger)
    {
      __logger->warn(std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void error(Args &&... args)
  {
    if (__logger)
    {
      __logger->error(std::forward<Args>(args)...);
    }
  }

private:
  std::shared_ptr<spdlog::logger> __logger;
};

#endif