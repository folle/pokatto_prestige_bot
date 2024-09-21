#include "pokatto_prestige.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <limits>
#include <utility>

#include <fmt/format.h>

namespace {
  auto constexpr kMaxMessageLength = 2000ULL;
  auto constexpr kMaxMessagesPerGetCall = 100ULL;
  auto constexpr kProcessedMessageEmoji = "✅";

  std::string GetRewardString(Settings::Rewards const reward) noexcept {
    switch (reward) {
      case Settings::Rewards::kSpecialDiscordRole: { return "Special Discord Role"; }
      case Settings::Rewards::kChaosCatDoodle: { return "Chaos Cat Doodle"; }
      case Settings::Rewards::kPokattosona: { return "Pokattosona"; }
      case Settings::Rewards::kVipOnTwitch: { return "Vip on Twitch"; }
      case Settings::Rewards::kStoreMerch: { return "Store Merch"; }
      case Settings::Rewards::kClayPokatto: { return "Clay Pokatto"; }
      default: { return {}; }
    }
  }

  std::string GetThreadString(dpp::snowflake const thread_id) noexcept {
    if (Settings::Get().GetThreadId(Settings::Threads::kSubmissionFanarts) == thread_id) {
      return "Fanarts & Bootifur Creations";
    }

    if (Settings::Get().GetThreadId(Settings::Threads::kSubmissionMemes) == thread_id) {
      return "Memes";
    }

    if (Settings::Get().GetThreadId(Settings::Threads::kSubmissionThumbnails) == thread_id) {
      return "Thumbnails";
    }

    if (Settings::Get().GetThreadId(Settings::Threads::kSubmissionVideosEdits) == thread_id) {
      return "Video Edits";
    }
    
    if (Settings::Get().GetThreadId(Settings::Threads::kSubmissionYoutubeClips) == thread_id) {
      return "Youtube Clips";
    }

    return {};
  }
  
  void AppendMessageContent(std::string& message, std::queue<std::string>& content) noexcept {
    while (!content.empty() && (message.length() + content.front().length()) <= kMaxMessageLength) {
      message.append(content.front());
      content.pop();
    }
  }

  std::queue<std::string> GetSortedLeaderboardEntries(std::list<std::pair<dpp::snowflake, size_t>>& pokattos_points) noexcept {
    pokattos_points.sort([](std::pair<dpp::snowflake, size_t> const& lhs, std::pair<dpp::snowflake, size_t> const& rhs)
                         { return lhs.second < rhs.second; });

    std::queue<std::string> leaderboard_entries;
    std::for_each(pokattos_points.cbegin(), pokattos_points.cend(),
                  [&leaderboard_entries](std::pair<dpp::snowflake, size_t> const& rank_entry) { 
                    auto const leaderboard_string = fmt::format("{} point{}: {}\n",
                                                            rank_entry.second,
                                                            (rank_entry.second == 1) ? "" : "s",
                                                            dpp::user::get_mention(rank_entry.first));
                    leaderboard_entries.push(leaderboard_string);
                  });
        
    return leaderboard_entries;
  }

  std::string GetMonthString(int const month) {
    switch (month) {
      case 0: { return "January"; }
      case 1: { return "February"; }
      case 2: { return "March"; }
      case 3: { return "April"; }
      case 4: { return "May"; }
      case 5: { return "June"; }
      case 6: { return "July"; }
      case 7: { return "August"; }
      case 8: { return "September"; }
      case 9: { return "October"; }
      case 10: { return "November"; }
      case 11: { return "December"; }
      default: { return {}; }
    }
  }

  size_t IncrementLeaderboard(std::list<std::pair<dpp::snowflake, size_t>>& pokattos_points, dpp::snowflake const user_id, size_t const rating) noexcept {
    size_t points{};
    auto it_pokatto_points = std::find_if(pokattos_points.begin(), pokattos_points.end(),
                                          [user_id](std::pair<dpp::snowflake, size_t> const& pokatto_points){ return pokatto_points.first == user_id; });
    if (it_pokatto_points == pokattos_points.cend()) {
      pokattos_points.emplace_back(std::make_pair(user_id, rating));
      points = rating;
    } else {
      it_pokatto_points->second += rating;
      points = it_pokatto_points->second ;
    }

    return points;
  }
}

PokattoPrestige::PokattoPrestige(std::shared_ptr<dpp::cluster> bot) noexcept :
  bot_(std::move(bot)) {
  pokattos_data_ = PokattoData::ReadPokattosData(successfully_initialized_);
  if (!successfully_initialized_) {
    logger_.Error("Error while reading pokattos data");
    return;
  }

  logger_.Info("Successfully read pokattos data");
}

