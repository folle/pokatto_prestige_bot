#include "settings.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace {
  Settings::Threads ThreadStringToEnum(std::string const& thread) noexcept {
    if (thread == "fanarts") {
      return Settings::Threads::kSubmissionFanarts;
    }

    if (thread == "memes") {
      return Settings::Threads::kSubmissionMemes;
    }

    if (thread == "thumbnails") {
      return Settings::Threads::kSubmissionThumbnails;
    }

    if (thread == "video_edits") {
      return Settings::Threads::kSubmissionVideosEdits;
    }

    if (thread == "youtube_clips") {
      return Settings::Threads::kSubmissionYoutubeClips;
    }

    return Settings::Threads::kNone;
  }

  Settings::Rewards RewardStringToEnum(std::string const& reward) noexcept {
    if (reward == "special_discord_role") {
      return Settings::Rewards::kSpecialDiscordRole;
    }

    if (reward == "chaos_cat_doodle") {
      return Settings::Rewards::kChaosCatDoodle;
    }

    if (reward == "pokattosona") {
      return Settings::Rewards::kPokattosona;
    }

    if (reward == "vip_on_twitch") {
      return Settings::Rewards::kVipOnTwitch;
    }

    if (reward == "store_merch") {
      return Settings::Rewards::kStoreMerch;
    }

    if (reward == "clay_pokatto") {
      return Settings::Rewards::kClayPokatto;
    }

    return Settings::Rewards::kNone;
  }
}

Settings& Settings::Get() noexcept {
  static Settings settings;
  return settings;
}

Settings::Settings() {
  std::ifstream discord_settings_file("settings/settings.json");
  auto const discord_settings_json = nlohmann::json::parse(discord_settings_file);

  bot_token_ = discord_settings_json["bot_token"].get<std::string>();

  bot_user_id_ = discord_settings_json["bot_user_id"].get<dpp::snowflake>();

  squchan_user_id_ = discord_settings_json["squchan_user_id"].get<dpp::snowflake>();
  
  folle_user_id_ = discord_settings_json["folle_user_id"].get<dpp::snowflake>();

  server_id_ = discord_settings_json["server_id"].get<dpp::snowflake>();

  pokatto_prestige_path_channel_id_ = discord_settings_json["pokatto_prestige_path_channel_id"].get<dpp::snowflake>();

  auto const& submission_threads_ids_json = discord_settings_json["submission_threads_ids"];
  for (auto it_thread_id = submission_threads_ids_json.cbegin(); it_thread_id != submission_threads_ids_json.cend(); ++it_thread_id) {
    threads_ids_[::ThreadStringToEnum(it_thread_id.key())] = it_thread_id.value().get<dpp::snowflake>();
  }
  threads_ids_[Threads::kNone] = {};
  threads_ids_[Threads::kEnd] = {};

  auto const& rewards_price_json = discord_settings_json["rewards_prices"];
  for (auto it_reward_price = rewards_price_json.cbegin(); it_reward_price != rewards_price_json.cend(); ++it_reward_price) {
    rewards_prices_[::RewardStringToEnum(it_reward_price.key())] = it_reward_price.value().get<size_t>();
  }
  rewards_prices_[Rewards::kNone] = {};
  rewards_prices_[Rewards::kEnd] = {};
}

std::string const& Settings::GetBotToken() const noexcept {
  return bot_token_;
}

dpp::snowflake Settings::GetBotUserId() const noexcept {
  return bot_user_id_;
}

dpp::snowflake Settings::GetSquchanUserId() const noexcept {
  return squchan_user_id_;
}

dpp::snowflake Settings::GetFolleUserId() const noexcept {
  return folle_user_id_;
}

dpp::snowflake Settings::GetServerId() const noexcept {
  return server_id_;
}

dpp::snowflake Settings::GetPokattoPrestigePathChannelId() const noexcept {
  return pokatto_prestige_path_channel_id_;
}

dpp::snowflake Settings::GetThreadId(Threads const thread) const noexcept  {
  return threads_ids_.at(thread);
}

size_t Settings::GetRewardPrice(Rewards const reward) const noexcept {
  return rewards_prices_.at(reward);
}
