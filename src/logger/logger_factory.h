#pragma once

#include <memory>
#include <string>
#include <vector>

#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "logger.h"

class LoggerFactory final {
public:
  static LoggerFactory& Get() noexcept;

  Logger Create(std::string const& name) const noexcept;

private:
  LoggerFactory() noexcept;
  ~LoggerFactory();

  LoggerFactory(LoggerFactory const&) = delete;
  void operator=(LoggerFactory const&) = delete;

private:
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> const stdout_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  std::shared_ptr<spdlog::sinks::daily_file_sink_mt> const file_sink_ = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/log.txt", 0, 0);
  std::vector<spdlog::sink_ptr> const sinks_;
};