bool PokattoPrestige::SuccessfullyInitialized() const noexcept {
  return successfully_initialized_;
}

bool PokattoPrestige::IsSubmissionMessage(dpp::snowflake const channel_id) const noexcept {
  return (Settings::Get().GetThreadId(Settings::Threads::kSubmissionFanarts) == channel_id) ||
         (Settings::Get().GetThreadId(Settings::Threads::kSubmissionMemes) == channel_id) ||
         (Settings::Get().GetThreadId(Settings::Threads::kSubmissionThumbnails) == channel_id) ||
         (Settings::Get().GetThreadId(Settings::Threads::kSubmissionVideosEdits) == channel_id) ||
         (Settings::Get().GetThreadId(Settings::Threads::kSubmissionYoutubeClips) == channel_id);
}

bool PokattoPrestige::IsValidRating(dpp::snowflake const user_id, std::string const& emoji_name) const noexcept {
  return (Settings::Get().GetSquchanUserId() == user_id) && rating_emojis_.contains(emoji_name);
}

void PokattoPrestige::AddRating(dpp::snowflake const message_id, dpp::snowflake const channel_id, std::string const& emoji_name) {
  std::scoped_lock<std::mutex> const mutex_lock(submissions_mutex_);

  ClearSubmissionsFutures();
  
  auto const& rating = rating_emojis_.at(emoji_name);
  submissions_futures_.push_back(
    std::async([this, channel_id, message_id, rating]{
      logger_.Info("Adding rating. Message id: '{}'. Rating: '{}'", message_id, rating);

      ProcessRating(message_id, channel_id, rating);

      if (0 < rating) {
        UpdateLeaderboard();
      }

      logger_.Info("Finished adding rating. Message id: '{}'. Rating: '{}'", message_id, rating);
    })
  );
}

void PokattoPrestige::SendPointsHistory(dpp::snowflake const user_id) {
  std::scoped_lock<std::mutex> const mutex_lock(submissions_mutex_);

  ClearSubmissionsFutures();

  submissions_futures_.push_back(
    std::async([this, user_id]{
      logger_.Info("Sending points history. User id: '{}'", user_id);

      for (size_t thread = static_cast<size_t>(Settings::Threads::kBegin); thread < static_cast<size_t>(Settings::Threads::kEnd); ++thread) {
        auto const thread_id = Settings::Get().GetThreadId(static_cast<Settings::Threads>(thread));
        SendThreadPointsToUser(thread_id, user_id);
      }

      logger_.Info("Finished sending points history. User id: '{}'", user_id);
    })
  );
}

void PokattoPrestige::ResyncAllPoints() {
  std::scoped_lock<std::mutex> const mutex_lock(submissions_mutex_);
  submissions_futures_.push_back(
    std::async([this]{
      logger_.Info("Resyncing all points");

      for (size_t thread = static_cast<size_t>(Settings::Threads::kBegin); thread < static_cast<size_t>(Settings::Threads::kEnd); ++thread) {
        auto const thread_id = Settings::Get().GetThreadId(static_cast<Settings::Threads>(thread));
        ResyncThreadPoints(thread_id, false);
      }

      UpdateLeaderboard();

      logger_.Info("Finished resyncing all points");
    })
  );
}

void PokattoPrestige::ResyncMissedPoints() {
  std::scoped_lock<std::mutex> const mutex_lock(submissions_mutex_);

  ClearSubmissionsFutures();

  submissions_futures_.push_back(
    std::async([this]{
      logger_.Info("Resyncing missed points");

      for (size_t thread = static_cast<size_t>(Settings::Threads::kBegin); thread < static_cast<size_t>(Settings::Threads::kEnd); ++thread) {
        auto const thread_id = Settings::Get().GetThreadId(static_cast<Settings::Threads>(thread));
        ResyncThreadPoints(thread_id, true);
      }

      UpdateLeaderboard();

      logger_.Info("Finished resyncing missed points");
    })
  );
}

void PokattoPrestige::HandleMonthChange() {
  auto const current_time = std::time(nullptr);
  int month{};
  int year{};
  GetMonthAndYearFromTimestamp(current_time, month, year);

  std::scoped_lock<std::mutex> const mutex_lock(pokattos_mutex_);
  if ((month != current_month_) || (year != current_year_)) {
    logger_.Info("Monthly leaderboard has been reseted");

    pokattos_monthly_points_.clear();
    
    current_month_ = month;
    current_year_ = year;
  }
}

