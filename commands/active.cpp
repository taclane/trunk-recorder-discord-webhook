#include "../discord_webhook_plugin.h"
#include <set>

void Discord_Bot::slash_active(dpp::cluster &bot, const dpp::slashcommand_t &event) {
  BOOST_LOG_TRIVIAL(info) << log_prefix << "/active - Requested by " << event.command.usr.username;

  std::string system_filter = "";

  // Check if system parameter was provided
  auto parameter = event.get_parameter("shortname");
  if (std::holds_alternative<std::string>(parameter)) {
    system_filter = std::get<std::string>(parameter);
  }

  dpp::embed active_embed;
  active_embed
      .set_color(dpp::colors::green)
      .set_title("Active Calls")
      .set_timestamp(time(nullptr));

  bool calls_found = false;
  int active_count = 0;

  // Iterate through all active calls
  for (std::vector<Call *>::iterator it = tr_calls.begin(); it != tr_calls.end(); ++it) {
    Call *call = *it;
    
    if (call == nullptr) {
      continue;
    }

    // Get call state
    State call_state = call->get_state();
    
    // Only show active calls (not inactive or stopped)
    if (call_state != MONITORING && call_state != RECORDING) {
      continue;
    }

    System *sys = call->get_system();
    if (sys == nullptr) {
      continue;
    }

    std::string sys_short_name = sys->get_short_name();

    // Apply system filter if provided
    if (!system_filter.empty() && sys_short_name != system_filter) {
      continue;
    }

    calls_found = true;
    active_count++;

    // Get call details
    long talkgroup_num = call->get_talkgroup();
    double freq = call->get_freq();
    time_t start_time = call->get_start_time();
    bool emergency = call->get_emergency();
    bool encrypted = call->get_encrypted();

    // Get talkgroup information
    Talkgroup *tg = sys->find_talkgroup(talkgroup_num);
    std::string talkgroup_tag = (tg != nullptr) ? tg->alpha_tag : std::to_string(talkgroup_num);

    // Calculate duration
    time_t current_time = time(nullptr);
    double duration = difftime(current_time, start_time);

    // Build field title
    std::ostringstream field_title;
    field_title << talkgroup_num;
    if (!talkgroup_tag.empty() && talkgroup_tag != std::to_string(talkgroup_num)) {
      field_title << " - " << talkgroup_tag;
    }
    if (emergency) {
      field_title << " 🚨";
    }
    if (encrypted) {
      field_title << " 🔒";
    }

    // Build field content
    std::ostringstream field_content;
    field_content << "**System:** " << sys_short_name << "\n";
    field_content << "**Frequency:** " << freq_to_str(freq) << "\n";
    field_content << "**Duration:** " << round_to_str(duration) << "s\n";
    
    // Add talkgroup description and group if available
    if (tg != nullptr) {
      if (!tg->description.empty()) {
        field_content << "**Description:** " << tg->description << "\n";
      }
      if (!tg->group.empty()) {
        field_content << "**Group:** " << tg->group << "\n";
      }
    }

    // Add audio type
    std::string audio_type = "digital";
    if (call->get_is_analog()) {
      audio_type = "analog";
    } else if (call->get_phase2_tdma()) {
      audio_type = "digital tdma";
      int tdma_slot = call->get_tdma_slot();
      if (tdma_slot >= 0) {
        audio_type += " (slot " + std::to_string(tdma_slot) + ")";
      }
    }
    field_content << "**Audio:** " << audio_type << "\n";

    // Add state indicator with monitoring reason if applicable
    boost::property_tree::ptree stat_node = call->get_stats();
    if (call_state == RECORDING) {
      field_content << "**Status:** 🔴 Recording\n";
      
      // Show recorder info
      int rec_num = stat_node.get<int>("recNum", -1);
      int src_num = stat_node.get<int>("srcNum", -1);
      if (rec_num >= 0 && src_num >= 0) {
        field_content << "**Recorder:** #" << rec_num << " (Source #" << src_num << ")\n";
      }
    } else if (call_state == MONITORING) {
      field_content << "**Status:** 👂 Monitoring";
      
      // Show why it's monitoring but not recording
      int mon_state = stat_node.get<int>("monState", 0);
      std::map<int, std::string> mon_reasons = {
        {1, "Unknown TG"}, {2, "Ignored TG"}, {3, "No Source"},
        {4, "No Recorder"}, {5, "Encrypted"}, {6, "Duplicate"}, {7, "Superseded"}
      };
      if (mon_reasons.count(mon_state) && mon_state != 0) {
        field_content << " (" << mon_reasons[mon_state] << ")";
      }
      field_content << "\n";
    }

    // Show current transmitting unit
    long src_id = stat_node.get<long>("srcId", 0);
    if (src_id > 0) {
      std::string unit_tag = sys->find_unit_tag(src_id);
      field_content << "**Current Unit:** " << src_id;
      if (!unit_tag.empty()) {
        field_content << " (" << unit_tag << ")";
      }
      field_content << "\n";
    }

    // Get transmissions to show all participating units
    std::vector<Transmission> transmissions = call->get_transmissions();
    if (!transmissions.empty() && transmissions.size() > 1) {
      // Collect unique source IDs (excluding current unit to avoid duplication)
      std::set<long> unique_sources;
      for (const auto &tx : transmissions) {
        if (tx.source > 0 && tx.source != src_id) {
          unique_sources.insert(tx.source);
        }
      }
      
      if (!unique_sources.empty()) {
        field_content << "**Other Units:** ";
        int count = 0;
        for (auto src : unique_sources) {
          if (count > 0) field_content << ", ";
          field_content << src;
          if (++count >= 4) break;
        }
        if (unique_sources.size() > 4) {
          field_content << " (+" << (unique_sources.size() - 4) << " more)";
        }
        field_content << "\n";
      }
    }

    // Show patches if applicable
    std::vector<unsigned long> patches = sys->get_talkgroup_patch(talkgroup_num);
    if (!patches.empty()) {
      field_content << "**Patched:** ";
      for (size_t i = 0; i < patches.size() && i < 3; i++) {
        if (i > 0) field_content << ", ";
        field_content << patches[i];
      }
      if (patches.size() > 3) {
        field_content << " (+" << (patches.size() - 3) << " more)";
      }
      field_content << "\n";
    }

    // Add field to embed (max 25 fields, inline for compact view)
    if (active_count <= 25) {
      active_embed.add_field(field_title.str(), field_content.str(), true);
    }
  }

  // Add summary footer
  if (calls_found) {
    std::ostringstream footer_text;
    footer_text << active_count << " active call";
    if (active_count != 1) footer_text << "s";
    if (!system_filter.empty()) {
      footer_text << " on " << system_filter;
    }
    active_embed.set_footer(dpp::embed_footer().set_text(footer_text.str()));
  } else {
    active_embed
        .set_description("No active calls at this time.")
        .set_color(dpp::colors::gray);
  }

  // Send the response
  event.reply(dpp::message().add_embed(active_embed));
}
