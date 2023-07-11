// Discord Webhook Plugin for Trunk-Recorder
// ********************************
// Requires trunk-recorder 4.5 or later
// ********************************

#include <curl/curl.h>
#include <time.h>
#include <iomanip>
#include <vector>
#include <map>

#include <trunk-recorder/plugin_manager/plugin_api.h>

#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <sys/stat.h>

struct Webhook
{
  std::string description;
  std::string event;
  std::string selector;
  std::string webhook_url;
  std::string rate_bucket;
  std::string username;
  std::string avatar_url;
  std::string message;
  std::string content;
  int color;
};

// struct RateBucket
// {
//   int limit;
//   int remaining;
//   int reset;
//   float reset_after;
//   std::string bucket;
// };

// struct WebHooks
// {
//   std::vector<Webhook> call;
//   std::vector<Webhook> rate;
// };

class Discord_Webhook : public Plugin_Api
{
  std::vector<Webhook> system_webhooks;
  std::vector<Webhook> status_webhooks;
  //std::map<std::string, RateBucket> rate_buckets;

public:
  // ********************************
  // trunk-recorder webhook messages
  // ********************************

  // system_rates()
  //   Send control channel messages per second updates; rounded to two decimal places
  int system_rates(std::vector<System *> systems, float timeDiff) override
  {
    for (std::vector<Webhook>::iterator it = status_webhooks.begin(); it != status_webhooks.end(); ++it)
    {
      if (it->event == "rate")
      {
        Webhook *hook = &(*it);

        bool execute_hook = false;
        boost::property_tree::ptree field, fields_array;

        for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it)
        {
          System *system = *it;
          std::string sys_name = system->get_short_name();

          if ((hook->selector == sys_name) || (hook->selector.empty()))
          {
            execute_hook = true;
            std::string sys_type = system->get_system_type();

            // Filter out conventional systems.  They do not have a call rate and
            // get_current_control_channel() will cause a sefgault on non-trunked systems.
            if (sys_type.find("conventional") == std::string::npos)
            {
              boost::property_tree::ptree stat_node = system->get_stats_current(timeDiff);

              field.put("name", stat_node.get<std::string>("id") + ". " + sys_name);
              field.put("value", round_to_str(stat_node.get<double>("decoderate")) + "\n`" + freq_to_str(system->get_current_control_channel()) + "`");
              field.put("inline", "true");
              fields_array.push_back(make_pair("", field));
              field.clear();
            }
          }
        }

        if (execute_hook)
        {
          boost::property_tree::ptree webhook_data;
          boost::property_tree::ptree embed, embeds_array;

          std::string author_name = "Message Decode Rates";
          std::string footer_text = round_to_str(timeDiff) + "s avg.";

          webhook_data.put("content", hook->content);
          webhook_data.put("username", hook->username);
          webhook_data.put("avatar_url", hook->avatar_url);

          embed.put("color", hook->color);
          embed.put("type", "rich");
          embed.put("description", hook->message);
          embed.put("author.name", author_name);
          embed.put("author.icon_url", hook->avatar_url);
          embed.put("footer.text", footer_text);
          embed.put("timestamp", epoch_to_iso(time(NULL)));

          embed.add_child("fields", fields_array);
          embeds_array.push_back(make_pair("", embed));
          webhook_data.add_child("embeds", embeds_array);

          execute_webhook(webhook_data, hook);
        }
      }
    }
    return 0;
  }

  // call_end()
  //   Send information about a completed call and participating (trunked/conventional) units.
  //   TRUNK-RECORDER PLUGIN API: Called after a call ends
  int call_end(Call_Data_t call_info)
  {
    for (std::vector<Webhook>::iterator it = system_webhooks.begin(); it != system_webhooks.end(); ++it)
    {
      Webhook *hook = &(*it);
      if ((hook->selector == call_info.short_name) || (hook->selector.empty()))
      {
        std::string sources;
        BOOST_FOREACH (auto &unit, call_info.transmission_source_list)
        {
          if (!sources.empty())
            sources += ", ";
          sources += "`" + std::to_string(unit.source) + "`";
          if (!unit.tag.empty())
            sources += " " + unit.tag;
        }

        boost::property_tree::ptree webhook_data;
        boost::property_tree::ptree embed, embeds_array;
        boost::property_tree::ptree field, fields_array;
        std::string author_name = "[" + std::to_string(call_info.talkgroup) + "] " + call_info.talkgroup_alpha_tag;
        std::string footer_text = call_info.short_name + " - " + freq_to_str(call_info.freq) + ", " + call_info.audio_type;

        webhook_data.put("content", hook->content);
        webhook_data.put("username", hook->username);
        webhook_data.put("avatar_url", hook->avatar_url);

        embed.put("color", hook->color);
        embed.put("type", "rich");
        embed.put("description", hook->message);
        embed.put("author.name", author_name);
        embed.put("author.icon_url", hook->avatar_url);
        embed.put("footer.text", footer_text);
        embed.put("timestamp", epoch_to_iso(call_info.start_time));

        field.put("name", "Length");
        field.put("value", round_to_str(call_info.length) + "s");
        field.put("inline", "true");
        fields_array.push_back(make_pair("", field));
        field.clear();

        field.put("name", "Units");
        field.put("value", sources);
        field.put("inline", "true");
        fields_array.push_back(make_pair("", field));
        field.clear();

        embed.add_child("fields", fields_array);
        embeds_array.push_back(make_pair("", embed));
        webhook_data.add_child("embeds", embeds_array);

        execute_webhook(webhook_data, hook);
      }
    }
    return 0;
  }

  // ********************************
  // Helper functions
  // ********************************

  // round_to_str()
  //   Round a float to two decimal places and return it as as string.
  //   "position", "length", and "duration" are the usual offenders.
  std::string round_to_str(double num)
  {
    char rounded[20];
    snprintf(rounded, sizeof(rounded), "%.2f", num);
    return std::string(rounded);
  }

  // epoch_to_iso()
  //   Convert an epoch timestamp from t-r to ISO format for Discord
  std::string epoch_to_iso(int epoch)
  {
    std::time_t epoch_time = epoch;
    boost::posix_time::ptime time = boost::posix_time::from_time_t(epoch_time);

    return boost::posix_time::to_iso_string(time);
  }

  // freq_to_str()
  //   Convert a freq to MHz string
  std::string freq_to_str(double num)
  {
    std::string freq = (boost::format("%10.6f MHz") % (num / 1000000.0)).str();
    return freq;
  }

  // ********************************
  // trunk-recorder plugin API & startup
  // ********************************

  // parse_config()
  //   TRUNK-RECORDER PLUGIN API: Called before init(); parses the config information for this plugin.
  int parse_config(boost::property_tree::ptree &cfg)
  {
    boost::regex url_regex("(https://discord.com/api/webhooks/[0-9]*)/(.*)?");
    boost::cmatch match;

    std::string default_username = "Trunk Recorder";
    std::string default_avatar_url = "https://cdn.discordapp.com/icons/928800455444791306/78e52a70389e4a5589b6d22001ccce45.png";
    int default_color = 0xff0000; // red

    // Get the configured systems and webhook URLs
    BOOST_FOREACH (boost::property_tree::ptree::value_type &node, cfg.get_child("webhooks"))
    {

      bool enabled = node.second.get<bool>("enabled", true);
      boost::optional<boost::property_tree::ptree &> webhook_entry = node.second.get_child_optional("event");
      int hook_count = 0;

      if ((webhook_entry) && (enabled))
      {

        Webhook hook;
        hook_count += 1;
        std::string hook_desc = "[" + std::to_string(hook_count) + "]";

        hook.event = node.second.get<std::string>("event", "");
        hook.selector = node.second.get<std::string>("selector", "");
        hook.webhook_url = node.second.get<std::string>("webhook", "");
        hook.description = node.second.get<std::string>("description", hook_desc);
        hook.username = node.second.get<std::string>("username", default_username);
        hook.avatar_url = node.second.get<std::string>("avatar", default_avatar_url);
        hook.color = node.second.get<int>("color", default_color);
        hook.message = node.second.get<std::string>("message", "");
        hook.content = node.second.get<std::string>("content", "");

        if (regex_match(hook.webhook_url.c_str(), match, url_regex))
        {
          std::string redacted_url(match[1].first, match[1].second);
          BOOST_LOG_TRIVIAL(info) << " [" << hook.event << "][" << hook.selector << "] \t" << hook.description;
          BOOST_LOG_TRIVIAL(info) << "  --> " << redacted_url << "/**TOKEN**";

          if (hook.event == "call")
          {
            this->system_webhooks.push_back(hook);
          }
          else if (hook.event == "rate")
          {
            this->status_webhooks.push_back(hook);
          }
        }
        else
        {
          BOOST_LOG_TRIVIAL(error) << " Unable to parse Discord webhook URL for: [" << hook.event << "][" << hook.selector << "]";
        }
      }
    }

    if (this->system_webhooks.size() + this->status_webhooks.size() == 0)
    {
      BOOST_LOG_TRIVIAL(error) << "Discord Webhook Plugin loaded, but no webhooks are configured.";
      return 1;
    }

    return 0;
  }

  // ********************************
  // Curl webhook execution
  // ********************************

  int execute_webhook(boost::property_tree::ptree webhook_data, Webhook *hook)
  {
    // https://autocode.com/tools/discord/embed-builder/
    // https://discord.com/developers/docs/resources/webhook#execute-webhook
    // https://birdie0.github.io/discord-webhooks-guide/discord_webhook.html

    // Discord Embeds require json arrays, see below for boost workarounds:
    // https://www.theunterminatedstring.com/boost-ptree-array/

    std::string webhook_url = hook->webhook_url;
    std::stringstream webhook_json;
    boost::property_tree::write_json(webhook_json, webhook_data);

    CURL *curl = nullptr;
    struct curl_slist *header_list = nullptr;

    BOOST_LOG_TRIVIAL(debug) << "[Discord Webhook JSON] " << webhook_json.str();

    std::string webhook_str = webhook_json.str();

    curl = curl_easy_init();
    if (curl != nullptr)
    {
      long curl_response;
      boost::property_tree::ptree headers;

      // Build the curl request
      header_list = curl_slist_append(header_list, "Content-Type: application/json");
      header_list = curl_slist_append(header_list, "charsets: utf-8");
      curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, webhook_str.size());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, webhook_str.c_str());
      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

      CURLcode curl_ret = curl_easy_perform(curl);

      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &curl_response);
      BOOST_LOG_TRIVIAL(debug) << "curl: " << curl_response << " " << curl_easy_strerror(curl_ret);

      // for (const auto &header : headers)
      // {
      //   std::cout << header.first << ": " << header.second.get_value<std::string>() << std::endl;
      // }
      // BOOST_LOG_TRIVIAL(debug) << "time " << time(NULL) << " time";

      // hook->rate_bucket = headers["x-ratelimit-bucket"];

      // this->rate_buckets[hook->rate_bucket].bucket = hook->rate_bucket;

      curl_easy_cleanup(curl);
      curl_global_cleanup();
    }

    return 0;
  }

  // Curl callback function to get Discord ratelimit headers
  static size_t header_callback(void *contents, size_t size, size_t nmemb, boost::property_tree::ptree *headers)
  {
    size_t totalSize = size * nmemb;
    std::string header(static_cast<char *>(contents), totalSize);

    // Find headers beginning with "x-ratelimit-"
    const std::string headerPrefix = "x-ratelimit-";
    const boost::regex pattern("^(\\S+):\\s*(\\S+)\\s*");
    boost::smatch match;

    if (header.compare(0, headerPrefix.length(), headerPrefix) == 0)
    {
      if (boost::regex_match(header, match, pattern))
      {
        headers->put(match[1].str(), match[2].str());
      }
    }
    return totalSize;
  }

  // ********************************
  // Create the plugin
  // ********************************

  // Factory method
  static boost::shared_ptr<Discord_Webhook> create()
  {
    return boost::shared_ptr<Discord_Webhook>(
        new Discord_Webhook());
  }
};

BOOST_DLL_ALIAS(
    Discord_Webhook::create, // <-- this function is exported with...
    create_plugin            // <-- ...this alias name
)