void PokattoPrestige::ClearLeaderboardsMessages() const {
  logger_.Info("Clearing leaderboard messages");

  dpp::snowflake latest_message_id{};
  dpp::message_map messages;
  do {
    try {
      messages = bot_->messages_get_sync(Settings::Get().GetPokattoPrestigePathChannelId(), {}, {}, latest_message_id, kMaxMessagesPerGetCall);
    } catch (dpp::exception const& rest_exception) {
      logger_.Error("Failed to get leaderboard messages. Exception '{}'", rest_exception.what());
    }

    for (auto const& [message_id, message] : messages) {
      if (message_id > latest_message_id) {
        latest_message_id = message_id;
      }

      if (bot_->me.id != message.author.id) {
        continue;
      }

      try {
        auto const delete_confirmation = bot_->message_delete_sync(message_id, Settings::Get().GetPokattoPrestigePathChannelId());
        if (!delete_confirmation.success) {
          logger_.Error("Failed to delete leaderboard message.");
        }
      } catch (dpp::exception const& rest_exception) {
        logger_.Error("Failed to delete leaderboard message. Exception: '{}'", rest_exception.what());
      }
    }
  } while (!messages.empty());

  logger_.Info("Finished clearing leaderboard messages");
}

void PokattoPrestige::UpdateLeaderboard() {
  ClearLeaderboardsMessages();
  
  auto leaderboard_entries = ::GetSortedLeaderboardEntries(pokattos_total_points_);
  std::string leaderboard_message;
  leaderboard_message.reserve(kMaxMessageLength);
  leaderboard_message = "**Full Pokatto Prestige Leaderboard:**\n";
  ProcessLeaderboardEntries(leaderboard_message, leaderboard_entries);

  auto monthly_leaderboard_entries = GetSortedLeaderboardEntries(pokattos_monthly_points_);
  std::string monthly_leaderboard_message;
  monthly_leaderboard_message.reserve(kMaxMessageLength);
  monthly_leaderboard_message = fmt::format("**Monthly Pokatto Prestige Leaderboard - {}:**\n", ::GetMonthString(current_month_));
  ProcessLeaderboardEntries(monthly_leaderboard_message, monthly_leaderboard_entries);
}

void PokattoPrestige::ResyncThreadPoints(dpp::snowflake const thread_id, bool const skip_processed) {
  dpp::snowflake latest_message_id{};
  dpp::message_map messages;
  do {
    try {
      messages = bot_->messages_get_sync(thread_id, {}, {}, latest_message_id, kMaxMessagesPerGetCall);
    } catch (dpp::exception const& rest_exception) {
      logger_.Error("Failed to get submission messages. Thread id: '{}'. Exception '{}'", thread_id, rest_exception.what());
    }

    for (auto const& [message_id, message] : messages) {
      if (message_id > latest_message_id) {
        latest_message_id = message_id;
      }

      auto const it_rating_reaction = std::find_if(message.reactions.cbegin(), message.reactions.cend(),
                                                   [this](auto const& reaction){ return rating_emojis_.contains(reaction.emoji_name); });
      if (message.reactions.cend() == it_rating_reaction) {
        continue;
      }
      auto const rating_reaction_users = GetReactionUsers(message, it_rating_reaction->emoji_name, it_rating_reaction->emoji_id);
      auto const has_squchan_reacted = std::any_of(rating_reaction_users.cbegin(), rating_reaction_users.cend(),
                                                   [](auto const& user){ return Settings::Get().GetSquchanUserId() == user.first; });
      if (!has_squchan_reacted) {
        continue;
      }

      auto const rating = rating_emojis_.at(it_rating_reaction->emoji_name);
      ProcessRating(message, rating, skip_processed);
    }
  } while (!messages.empty());
}

void PokattoPrestige::ProcessRating(dpp::snowflake const message_id, dpp::snowflake const channel_id, size_t const rating) {  
  dpp::message message;
  try {
    message = bot_->message_get_sync(message_id, channel_id);
  } catch (dpp::exception const& rest_exception) {
    logger_.Error("Failed to get submission message. Message id: '{}'. Channel id: '{}'. Exception '{}'", message_id, channel_id, rest_exception.what());
  }

  ProcessRating(message, rating, true);
}

