#include <exception>
#include <iostream>

#include <argparse/argparse.hpp>

#include "bot/pokatto_prestige_bot.h"

void ParseArguments(int const argc, char const *const *const argv, bool& deploy_slash_commands, bool& welcome_squchan) {
  argparse::ArgumentParser argument_parser("Pokatto Prestige Bot", "1.0");

  argument_parser.add_argument("--deploy_slash_commands")
    .help("Deploy the slash commands to the guild specified in settings/discord_settings.json")
    .store_into(deploy_slash_commands);

  argument_parser.add_argument("--welcome_squchan")
    .help("Sends a welcome message to SquChan")
    .store_into(welcome_squchan);

  argument_parser.parse_args(argc, argv);
}

int main(int const argc, char const *const *const argv) {
  bool deploy_slash_commands{};
  bool welcome_squchan{};
  try {
    ParseArguments(argc, argv, deploy_slash_commands, welcome_squchan);

    PokattoPrestigeBot bot(deploy_slash_commands, welcome_squchan);
    bot.Start();
  }
  catch (std::exception const& exception) {
    std::cerr << exception.what();
    return -1;
  }

  return 0;
}