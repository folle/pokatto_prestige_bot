#include "pokatto_data.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <utility>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace {
  auto constexpr kDataDirectory = "data";
  auto constexpr kUnlockedRewardsKey = "unlocked_rewards";
  auto constexpr kSpecialDiscordRoleKey = "special_discord_role";
  auto constexpr kChaosCatDoodleKey = "chaos_cat_doodle";
  auto constexpr kPokattosonaKey = "pokattosona";
  auto constexpr kVipOnTwitchKey = "vip_on_twitch";
  auto constexpr kStoreMerchKey = "store_merch";
  auto constexpr kClayPokattoKey = "clay_pokatto";
}

PokattoData::PokattoData(dpp::snowflake const user_id) noexcept :
  user_id_(user_id) {

}

std::map<dpp::snowflake, PokattoData> PokattoData::ReadPokattosData(bool& all_successful) noexcept {
  all_successful = true;
  
  try {
    if (!std::filesystem::exists(kDataDirectory)) {
      if (!std::filesystem::create_directory(kDataDirectory)) {
        all_successful = false;
      }

      return {};
    }
  } catch (std::exception const& exception) {
    all_successful = false;
    return {};
  }

  std::map<dpp::snowflake, PokattoData> pokattos_data;
  for (auto const& directory_entry : std::filesystem::directory_iterator(kDataDirectory)) {
    auto const& file_path = directory_entry.path();

    dpp::snowflake user_id{};
    try {
      user_id = std::stoull(file_path.stem().string());
    } catch (std::exception const& rest_exception) {
      all_successful = false;
      return {};
    }

    std::map<Settings::Rewards, bool> unlocked_rewards;
    std::ifstream pokatto_data_file(file_path);
    try {
      auto const pokatto_data_json = nlohmann::json::parse(pokatto_data_file);
      auto const& unlocked_rewards_json = pokatto_data_json[kUnlockedRewardsKey];

      unlocked_rewards.emplace(Settings::Rewards::kSpecialDiscordRole, unlocked_rewards_json[kSpecialDiscordRoleKey]);
      unlocked_rewards.emplace(Settings::Rewards::kChaosCatDoodle, unlocked_rewards_json[kChaosCatDoodleKey]);
      unlocked_rewards.emplace(Settings::Rewards::kPokattosona, unlocked_rewards_json[kPokattosonaKey]);
      unlocked_rewards.emplace(Settings::Rewards::kVipOnTwitch, unlocked_rewards_json[kVipOnTwitchKey]);
      unlocked_rewards.emplace(Settings::Rewards::kStoreMerch, unlocked_rewards_json[kStoreMerchKey]);
      unlocked_rewards.emplace(Settings::Rewards::kClayPokatto, unlocked_rewards_json[kClayPokattoKey]);
    } catch (std::exception const& rest_exception) {
      all_successful = false;
      return {};
    }

    PokattoData pokatto_data(user_id);
    pokatto_data.unlocked_rewards_ = std::move(unlocked_rewards);

    pokattos_data.emplace(user_id, std::move(pokatto_data));
  }

  return pokattos_data;
}

bool PokattoData::UnlockReward(Settings::Rewards reward) noexcept {
  unlocked_rewards_[reward] = true;
  return StorePokattoData();
}

bool PokattoData::IsRewardUnlocked(Settings::Rewards reward) noexcept {
  return unlocked_rewards_[reward];
}

bool PokattoData::StorePokattoData() noexcept {
  nlohmann::json pokatto_data_json;

  auto& unlocked_rewards_json = pokatto_data_json[kUnlockedRewardsKey];
  unlocked_rewards_json[kSpecialDiscordRoleKey] = IsRewardUnlocked(Settings::Rewards::kSpecialDiscordRole);
  unlocked_rewards_json[kChaosCatDoodleKey] = IsRewardUnlocked(Settings::Rewards::kChaosCatDoodle);
  unlocked_rewards_json[kPokattosonaKey] = IsRewardUnlocked(Settings::Rewards::kPokattosona);
  unlocked_rewards_json[kVipOnTwitchKey] = IsRewardUnlocked(Settings::Rewards::kVipOnTwitch);
  unlocked_rewards_json[kStoreMerchKey] = IsRewardUnlocked(Settings::Rewards::kStoreMerch);
  unlocked_rewards_json[kClayPokattoKey] = IsRewardUnlocked(Settings::Rewards::kClayPokatto);

  auto const file_path = fmt::format("{}/{}.json", kDataDirectory, user_id_);
  std::ofstream output_file(file_path, std::ios_base::out | std::ios_base::trunc);
  try {
    output_file << pokatto_data_json;
  } catch (std::exception const& exception) {
    return false;
  }

  return output_file.good();
}