void PokattoPrestige::ProcessRating(dpp::message const& message, size_t const rating, bool const skip_processed) {
  auto const& user_id = message.author.id;
  auto const& username = message.author.username;

  logger_.Info("Processing rating. Message id: '{}'. Rating: '{}'. User id: '{}'", message.id, rating, user_id);

  HandleMonthChange();

  std::scoped_lock<std::mutex> const mutex_lock(pokattos_mutex_);

  auto const processed_reaction_users = GetReactionUsers(message, kProcessedMessageEmoji);
  auto const processed = std::any_of(processed_reaction_users.cbegin(), processed_reaction_users.cend(),
                                      [this](auto const& user) { return bot_->me.id == user.first; });
  if (processed && skip_processed) {
    logger_.Info("Rating skipped. Message id: '{}'. Rating: '{}'. User id: '{}'. Processed: '{}'. Skip Processed: '{}'",
                  message.id, rating, user_id, processed, skip_processed);
    return;
  }

  auto const total_points = ::IncrementLeaderboard(pokattos_total_points_, user_id, rating);

  int month{};
  int year{};
  auto const timestamp = static_cast<std::time_t>(message.get_creation_time());
  GetMonthAndYearFromTimestamp(timestamp, month, year);
  if ((month == current_month_) && (year == current_year_)) {
    logger_.Info("Rating is from current month. Message id: '{}'. Rating: '{}'. User id: '{}'. Month: '{}'. Year: '{}'",
                  message.id, rating, user_id, month, year);

    ::IncrementLeaderboard(pokattos_monthly_points_, user_id, rating);
  }

  if (!pokattos_data_.contains(user_id)) {
    logger_.Info("User's first entry. Message id: '{}'. Rating: '{}'. User id: '{}'.", message.id, rating, user_id);

    pokattos_data_.emplace(user_id, PokattoData(user_id));
  }

  for (size_t reward = static_cast<size_t>(Settings::Rewards::kBegin); reward < static_cast<size_t>(Settings::Rewards::kEnd); ++reward) {
    auto const reward_class = static_cast<Settings::Rewards>(reward);
    if ((Settings::Get().GetRewardPrice(reward_class) > total_points) ||
        (pokattos_data_.at(user_id).IsRewardUnlocked(reward_class))) {
      continue;
    }

    auto const reward_string = ::GetRewardString(reward_class);
    logger_.Info("User unlocked reward. Message id: '{}'. Rating: '{}'. User id: '{}'. Reward: '{}'",
                  message.id, rating, user_id, reward_string);

    auto const reward_unlocked_message = fmt::format("User {} has unlocked **{}**", dpp::user::get_mention(user_id), reward_string);
    SendDirectMessage(Settings::Get().GetSquchanUserId(), reward_unlocked_message);

    auto const user_reward_unlocked_message = fmt::format("You have unlocked **{}**", reward_string);
    SendDirectMessage(user_id, user_reward_unlocked_message);

    pokattos_data_.at(user_id).UnlockReward(reward_class);
  }

  try {
    auto const add_reaction_confirmation = bot_->message_add_reaction_sync(message, kProcessedMessageEmoji);
    if (!add_reaction_confirmation.success) {
      logger_.Error("Failed to add processed reaction. Message id: '{}'. User id: '{}'", message.id, user_id);
    }
  }
  catch (dpp::exception const& rest_exception) {
    logger_.Error("Failed to add processed reaction. Message id: '{}'. User id: '{}'. Exception '{}'", message.id, user_id, rest_exception.what());
  }

  logger_.Info("Finished processing rating. Message id: '{}'. Rating: '{}'", message.id, rating);
}

void PokattoPrestige::GetMonthAndYearFromTimestamp(std::time_t const timestamp, int& month, int& year) const {
  auto const local_time = std::localtime(&timestamp);
  if (nullptr == local_time) {
    logger_.Error("Failed to get month and year from timestamp. Timestamp: '{}'", timestamp);
    return;
  }

  month = local_time->tm_mon;
  year = local_time->tm_year;
}

dpp::user_map PokattoPrestige::GetReactionUsers(dpp::message const& message, std::string const& emoji_name, dpp::snowflake const emoji_id) const {
  dpp::user_map reaction_users;
  try {
    auto const reaction = (emoji_id > 0) ? fmt::format("{}:{}", emoji_name, emoji_id) : emoji_name;
    reaction_users = bot_->message_get_reactions_sync(message, reaction, {}, {}, std::numeric_limits<dpp::snowflake>::max());
  } catch (dpp::exception const& rest_exception) {
    logger_.Error("Failed to get message reactions. Message id: '{}'. Emoji name: '{}'. Exception '{}'", message.id, emoji_name, rest_exception.what());
  }

  return reaction_users;
}

