// Discord Webhook Plugin for Trunk-Recorder
// ********************************
// Requires trunk-recorder 4.5 or later
// ********************************

#include "discord_webhook_plugin.h"

// ********************************
// trunk-recorder discord
// ********************************

// system_rates()
// Send control channel messages per second updates;
// rounded to two decimal places
int Discord_Bot::system_rates(std::vector<System *> systems, float timeDiff)
{

  std::map<int, std::string> graph_blocks = {
      {0, " "},
      {1, "▁"},
      {2, "▂"},
      {3, "▃"},
      {4, "▄"},
      {5, "▅"},
      {6, "▆"},
      {7, "▇"},
      {8, "█"}};

  nlohmann::json rates;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it)
  {
    System *system = *it;
    std::string sys_name = system->get_short_name();
    std::string sys_type = system->get_system_type();
    std::string sys_id = std::to_string(system->get_sys_num());

    // Filter out conventional systems.  They do not have a call rate and
    // get_current_control_channel() will cause a sefgault on non-trunked systems.
    if (sys_type.find("conventional") == std::string::npos)
    {
      // Initialize a system history with null entries
      if (rate_history.find(sys_id) == rate_history.end())
      {
        rate_history[sys_id] = std::deque<double>(65, -1);
      }

      // Update the last rate and history
      boost::property_tree::ptree stat_node = system->get_stats_current(timeDiff);
      rates[sys_id]["short_name"] = sys_name;
      rates[sys_id]["decode_rate"] = round_to_str(stat_node.get<double>("decoderate"));
      rates[sys_id]["freq"] = freq_to_str(system->get_current_control_channel());
      rates[sys_id]["rate_interval"] = round_to_str(timeDiff);
      rates[sys_id]["rate_history"] = round_to_str(timeDiff * rate_history[sys_id].size());

      rate_history[sys_id].push_back(stat_node.get<double>("decoderate"));
      rate_history[sys_id].pop_front();

      //std::vector<int> averages;
      std::string a_graph;

      for (size_t i = 0; i < rate_history[sys_id].size(); i += 5)
      {
        double sum = 0;
        int count = 0;
        for (size_t j = i; j < i + 5 && j < rate_history[sys_id].size(); ++j)
        {
          if (rate_history[sys_id][j] >= 0)
          {
            sum += rate_history[sys_id][j];
            count++;
          }
        }
        // Avoid dividing by zero
        double average = (sum / std::max(count, 1));
        // averages.push_back(std::min(int(std::round(average / 5)), 8));
        // a_graph += graph_blocks[averages.back()];
        a_graph += graph_blocks[std::min(int(std::round(average / 5)), 8)];
      }
      rates[sys_id]["rate_graph"] = a_graph;
    }
  }
  last_rates = rates;
  return 0;
}

int Discord_Bot::call_start(Call *call)
{
  boost::property_tree::ptree stat_node = call->get_stats();
  call_count = stat_node.get<int>("callNum");
  tx_count++;

  return 0;
}

// ********************************
// Helper functions
// ********************************

// round_to_str()
//   Round a float to two decimal places and return it as as string.
//   "position", "length", and "duration" are the usual offenders.
std::string Discord_Bot::round_to_str(double num)
{
  char rounded[20];
  snprintf(rounded, sizeof(rounded), "%.2f", num);
  return std::string(rounded);
}

double Discord_Bot::round_to_two(double num)
{
  std::stringstream stream;
  double rounded_value;

  stream << std::fixed << std::setprecision(2) << num;
  stream >> rounded_value;
  return rounded_value;
}

// epoch_to_iso()
//   Convert an epoch timestamp from t-r to ISO format for Discord
std::string Discord_Bot::epoch_to_iso(int epoch)
{
  std::time_t epoch_time = epoch;
  boost::posix_time::ptime time = boost::posix_time::from_time_t(epoch_time);

  return boost::posix_time::to_iso_string(time);
}

// freq_to_str()
//   Convert a freq to MHz string
std::string Discord_Bot::freq_to_str(double num)
{
  std::string freq = (boost::format("%10.6f MHz") % (num / 1000000.0)).str();
  return freq;
}

// ********************************
// trunk-recorder plugin API & startup
// ********************************

// init()
//   TRUNK-RECORDER PLUGIN API: Plugin initialization; called after parse_config().
int Discord_Bot::init(Config *config, std::vector<Source *> sources, std::vector<System *> systems)
{
  // Establish pointers to systems, sources, and configs if needed later.
  tr_sources = sources;
  tr_systems = systems;
  tr_config = config;

  // Count number of recorders
  for (std::vector<Source *>::iterator it = tr_sources.begin(); it != tr_sources.end(); ++it)
  {
  int count = (*it)->get_recorders().size(); 
    recorder_count += count;
  };
  
  return 0;
}

// parse_config()
//   TRUNK-RECORDER PLUGIN API: Called before init(); parses the config information for this plugin.
// int parse_config(boost::property_tree::ptree &cfg)
int Discord_Bot::parse_config(nlohmann::json cfg)
{
  int default_color = 0xff0000; // red
  this->log_prefix = "\t[Discord Bot]\t";
  this->api_key = cfg["apiKey"];

  this->log_prefix = "[Discord Bot]\t";

  tx_count = 0;
  call_count = 0;
  recorder_count = 0;
  return 0;
}

