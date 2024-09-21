#include "pokatto_prestige_bot.h"

namespace {
  auto constexpr kGetPointsHistorySlashCommand = "get_points_history";
  auto constexpr kResyncMissedPointsSlashCommand = "resync_missed_points";
}

PokattoPrestigeBot::PokattoPrestigeBot(bool const deploy_slash_commands, bool const welcome_squchan) noexcept {
  bot_->on_log([this](dpp::log_t const& event) { OnLog(event); });
  bot_->on_message_reaction_add([this](dpp::message_reaction_add_t const& message_reaction_add) { OnMessageReactionAdd(message_reaction_add); });
  bot_->on_ready([this, deploy_slash_commands, welcome_squchan](dpp::ready_t const& ready) { OnReady(ready, deploy_slash_commands, welcome_squchan); });
  bot_->on_slashcommand([this](dpp::slashcommand_t const& slash_command) { OnSlashCommand(slash_command); });

  logger_.Info("Initialized bot");
}

PokattoPrestigeBot::~PokattoPrestigeBot() {
  logger_.Info("Bot terminated");
}

void PokattoPrestigeBot::Start() const noexcept {
  logger_.Info("Starting bot event handler loop");
  bot_->start(dpp::st_wait);
}

void PokattoPrestigeBot::OnLog(dpp::log_t const& event) const noexcept {
  switch (event.severity) {
    case dpp::ll_info: {
      logger_.Info("{}", event.message);
      break;
    }
    case dpp::ll_warning: {
      logger_.Warn("{}", event.message);
      break;
    }
    default: {
      logger_.Error("{}", event.message);
      break;
    }
  }
}

void PokattoPrestigeBot::OnMessageReactionAdd(dpp::message_reaction_add_t const& message_reaction_add) {
  if (!pokatto_prestige_.IsSubmissionMessage(message_reaction_add.channel_id) ||
      !pokatto_prestige_.IsValidRating(message_reaction_add.reacting_user.id, message_reaction_add.reacting_emoji.name)) {
    return;
  }

  pokatto_prestige_.AddRating(message_reaction_add.message_id, message_reaction_add.channel_id, message_reaction_add.reacting_emoji.name);
}

void PokattoPrestigeBot::OnReady(dpp::ready_t const& ready, bool const deploy_slash_commands, bool const welcome_squchan) {
  logger_.Info("Bot event handler loop started");

  if (deploy_slash_commands) {
    DeploySlashCommands();
  }

  if (welcome_squchan) {
    bot_->direct_message_create(Settings::Get().GetSquchanUserId(), dpp::message("Hello Squ! Welcome to the Pokatto Prestige Bot!"));
  }

  if (pokatto_prestige_.SuccessfullyInitialized()) {
    bot_->direct_message_create(Settings::Get().GetFolleUserId(), dpp::message("INIT OK"));

    pokatto_prestige_.ResyncAllPoints();
  } else {
    bot_->direct_message_create(Settings::Get().GetFolleUserId(), dpp::message("INIT FAILED"));
  }
}

void PokattoPrestigeBot::OnSlashCommand(dpp::slashcommand_t const& slash_command) {
  if (slash_command.command.get_command_name() == kGetPointsHistorySlashCommand) {
    logger_.Info("Received 'get_points_history' slash command. Username: '{}'. User id '{}'",
                 slash_command.command.get_issuing_user().username, slash_command.command.get_issuing_user().id);

    auto const get_points_history_reply = dpp::message("Your points history will be DM'd to you soon.").set_flags(dpp::m_ephemeral);
    slash_command.reply(get_points_history_reply);

    pokatto_prestige_.SendPointsHistory(slash_command.command.get_issuing_user().id);
  } else if (slash_command.command.get_command_name() == kResyncMissedPointsSlashCommand) {
    logger_.Info("Received 'resync_missed_points' slash command");

    if (slash_command.command.get_issuing_user().id != Settings::Get().GetSquchanUserId()) {
      auto const invalid_user_reply = dpp::message("Only SquChan can trigger this command.").set_flags(dpp::m_ephemeral);
      slash_command.reply(invalid_user_reply);
    }

    auto const resync_missed_points_reply = dpp::message("Triggered missed points resync.").set_flags(dpp::m_ephemeral);
    slash_command.reply(resync_missed_points_reply);

    pokatto_prestige_.ResyncMissedPoints();
  }
}

void PokattoPrestigeBot::DeploySlashCommands() const noexcept {
  if (dpp::run_once<struct register_bot_commands>()) {
    dpp::slashcommand get_points_history_command(kGetPointsHistorySlashCommand, "You will be DM'd all yours posts and points.", bot_->me.id);
    dpp::slashcommand resync_missed_points_command(kResyncMissedPointsSlashCommand, "SquChan only. Triggers a resync of any missed points.", bot_->me.id);

    bot_->guild_bulk_command_create({get_points_history_command, resync_missed_points_command}, Settings::Get().GetServerId());

    logger_.Info("Triggered slash commands deployment");
  }
}