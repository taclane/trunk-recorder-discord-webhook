#include "../discord_webhook_plugin.h"

void Discord_Bot::slash_rates(dpp::cluster &bot, const dpp::slashcommand_t &event)
{
	BOOST_LOG_TRIVIAL(info) << log_prefix << "/rates - Requested by " << event.command.usr.username;

	std::string system = "";

	auto parameter = event.get_parameter("shortname");
	if (std::holds_alternative<std::string>(parameter))
	{
		system = std::get<std::string>(parameter);
	}

	dpp::message rate_reply = get_rate_message(system);

	event.reply(rate_reply);
}

dpp::message Discord_Bot::get_rate_message(std::string system)
{
	bool rates_found = false;
	dpp::message rate_reply;
	dpp::embed rate_embed;

	rate_embed
		.set_color(dpp::colors::red)
		.set_title("Control Channel Decode Rates")
		.set_timestamp(time(nullptr));

	for (auto &[key, val] : last_rates.items())
	{
		if ((std::string(val["short_name"]) == system) || (system == ""))
		{
			rates_found = true;
			std::string field_title = key + ". " + std::string(val["short_name"]);
			std::string field_msg = std::string(val["decode_rate"]) + " msg/s\n```" + std::string(val["freq"]) + "\n" + std::string(val["rate_graph"]) + "```";

			rate_embed
				.add_field(field_title, field_msg, true)
				.set_footer(
					dpp::embed_footer()
						.set_text(std::string(val["rate_interval"]) + "s avg. | " + std::string(val["rate_history"]) + " s. history"));
		}
	}

	if (!rates_found)
	{
		rate_embed
			.set_description("No rates to display.");
	};

	rate_reply.add_embed(rate_embed);

	return rate_reply;
}