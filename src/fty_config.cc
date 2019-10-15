/*  =========================================================================
    fty_config - Binary

    Copyright (C) 2014 - 2018 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
 */

/*
@header
    fty_config - Binary
@discuss
@end
 */

#include <csignal>

#include "fty_config_classes.h"

#include "fty_common_mlm_zconfig.h"

//functions

void usage();
volatile bool g_compute = true;

void sigHandler(int s)
{
    log_debug("Caught signal %d\n", s);
    g_compute = false;
}

/**
 * Set Signal handler
 */
void setSignalHandler()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
}

int main(int argc, char *argv [])
{
    using Parameters = std::map<std::string, std::string>;
    Parameters paramsConfig;

    // Set signal handler
    setSignalHandler();
    
    try
    {
        ftylog_setInstance(AGENT_NAME, "");
        
        int argn;
        char *config_file = NULL;
        bool verbose = false;
        // Parse command line
        for (argn = 1; argn < argc; argn++)
        {
            char *param = NULL;
            if (argn < argc - 1) param = argv [argn + 1];

            if (streq(argv [argn], "--help")
                    || streq(argv [argn], "-h"))
            {
                usage();
                return EXIT_SUCCESS;
            } else if (streq(argv [argn], "--verbose") || streq(argv [argn], "-v"))
            {
                verbose = true;
            } else if (streq(argv [argn], "--config") || streq(argv [argn], "-c"))
            {
                if (param) config_file = param;
                ++argn;
            }
        }

        // Default configuration.
        paramsConfig[ENDPOINT_KEY] = DEFAULT_ENDPOINT;
        paramsConfig[AGENT_NAME_KEY] = AGENT_NAME;
        paramsConfig[QUEUE_NAME_KEY] = MSG_QUEUE_NAME;
        // Default configuration files path.
        paramsConfig[MONITORING_FEATURE_NAME] = "/etc/fty-nut/fty-nut.cfg";
        paramsConfig[NOTIFICATION_FEATURE_NAME] = "/etc/fty-email/fty-email.cfg";
        paramsConfig[AUTOMATION_SETTINGS] = "/etc/etn-automation/etn-automation.cfg";
        paramsConfig[USER_SESSION_FEATURE_NAME] = "/etc/fty/fty-session.cfg";
        paramsConfig[DISCOVERY] = "/etc/fty-discovery/fty-discovery.cfg";
        paramsConfig[GENERAL_CONFIG] = "/etc/default/fty.cfg";
        paramsConfig[NETWORK] = "/etc/network/interfaces";
        // Default augeas configuration.
        paramsConfig[AUGEAS_LENS_PATH] = "/usr/share/fty/lenses/";
        paramsConfig[AUGEAS_OPTIONS] = AUG_NONE;

        if (config_file)
        {
            log_debug(AGENT_NAME ": loading configuration file from '%s' ...", config_file);
            mlm::ZConfig config(config_file);
            // Message bus configuration.
            paramsConfig[ENDPOINT_KEY] = config.getEntry("srr-msg-bus/endpoint", DEFAULT_ENDPOINT);
            paramsConfig[QUEUE_NAME_KEY] = config.getEntry("srr-msg-bus/queueName", MSG_QUEUE_NAME);
            // Configuration file path
            paramsConfig[MONITORING_FEATURE_NAME] = config.getEntry("available_features/monitoring", "");
            paramsConfig[NOTIFICATION_FEATURE_NAME] = config.getEntry("available_features/notification", "");
            paramsConfig[AUTOMATION_SETTINGS] = config.getEntry("available_features/automationSettings", "");
            paramsConfig[USER_SESSION_FEATURE_NAME] = config.getEntry("available_features/user-session", "");
            paramsConfig[DISCOVERY] = config.getEntry("available_features/discovery", "");
            paramsConfig[GENERAL_CONFIG] = config.getEntry("available_features/generalConfig", "");
            paramsConfig[NETWORK] = config.getEntry("available_features/network", "");
            // Augeas configuration
            paramsConfig[AUGEAS_LENS_PATH] = config.getEntry("augeas/lensPath", "/usr/share/fty/lenses/");
            paramsConfig[AUGEAS_OPTIONS] = config.getEntry("augeas/augeasOptions", "0");
        }

        if (verbose)
        {
            ftylog_setVeboseMode(ftylog_getInstance());
            log_trace("Verbose mode OK");
        }

        log_info(AGENT_NAME " starting");

        // Start config agent
        config::ConfigurationManager configManager(paramsConfig);

        while (g_compute == true) 
        {
            std::this_thread::sleep_for (std::chrono::seconds(1));
        }

        log_info(AGENT_NAME " Interrupted ...");

        // Exit application
        return EXIT_SUCCESS;
    } catch (std::exception & e)
    {
        log_error(AGENT_NAME ": Error '%s'", e.what());
        exit(EXIT_FAILURE);
    } catch (...)
    {
        log_error(AGENT_NAME ": Error unknown");
        exit(EXIT_FAILURE);
    }
}

void usage()
{
    puts(AGENT_NAME " [options] ...");
    puts("  -v|--verbose        verbose test output");
    puts("  -h|--help           this information");
    puts("  -c|--config         path to config file");
}