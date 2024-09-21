#pragma once

#include <map>
#include <string>

#include <dpp/dpp.h>

#include "settings/settings.h"

class PokattoData final {
public:
  PokattoData() = delete;
  ~PokattoData() = default;

  PokattoData(dpp::snowflake user_id) noexcept;

  static std::map<dpp::snowflake, PokattoData> ReadPokattosData(bool& all_successful) noexcept;
  
  bool UnlockReward(Settings::Rewards reward) noexcept;
  bool IsRewardUnlocked(Settings::Rewards reward) noexcept;

private:
  bool StorePokattoData() noexcept;

private:
  dpp::snowflake const user_id_ = {};
  std::map<Settings::Rewards, bool> unlocked_rewards_;
};