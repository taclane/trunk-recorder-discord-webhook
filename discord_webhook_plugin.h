#pragma once

#include <time.h>
#include <iomanip>
#include <vector>
#include <map>

#include <trunk-recorder/plugin_manager/plugin_api.h>
#include <trunk-recorder/json.hpp>

#include <trunk-recorder/source.h>


#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <sys/stat.h>

#include "commands/commands.h"

// D++ LIBRARY MUST BE INCLUDED AFTER TRUNK-RECORDER INCLUDES (json.hpp)
#include <dpp/dpp.h>

struct Webhook
{
    std::string description;
    std::string event;
    std::string selector;
    std::string message;
    std::string content;
    int color;
};

using slash_command_function = std::function<void(dpp::cluster &, const dpp::slashcommand_t &)>;

struct slash_command
{
    // Description of the command, must be lower case, no spaces.
    std::string description;
    // Callback function
    slash_command_function function;
    // Parameters of the slash command
    std::vector<dpp::command_option> parameters = {};
};

class Discord_Bot : public Plugin_Api
{
public:
    std::string log_prefix;
    std::string api_key;

    nlohmann::json last_rates;
    std::map<std::string, std::deque<double>> rate_history;

    int call_count;
    int tx_count;
    int recorder_count;

    // Trunk-Recorder
    Config *tr_config;
    std::vector<Source *> tr_sources;
    std::vector<System *> tr_systems;
    std::vector<Call *> tr_calls;

    std::string round_to_str(double num);
    std::string epoch_to_iso(int epoch);
    std::string freq_to_str(double num);
    double round_to_two(double num);


    int init(Config *config, std::vector<Source *> sources, std::vector<System *> systems) override;
    int parse_config(nlohmann::json cfg) override;
    int system_rates(std::vector<System *> systems, float timeDiff) override;
    int call_start(Call *call) override;
    int start() override;
    int stop() override;
    
    static boost::shared_ptr<Discord_Bot> create();

    dpp::cluster *bot;
    void start_bot(std::string token);
    void stop_bot();

    std::map<std::string, slash_command> slash_commands;

    void slash_ping(dpp::cluster &bot, const dpp::slashcommand_t &event);
    void slash_help(dpp::cluster &bot, const dpp::slashcommand_t &event);
    void slash_info(dpp::cluster &bot, const dpp::slashcommand_t &event);
    void slash_rates(dpp::cluster &bot, const dpp::slashcommand_t &event);
    void slash_rates_tr(std::vector<System *> systems, float timeDiff);
    void slash_tg(dpp::cluster &bot, const dpp::slashcommand_t &event);

    dpp::message get_rate_message(std::string system);

};
