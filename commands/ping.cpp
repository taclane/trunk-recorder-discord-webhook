#include "../discord_webhook_plugin.h"
#include <fmt/format.h>

void Discord_Bot::slash_ping(dpp::cluster& bot, const dpp::slashcommand_t& event) {
	BOOST_LOG_TRIVIAL(info) << log_prefix << "/ping - Requested by " << event.command.usr.username;

	/* Get websocket ping of first shard */
	float ws_ping = bot.get_shard(0)->websocket_ping;

	/* Ping reply */
	event.reply(
		dpp::message().add_embed(
			dpp::embed().
			set_color(dpp::colors::yellow).
			set_title("PONG!").
			set_description(
				fmt::format("Ping is: {0:.02f} ms", (bot.rest_ping + ws_ping) * 1000)
			)
		)
	);
}
