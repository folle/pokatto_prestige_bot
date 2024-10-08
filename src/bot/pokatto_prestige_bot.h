#pragma once

#include <memory>

#include <dpp/dpp.h>

#include "pokatto_prestige/pokatto_prestige.h"
#include "settings/settings.h"
#include "logger/logger_factory.h"

class PokattoPrestigeBot final {
public:
  PokattoPrestigeBot() = delete;
  ~PokattoPrestigeBot();

  PokattoPrestigeBot(bool deploy_slash_commands, bool welcome_squchan);

  void Start() const noexcept;

private:
  void OnLog(dpp::log_t const& log) const noexcept;
  void OnMessageReactionAdd(dpp::message_reaction_add_t const& message_reaction_add) noexcept;
  void OnReady(dpp::ready_t const& ready) const noexcept;
  void OnSlashCommand(dpp::slashcommand_t const& slash_command) noexcept;

  bool DeploySlashCommands() const;

private:
  Logger const logger_ = LoggerFactory::Get().Create("Pokatto Prestige Bot");

  std::shared_ptr<dpp::cluster> const bot_ = std::make_shared<dpp::cluster>(Settings::Get().GetBotToken(), dpp::i_default_intents);

  std::unique_ptr<PokattoPrestige> pokatto_prestige_;
};