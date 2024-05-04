#include "../discord_webhook_plugin.h"

void Discord_Bot::slash_help(dpp::cluster& bot, const dpp::slashcommand_t& event) {
	auto parameter = event.get_parameter("term");

	/* Check to see if the parameter is provided by the user */
	if (std::holds_alternative<std::string>(parameter)) {

		/* If it is, get it and check what's in it */
		std::string search_term = std::get<std::string>(parameter);
		if (search_term == "ping") {
			/* Some reply to help for PING */
			event.reply("This help left useless as an exercise for the developer.");
		} else {
			/* Anything, something, so long as the bot replies. */
			event.reply("This message intentionally left blank. 🤪");
		}
	}
	/* Help reply */
	event.reply(
		dpp::message().add_embed(
			dpp::embed().
			set_color(dpp::colors::yellow).
			set_title("Help").
			set_description("Help goes here").
			add_field("/helpy", "This command").
			add_field("/info", "TR system info").
			add_field("/ping", "Ping the bot").
			add_field("/rates", "System rates").
			add_field("/tg", "System talkgroups")
		)
	);

	BOOST_LOG_TRIVIAL(info) << log_prefix << "Help!";
}


		// .set_text("Requested by " + event.command.usr.username);
