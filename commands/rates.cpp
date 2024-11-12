#include "../discord_webhook_plugin.h"

void Discord_Bot::slash_rates_tr(std::vector<System *> systems, float timeDiff) {

  //   std::vector<std::string> graph_blocks = {
  //       " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
  // " "
  std::vector<std::string> braille_blocks = {
      "⠀", "⢀", "⢠", "⢰", "⢸",
      "⡀", "⣀", "⣠", "⣰", "⣸",
      "⡄", "⣄", "⣤", "⣴", "⣼",
      "⡆", "⣆", "⣦", "⣶", "⣾",
      "⡇", "⣇", "⣧", "⣷", "⣿"};

  nlohmann::json rates;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *system = *it;
    std::string sys_name = system->get_short_name();
    std::string sys_type = system->get_system_type();
    std::string sys_id = std::to_string(system->get_sys_num());

    int sample_size = 5;

    // Filter out conventional systems.  They do not have a call rate and
    // get_current_control_channel() will cause a sefgault on non-trunked systems.
    if (sys_type.find("conventional") == std::string::npos) {
      // Initialize a system history with null entries
      if (rate_history.find(sys_id) == rate_history.end()) {
        // rate_history[sys_id] = std::deque<double>(65, -1); //  13 15-second avgs
        rate_history[sys_id] = std::deque<double>(120, -1); // 12 15-second avgs
      }

      // Update the last rate and history
      boost::property_tree::ptree stat_node = system->get_stats_current(timeDiff);
      rates[sys_id]["short_name"] = sys_name;
      rates[sys_id]["decode_rate"] = round_to_str(stat_node.get<double>("decoderate"));
      rates[sys_id]["freq"] = freq_to_str(system->get_current_control_channel());
      rates[sys_id]["rate_interval"] = std::to_string(int(timeDiff * sample_size));
      rates[sys_id]["rate_history"] = std::to_string(int(timeDiff * rate_history[sys_id].size()));

      rate_history[sys_id].push_back(stat_node.get<double>("decoderate"));
      rate_history[sys_id].pop_front();

      // std::vector<int> averages;
      // std::string a_graph;

      std::string t_graph;
      std::string b_graph;

      for (size_t i = 0; i < rate_history[sys_id].size(); i += (2 * sample_size)) {
        double sum = 0;
        int count = 0;

        for (size_t j = i; j < i + sample_size && j < rate_history[sys_id].size(); ++j) {
          if (rate_history[sys_id][j] != -1) {
            sum += rate_history[sys_id][j];
            count++;
          }
        }
        // Avoid dividing by zero
        double average_l = (sum / std::max(count, 1));

        std::vector<int> block_l = {
            std::max(std::min(int(std::round(average_l / 5)), 8) - 4, 0),
            std::min(int(std::round(average_l / 5)), 4)};

        sum = 0;
        count = 0;

        for (size_t j = i + sample_size; j < i + (2 * sample_size) && j < rate_history[sys_id].size(); ++j) {
          if (rate_history[sys_id][j] != -1) {
            sum += rate_history[sys_id][j];
            count++;
          }
        }
        // Avoid dividing by zero
        double average_r = (sum / std::max(count, 1));

        std::vector<int> block_r = {
            std::max(std::min(int(std::round(average_r / 5)), 8) - 4, 0),
            std::min(int(std::round(average_r / 5)), 4)};

        // averages.push_back(std::min(int(std::round(average / 5)), 8));
        // a_graph += graph_blocks[averages.back()];
        // a_graph += graph_blocks[std::min(int(std::round(average / 5)), 8)];

        // t_graph += braille_blocks[std::max(std::min(int(std::round(average / 5)), 8) - 4, 0)];
        // b_graph += braille_blocks[std::min(int(std::round(average / 5)), 4)];

        // BOOST_LOG_TRIVIAL(info) << log_prefix << "/rates - " << block_l[0] << " " << block_l[1] << " " << block_r[0] << " " << block_r[1];

        t_graph += braille_blocks[block_l[0] * 5 + block_r[0]];
        b_graph += braille_blocks[block_l[1] * 5 + block_r[1]];
      }

      //   rates[sys_id]["rate_graph"] = a_graph;
      rates[sys_id]["t_graph"] = t_graph;
      rates[sys_id]["b_graph"] = b_graph;
    }
  }
  last_rates = rates;
}

void Discord_Bot::slash_rates(dpp::cluster &bot, const dpp::slashcommand_t &event) {
  BOOST_LOG_TRIVIAL(info) << log_prefix << "/rates - Requested by " << event.command.usr.username;

  std::string system = "";

  auto parameter = event.get_parameter("shortname");
  if (std::holds_alternative<std::string>(parameter)) {
    system = std::get<std::string>(parameter);
  }

  dpp::message rate_reply = get_rate_message(system);

  event.reply(rate_reply);
}

dpp::message Discord_Bot::get_rate_message(std::string system) {
  bool rates_found = false;
  dpp::message rate_reply;
  dpp::embed rate_embed;

  rate_embed
      .set_color(dpp::colors::red)
      .set_title("Control Channel Decode Rates")
      .set_timestamp(time(nullptr));

  for (auto &[key, val] : last_rates.items()) {
    if ((std::string(val["short_name"]) == system) || (system == "")) {
      rates_found = true;
      std::string field_title = key + ". " + std::string(val["short_name"]);
      // std::string field_msg = std::string(val["decode_rate"]) + " msg/s\n```" + std::string(val["freq"]) + "\n" + std::string(val["rate_graph"]) + "```";
      std::string field_msg = std::string(val["decode_rate"]) + " msg/s\n```" + std::string(val["freq"]) + "\n" + std::string(val["t_graph"]) + "\n" + std::string(val["b_graph"]) + "```";

      rate_embed
          .add_field(field_title, field_msg, true)
          .set_footer(
              dpp::embed_footer()
                  .set_text(std::string(val["rate_interval"]) + "s avg. | " + std::string(val["rate_history"]) + "s history"));
    }
  }

  if (!rates_found) {
    rate_embed
        .set_description("No rates to display.");
  };

  rate_reply.add_embed(rate_embed);

  return rate_reply;
}