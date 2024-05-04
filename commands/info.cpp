#include "../discord_webhook_plugin.h"
//#include <fmt/format.h>

void Discord_Bot::slash_info(dpp::cluster& bot, const dpp::slashcommand_t& event) 
{
	// Info reply
	BOOST_LOG_TRIVIAL(info) << log_prefix << "/info - Requested by " << event.command.usr.username;
	event.reply(
		dpp::message().add_embed(
			dpp::embed().
				set_color(dpp::colors::red).
				set_title("Trunk Recorder Discord bot").
				set_description("Real-time interface for RF and LOL").
				add_field("Uptime", bot.uptime().to_string(), true).
				add_field("Calls", std::to_string(call_count), true).
				add_field("Transmissions", std::to_string(tx_count), true).
				add_field("Systems", std::to_string(tr_systems.size()), true).
				add_field("Sources", std::to_string(tr_sources.size()), true).
				add_field("Recorders", std::to_string(recorder_count), true)
		)
	);
}

//	add_field("Users", std::to_string(dpp::get_user_count()), true).
//	add_field("Library Version", fmt::format("[{}]({})", dpp::utility::version(), "https://github.com/brainboxdotcc/DPP"), true)
