#pragma once

#include <cstdlib>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

#include <dpp/dpp.h>

#include "pokatto/pokatto_data.h"
#include "settings/settings.h"
#include "logger/logger_factory.h"

class PokattoPrestige final {
public:
  PokattoPrestige() = delete;
  ~PokattoPrestige() = default;

  PokattoPrestige(std::shared_ptr<dpp::cluster> bot);

  bool IsSubmissionMessage(dpp::snowflake channel_id) const noexcept;
  bool IsValidRating(dpp::snowflake user_id, std::string const& emoji_name) const noexcept;

  void AddRating(dpp::snowflake message_id, dpp::snowflake channel_id, std::string const& emoji_name) noexcept;

  void SendPointsHistory(dpp::snowflake user_id) noexcept;

  void ResyncMissedPoints() noexcept;

private:
  bool ResyncAllPoints() noexcept;

  bool HandleMonthChange() noexcept;

  bool ClearLeaderboardsMessages() const noexcept;
  bool UpdateLeaderboard() noexcept;

  bool ResyncThreadPoints(dpp::snowflake thread_id, bool skip_processed) noexcept;

  bool ProcessRating(dpp::snowflake message_id, dpp::snowflake channel_id, size_t rating) noexcept;
  bool ProcessRating(dpp::message const& message, size_t rating, bool skip_processed) noexcept;

  bool GetMonthAndYearFromTimestamp(std::time_t timestamp, int& month, int& year) const noexcept;

  bool GetReactionUsers(dpp::message const& message, std::string const& emoji_name, dpp::snowflake emoji_id, dpp::user_map& reaction_users) const noexcept;

  bool SendThreadPointsToUser(dpp::snowflake thread_id, dpp::snowflake user_id) const noexcept;
  bool SendDirectMessage(dpp::snowflake user_id, std::string const& message) const noexcept;

  bool ProcessLeaderboardEntries(std::string& leaderboard_message, std::queue<std::string>& leaderboard_entries) const noexcept;
  bool SendLeaderboardMessage(std::string const& leaderboard_message) const noexcept;

  void ClearSubmissionsFutures() noexcept;

  std::queue<std::string> GetSortedLeaderboardEntries(std::list<std::pair<dpp::snowflake, size_t>>& pokattos_points) const noexcept;

private:
  Logger const logger_ = LoggerFactory::Get().Create("Pokatto Prestige");
  
  std::shared_ptr<dpp::cluster> const bot_;

  std::map<std::string, size_t> const rating_emojis_ = {{"x_", 0}, {"1️⃣", 1}, {"2️⃣", 2}, {"3️⃣", 3}, {"4️⃣", 4},
                                                        {"5️⃣", 5}, {"6️⃣", 6}, {"7️⃣", 7}, {"8️⃣", 8}, {"9️⃣", 9}};

  std::mutex pokattos_mutex_;
  std::map<dpp::snowflake, PokattoData> pokattos_data_;
  std::list<std::pair<dpp::snowflake, size_t>> pokattos_total_points_;
  std::list<std::pair<dpp::snowflake, size_t>> pokattos_monthly_points_;

  int current_month_ = {};
  int current_year_ = {};

  std::mutex submissions_mutex_;
  std::list<std::future<void>> submissions_futures_;
};