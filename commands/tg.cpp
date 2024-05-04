#include "../discord_webhook_plugin.h"

void Discord_Bot::slash_tg(dpp::cluster &bot, const dpp::slashcommand_t &event)
{
    BOOST_LOG_TRIVIAL(info) << log_prefix << "/tg - Requested by " << event.command.usr.username;

	bool tgs_found = false;
	dpp::message tg_reply;
	dpp::embed tg_embed;

	tg_embed
		.set_color(dpp::colors::red)
		.set_title("System Talkgroups")
		.set_timestamp(time(nullptr));

    for (std::vector<System *>::iterator it = tr_systems.begin(); it != tr_systems.end(); ++it)
	{
        System *sys = (System *)*it;

		// std::vector<Talkgroup *> talkgroups = sys->get_talkgroups();
		// for (Talkgroup* ptr : talkgroups) {
		// 	delete ptr; // Delete each pointer
		// }
		// talkgroups.clear(); 

        BOOST_LOG_TRIVIAL(info) << log_prefix << " system: " << sys->get_sys_num() << " - " << sys->get_short_name();
        BOOST_LOG_TRIVIAL(info) << log_prefix << " - " << sys->get_talkgroups().size();
        BOOST_LOG_TRIVIAL(info) << log_prefix << " - " << sys->get_talkgroups_file();

        int sys_num = sys->get_sys_num();
        std::string talkgroups_file = sys->get_talkgroups_file();

        // Talkgroups* tg = *(sys->talkgroups->talkgroups);
        // sys->talkgroups->load_talkgroups(sys_num, talkgroups_file)
        // = nullptr;;

		if (talkgroups_file != "")
		{
			tgs_found = true;
			std::string field_title = std::to_string(sys->get_sys_num()) + ". " + sys->get_short_name();
			std::string field_msg = std::to_string(sys->get_talkgroups().size());

			//std::string(val["decode_rate"]) + " msg/s\n```" + std::string(val["freq"]) + "\n" + std::string(val["rate_graph"]) + "```";

			tg_embed
				.add_field(field_title, field_msg, true)
				.set_footer(
					dpp::embed_footer()
						.set_text(std::string("Talkgroups")));
		}
	}

	if (!tgs_found)
	{
	tg_embed
			.set_description("No rates to display.");
	};


	tg_reply.add_embed(tg_embed);
	event.reply(tg_reply);

}