#pragma once
#include <dpp/dpp.h>

void handle_ping(dpp::cluster &bot, const dpp::slashcommand_t &event);
void handle_help(dpp::cluster &bot, const dpp::slashcommand_t &event);
void handle_info(dpp::cluster &bot, const dpp::slashcommand_t &event);