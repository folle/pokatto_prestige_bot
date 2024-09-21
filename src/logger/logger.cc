#include "logger.h"

#include <utility>

Logger::Logger(std::shared_ptr<spdlog::async_logger>&& logger) noexcept
  : logger_(std::move(logger)) {

}

Logger::~Logger() {
  logger_->flush();
}