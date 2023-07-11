# Trunk Recorder MQTT Discord Webhook Plugin <!-- omit from toc -->

This is a plugin for Trunk Recorder that publish completed calls to a discord webhook.

- [Install](#install)
- [Generating Webhooks](#generating-webhooks)
- [Configure](#configure)

## Install

1. **Build and install the current version of Trunk Recorder** following these [instructions](https://github.com/robotastic/trunk-recorder/blob/master/docs/INSTALL-LINUX.md). Make sure you do a `sudo make install` at the end to install the Trunk Recorder binary and libaries systemwide. The plugin will be built against these libraries.

2. **Build and install the plugin:**

&emsp; Plugin repos should be cloned in a location other than the trunk-recorder source dir.

```bash
mkdir build
cd build
cmake ..
sudo make install
```

&emsp; **IMPORTANT NOTE:** To avoid SEGFAULTs or other errors, plugins should be rebuilt after every new release of trunk-recorder.

## Generating Webhooks

You must be a server owner or have permissions to generate webhooks for a discord channel.  See [this link](https://support.discord.com/hc/en-us/articles/228383668-Intro-to-Webhooks) for instructions how to generate and configure a webhook.  Once that is done, take note of the URL for each webhook that has been created.

Webhooks do not need to be unique per-system.  The plugin will attempt send the system data to configured webhook URLs in the order they appear in the config, subject to any rate limiting that the Discord API may enforce.

&emsp; **IMPORTANT NOTE:** The plugin does not currently keep track of rate limits.  Try not to send more than ~5 messages per second.

## Configure

**Webhooks Object:**
| Key         | Required? | Default Value  | Type       | Description                                                      |
| ----------- | :-------: | :------------: | ---------- | ---------------------------------------------------------------- |
| description |           |                | string     | Descriptor field to identify the entry                           |
| event       |     ✓     |                | string     | Trunk-recorder event to monitor                                  |
| selector    |           |                | string     | Optional selectors for webhook (see below)                       |
| webhook     |     ✓     |                | int        | Webhook URL (redacted in system logs)                            |
| message     |           |                | string     | Optional text to display in the webhook                          |
| content     |           |                | string     | Optional text to display *above* the webhook; can use pings here |
| enabled     |           |      true      | true/false | Disable/enable individual webhooks if needed                     |
| username    |           | Trunk Recorder | string     | Username to publish the webhook message                          |
| avatar      |           |  (T-R Radio)   | string     | Avatar URL                                                       |
| color       |           |     (red)      | int        | Integer color code for message embeds                            |

**Events:**
| Event | Selector    | Selector Required? | Default     | Description                              |
| ----- | ----------- | ------------------ | ----------- | ---------------------------------------- |
| call  | `shortName` |                    | All Systems | Completed calls                          |
| rates | `shortName` |                    | All Systems | Message decode rates for digital systems |

Optional ping usage:
```
"content": "Ping @everone, user <@12345678>, and role group <@$23456789>",
```
Right-click on a user or a role and "Get ID" for the above id numbers.

**Plugin Usage:**

See the included [config.json](./config.json) for an example how to load this plugin.

```json
"name": "discord_webhook",
"library": "libdiscord_webhook_plugin.so",
"enabled": true,
"webhooks": [
    {
        "description": "county to #all-responders",
        "event": "call",
        "selector": "county",
        "webhook": "https://discord.com/api/webhooks/1125411831159201847/KVjwJ7xSvdU7gH_PQnLeTCqJ5h7mBMyHVcuQrNu3BLHvDnsGMY5eaaYSKGwDZBTrpA4i"
    },{
        "description": "PD to #just-PD",
        "event": "call", 
        "selector": "PD",
        "color": 255,
        "webhook": "https://discord.com/api/webhooks/1126190143980679768/nsyTAw8E50ZMBzEQ6_dtpgzOzfJDocQMNPKYPFji2IAGSHMovO7mjeJDi9QEIYcCSwt4"
    },{
        "description": "all calls!",
        "event": "call", 
        "webhook": "https://discord.com/api/webhooks/1126190471480388862/xhK4Yn5-cgTGbkUYvxu7kIn0SlGxyOmssraUVrPWsBPqr2okomR3roUQRHZ21vjP3oGR"
    },{
        "description": "rates to #tr-status",
        "enabled": false,
        "event": "rate",
        "webhook": "https://discord.com/api/webhooks/1125411831159201847/KVjwJ7xSvdU7gH_PQnLeTCqJ5h7mBMyHVcuQrNu3BLHvDnsGMY5eaaYSKGwDZBTrpA4i"
    }
]
```

If the plugin cannot be found, or it is being run from a different location, it may be necesarry to supply the full path:

```json
        "library": "/usr/local/lib/trunk-recorder/libdiscord_webhook_plugin.so",
```
