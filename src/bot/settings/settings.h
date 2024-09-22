#pragma once

#include <cstdlib>
#include <map>
#include <string>

#include <dpp/dpp.h>

class Settings final {
public:
  enum class Threads : size_t {
    kNone = 0,
    kBegin = 1,
    kSubmissionFanarts = 1,
    kSubmissionMemes,
    kSubmissionThumbnails,
    kSubmissionVideosEdits,
    kSubmissionYoutubeClips,
    kEnd
  };

  enum class Rewards : size_t {
    kNone = 0,
    kBegin = 1,
    kSpecialDiscordRole = 1,
    kChaosCatDoodle,
    kPokattosona,
    kVipOnTwitch,
    kStoreMerch,
    kClayPokatto,
    kEnd
  };

  static Settings& Get() noexcept;

  std::string const& GetBotToken() const noexcept;

  dpp::snowflake GetBotUserId() const noexcept;
  dpp::snowflake GetSquchanUserId() const noexcept;
  dpp::snowflake GetFolleUserId() const noexcept;
  dpp::snowflake GetServerId() const noexcept;
  dpp::snowflake GetPokattoPrestigePathChannelId() const noexcept;
  dpp::snowflake GetThreadId(Threads const thread) const noexcept;

  size_t GetRewardPrice(Rewards const reward) const noexcept;

private:
  Settings();
  ~Settings() = default;

  Settings(Settings const&) = delete;
  void operator=(Settings const&) = delete;

private:
  std::string bot_token_;

  dpp::snowflake server_id_ = {};
  dpp::snowflake bot_user_id_ = {};
  dpp::snowflake squchan_user_id_ = {};
  dpp::snowflake folle_user_id_ = {};
  dpp::snowflake pokatto_prestige_path_channel_id_ = {};

  std::map<Threads, dpp::snowflake> threads_ids_;

  std::map<Rewards, size_t> rewards_prices_;
};