int Discord_Bot::start()
{
  start_bot(this->api_key);
  return 0;
}

int Discord_Bot::stop()
{
  stop_bot();
  return 0;
}

void Discord_Bot::start_bot(std::string token)
{
  // Define slash commands
  slash_commands = {
      {"ping", {"A ping command", [this](dpp::cluster &bot, const dpp::slashcommand_t &command)
                {
                  this->slash_ping(bot, command);
                }}},
      {"helpy", {"A help command", [this](dpp::cluster &bot, const dpp::slashcommand_t &command)
                 {
                   this->slash_help(bot, command);
                 },
                 {
                     dpp::command_option(dpp::co_string, "term", "Help term", false),
                     dpp::command_option(dpp::co_string, "blerp", "Blerp term", false),
                 }}},
      {"info", {"An info command", [this](dpp::cluster &bot, const dpp::slashcommand_t &command)
                {
                  this->slash_info(bot, command);
                }}},
      {"rates", {"System decode rates", [this](dpp::cluster &bot, const dpp::slashcommand_t &command)
                 {
                   this->slash_rates(bot, command);
                 },
                 {
                     dpp::command_option(dpp::co_string, "shortname", "System `shortName`", false),
                 }}},
      {"tg", {"System talkgroups", [this](dpp::cluster &bot, const dpp::slashcommand_t &command)
              {
                this->slash_tg(bot, command);
              },
              {
                  dpp::command_option(dpp::co_string, "shortname", "System `shortName`", false),
              }}},
  };

  // Create the bot
  bot = new dpp::cluster(token);

  // Set actions on bot log
  bot->on_log([this](const dpp::log_t &log)
              {
                // Log most bot messages to the info level, critical and errors to the error level
                switch (log.severity) {
                  case dpp::loglevel::ll_trace: BOOST_LOG_TRIVIAL(info) << log_prefix << log.message; break;
                  case dpp::loglevel::ll_debug: BOOST_LOG_TRIVIAL(info) << log_prefix << log.message; break;
                  case dpp::loglevel::ll_info: BOOST_LOG_TRIVIAL(info) << log_prefix << log.message; break;
                  case dpp::loglevel::ll_warning: BOOST_LOG_TRIVIAL(info) << log_prefix << log.message; break;
                  case dpp::loglevel::ll_error: BOOST_LOG_TRIVIAL(error) << log_prefix << log.message; break;
                  case dpp::loglevel::ll_critical: BOOST_LOG_TRIVIAL(error) << log_prefix << log.message; break;
                } });

  // Set actions on bot ready
  bot->on_ready([this](const dpp::ready_t &event)
                {
                  bot->set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_watching, "the airwaves"));
                  BOOST_LOG_TRIVIAL(info) << log_prefix << "Logged in as: \033[0;35m" << bot->me.username << "#" << bot->me.discriminator << "\033[0m (" << bot->me.id << ")";
                  BOOST_LOG_TRIVIAL(info) << log_prefix << "Invite link: \033[0;36m" << dpp::utility::bot_invite_url(bot->me.id, 380175109184, {"bot", "applications.commands"}) << "\033[0m";

                  // if (dpp::run_once<struct clear_bot_commands>()) {
                  //     bot->global_bulk_command_delete();
                  // }

                  if (dpp::run_once<struct bulk_register>())
                  {
                    std::vector<dpp::slashcommand> ready_slash_commands;
                    // Get the defined slash commands
                    for (auto &cmd : slash_commands)
                    {
                      // Create the slash command
                      dpp::slashcommand slash_command;
                      slash_command.set_name(cmd.first).set_description(cmd.second.description).set_application_id(bot->me.id);
                      slash_command.options = cmd.second.parameters;
                      ready_slash_commands.push_back(slash_command);
                      BOOST_LOG_TRIVIAL(info) << log_prefix << "  Registering command: /" << cmd.first;
                    }
                    // Register the slash commands
                    bot->global_bulk_command_create(ready_slash_commands);
                  } });

  // Set functions to run on slash command
  bot->on_slashcommand([this](const dpp::slashcommand_t &event)
                       {
                         // Get the slash command
                         dpp::command_interaction cmd_data = event.command.get_command_interaction();
                         // Verify that the command exists
                         auto cmd = slash_commands.find(cmd_data.name);
                         if (cmd != slash_commands.end())
                         {
                           // Execute the command
                           cmd->second.function(*bot, event);
                         } });

  // Start the bot
  bot->start(dpp::st_return);
}

void Discord_Bot::stop_bot()
{
  // Stop the bot
  bot->set_presence(dpp::presence(dpp::presence_status::ps_dnd, dpp::activity_type::at_watching, "the airwaves"));
  BOOST_LOG_TRIVIAL(info) << log_prefix << "Stopping bot";
  delete bot;
}

// ********************************
// Create the plugin
// ********************************

// Factory method
boost::shared_ptr<Discord_Bot> Discord_Bot::create()
{
  return boost::shared_ptr<Discord_Bot>(new Discord_Bot());
};

BOOST_DLL_ALIAS(
    Discord_Bot::create, // <-- this function is exported with...
    create_plugin        // <-- ...this alias name
)