void PokattoPrestige::SendThreadPointsToUser(dpp::snowflake thread_id, dpp::snowflake user_id) const {
  dpp::snowflake latest_message_id{};
  dpp::message_map messages;
  size_t total_user_points_in_thread{};
  std::queue<std::string> submissions_info;
  do {
    try {
      messages = bot_->messages_get_sync(thread_id, {}, {}, latest_message_id, kMaxMessagesPerGetCall);
    } catch (dpp::exception const& rest_exception) {
      logger_.Error("Failed to get submission thread messages. Thread id: '{}'. Exception '{}'", thread_id, rest_exception.what());
    }

    for (auto const& [message_id, message] : messages) {
      if (message_id > latest_message_id) {
        latest_message_id = message_id;
      }

      if (message.author.id != user_id) {
        continue;
      }

      auto const message_url = fmt::format("https://discord.com/channels/{}/{}/{}", Settings::Get().GetServerId(), thread_id, message_id);

      auto const it_rating_reaction = std::find_if(message.reactions.cbegin(), message.reactions.cend(),
                                                   [this](auto const& reaction){ return rating_emojis_.contains(reaction.emoji_name); });
      if (message.reactions.cend() == it_rating_reaction) {
        submissions_info.push(fmt::format("Not rated: {} - ID: {}\n", message_url, message_id));
        continue;
      }
      auto const rating_reaction_users = GetReactionUsers(message, it_rating_reaction->emoji_name, it_rating_reaction->emoji_id);
      auto const has_squchan_reacted = std::any_of(rating_reaction_users.cbegin(), rating_reaction_users.cend(),
                                                  [](auto const& user){ return Settings::Get().GetSquchanUserId() == user.first; });
      if (!has_squchan_reacted) {
        submissions_info.push(fmt::format("Not rated: {} - ID: {}\n", message_url, message_id));
        continue;
      }

      auto const rating = rating_emojis_.at(it_rating_reaction->emoji_name);
      submissions_info.push(fmt::format("{} point{}: {} - ID: {}\n", rating, (rating == 1) ? "" : "s", message_url, message_id));
      total_user_points_in_thread += rating;
    }
  } while (!messages.empty());

  std::string submissions_log;
  submissions_log.reserve(kMaxMessageLength);
  submissions_log = fmt::format("**{} point{} for submissions in {}:**\n",
                                total_user_points_in_thread, (total_user_points_in_thread == 1) ? "" : "s", ::GetThreadString(thread_id));
  if (submissions_info.empty()) {
    submissions_log.append("No entries");
    SendDirectMessage(user_id, submissions_log);
  } else {
    while (!submissions_info.empty()) {
      AppendMessageContent(submissions_log, submissions_info);
      SendDirectMessage(user_id, submissions_log);
      submissions_log.clear();
    }
  }
}

void PokattoPrestige::SendDirectMessage(dpp::snowflake const user_id, std::string const& message) const {
  try {
    bot_->direct_message_create_sync(user_id, dpp::message(message));
  } catch (dpp::exception const& rest_exception) {
    logger_.Error("Failed to send direct message. User id: '{}'. Message: '{}' Exception '{}'", user_id, message, rest_exception.what());
  }
}

void PokattoPrestige::ProcessLeaderboardEntries(std::string& leaderboard_message, std::queue<std::string>& leaderboard_entries) const {
  if (leaderboard_entries.empty()) {
    leaderboard_message.append("No entries");
    SendLeaderboardMessage(leaderboard_message);
  } else {
    while (!leaderboard_entries.empty()) {
      AppendMessageContent(leaderboard_message, leaderboard_entries);
      SendLeaderboardMessage(leaderboard_message);
      leaderboard_message.clear();
    }
  }
}

void PokattoPrestige::SendLeaderboardMessage(std::string const& leaderboard_message) const {
  auto const message = dpp::message(Settings::Get().GetPokattoPrestigePathChannelId(), leaderboard_message);
  try {
    bot_->message_create_sync(message);
  } catch (dpp::exception const& rest_exception) {
    logger_.Error("Failed to send leaderboard message. Message: '{}'. Exception '{}'", leaderboard_message, rest_exception.what());
  }
}

void PokattoPrestige::ClearSubmissionsFutures() noexcept {
  submissions_futures_.remove_if([](auto const& future) { return std::future_status::ready == future.wait_for(std::chrono::milliseconds(0)); });
}