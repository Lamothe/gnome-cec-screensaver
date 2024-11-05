/*
    GNOME CEC Screensaver

    Copyright (C) 2024 Michael Lamothe <michael.lamothe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see https://www.gnu.org/licenses/.
*/

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cectypes.h>
#include <cec.h>
#include <cecloader.h>
#include <giomm.h>
#include <glibmm.h>

using namespace std;
using namespace Glib;
using namespace Gio::DBus;

// Globals
CEC::ICECAdapter *cec = NULL;
CEC::ICECCallbacks callbacks;
CEC::libcec_configuration config;
auto is_screensaver_active = false;
const auto tz = TimeZone::create_local();

void log(const string &message)
{
    static ofstream log_file_stream;

    if (!log_file_stream.is_open())
    {
        log_file_stream.open(get_home_dir() + "/.gnome-cec-screensaver.log", ios::out | ios::trunc);

        if (!log_file_stream.is_open())
        {
            throw runtime_error("Failed to open log file");
        }
    }

    auto date_time_string = DateTime::create_now(tz).format_iso8601();
    auto formatted_message = date_time_string + " - " + message + "\n";
    cout << formatted_message;

    if (log_file_stream.is_open())
    {
        log_file_stream << formatted_message;
        log_file_stream.flush();
    }
}

void log(const exception &ex)
{
    log(string("Exception: ") + ex.what());
}

void cec_log_message(void *, const CEC::cec_log_message *message)
{
    if (message->level >= CEC::CEC_LOG_WARNING)
    {
        return;
    }

    log(message->message);
}

void destroy_cec()
{
    if (cec != NULL)
    {
        cec->Close();
        UnloadLibCec(cec);
        cec = NULL;
    }
}

void create_cec()
{
    try
    {
        if (cec != NULL)
        {
            throw runtime_error("CEC library has already been initialised");
        }

        cec = LibCecInitialise(&config);

        if (cec == NULL)
        {
            throw runtime_error("Failed to initialise CEC library");
        }

        cec->InitVideoStandalone();

        log("Detecting CEC adapters");
        CEC::cec_adapter_descriptor adapters[10];
        auto adapterCount = cec->DetectAdapters(adapters, 10, NULL, true);

        if (adapterCount <= 0)
        {
            throw runtime_error("No CEC adapters found");
        }

        // Display all the CEC adapters
        for (int i = 0; i < adapterCount; i++)
        {
            log(ustring::compose(" - [%1]: %2", i, adapters[i].strComName));
        }

        // Select the first adapter by default.
        auto port = adapters[0].strComName;

        log(string("Opening CEC adapter at ") + port);

        if (!cec->Open(port))
        {
            throw runtime_error("Failed to open CEC adapter");
        }
    }
    catch (const exception &e)
    {
        log(e);

        // If anything goes wrong, tear it all down so we know to try again later.
        destroy_cec();
    }
}

void cec_key_press(void *, const CEC::cec_keypress *_)
{
    // Not used.
}

void cec_command(void *, const CEC::cec_command *_)
{
    // Not used.
}

void cec_alert(void *, const CEC::libcec_alert type, const CEC::libcec_parameter parameter)
{
    switch (type)
    {
    case CEC::CEC_ALERT_CONNECTION_LOST:
        log("Alert: CEC_ALERT_CONNECTION_LOST");

        // Tear it all down so we know to retry later.
        destroy_cec();

        break;

    case CEC::CEC_ALERT_PHYSICAL_ADDRESS_ERROR:
        log("Alert: CEC_ALERT_PHYSICAL_ADDRESS_ERROR");

        if (parameter.paramType == CEC::CEC_PARAMETER_TYPE_STRING)
        {
            log(ustring::compose("Parameter: %1", (char *)parameter.paramData));
        }
        else
        {
            log("Unknown parameter type");
        }

        break;

    default:
        log(ustring::compose("Alert: %1", type));
        break;
    }
}

void on_screensaver_signal(const ustring &sender_name,
                           const ustring &signal_name,
                           const VariantContainerBase &parameters)
{
    try
    {
        if (signal_name != "ActiveChanged")
        {
            return;
        }

        // Get the first parameter which is a bool that indicates if the screensaver is active or not.
        is_screensaver_active = VariantBase::cast_dynamic<Variant<bool>>(parameters.get_child(0)).get();

        // Create the CEC adapter if it hasn't been already.
        if (cec == NULL)
        {
            create_cec();
        }

        // If cec is still NULL then we failed to create it, do nothing.
        if (cec != NULL)
        {
            // Turn TV off if screensaver is active and turn it on when not active.
            if (is_screensaver_active)
            {
                log("Sending standby");
                if (!cec->StandbyDevices(CEC::cec_logical_address::CECDEVICE_TV))
                {
                    throw runtime_error("Standby failed");
                }
            }
            else
            {
                log("Setting to default physical address.");
                cec->SetPhysicalAddress();

                log("Sending power on");
                cec->PowerOnDevices(CEC::cec_logical_address::CECDEVICE_TV);
            }
        }
    }
    catch (const exception &e)
    {
        log(e);

        destroy_cec();
    }
}

// Send a periodic standby to make sure that the TV is off if the screensaver is active.
bool on_timeout()
{
    try
    {
        if (cec != NULL)
        {
            auto power_status = cec->GetDevicePowerStatus(CEC::cec_logical_address::CECDEVICE_TV);
            auto is_at_standby = power_status == CEC::cec_power_status::CEC_POWER_STATUS_STANDBY || power_status == CEC::cec_power_status::CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY;

            if (is_screensaver_active && !is_at_standby)
            {
                log("Sending another standby");
                if (!cec->StandbyDevices(CEC::cec_logical_address::CECDEVICE_TV))
                {
                    throw runtime_error("Standby failed");
                }
            }
        }
    }
    catch (const exception &e)
    {
        log(e);
        destroy_cec();
    }

    return true;
}

int main()
{
    try
    {
        log("GNOME CEC Screensaver");

        // Initialise Glib/Gio
        Glib::init();
        Gio::init();

        // Setup CEC
        callbacks.Clear();
        callbacks.logMessage = &cec_log_message;
        callbacks.keyPress = &cec_key_press;
        callbacks.commandReceived = &cec_command;
        callbacks.alert = &cec_alert;

        config.Clear();
        snprintf(config.strDeviceName, LIBCEC_OSD_NAME_SIZE, Glib::get_host_name().c_str());
        config.clientVersion = CEC::LIBCEC_VERSION_CURRENT;
        config.bActivateSource = 0;
        config.callbacks = &callbacks;
        // config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);
        config.deviceTypes.types[0] = CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE;

        // Connect to the D-Bus session bus.
        auto connection = Connection::get_sync(BusType::SESSION);

        if (!connection)
        {
            throw runtime_error("Failed to create connection to the D-Bus session bus");
        }

        // Create a proxy for the ScreenSaver service
        auto screensaver_proxy = Proxy::create_sync(
            connection,               // D-Bus connection
            "org.gnome.ScreenSaver",  // Name of the remote object
            "/org/gnome/ScreenSaver", // Path
            "org.gnome.ScreenSaver"   // Interface
        );

        if (!screensaver_proxy)
        {
            throw runtime_error("Failed to create proxy");
        }

        // Connect to the screensaver event signal and set the event handler.
        screensaver_proxy->signal_signal().connect(sigc::ptr_fun(&on_screensaver_signal));

        // Periodically send standbys incase the the TV didn't obey previous attempts.
        signal_timeout().connect(sigc::ptr_fun(&on_timeout), 10000);

        log("Listening for screensaver events");

        // Start the main application loop.
        auto loop = MainLoop::create();
        loop->run();
    }
    catch (const exception &e)
    {
        log(e);
    }

    // Cleanup
    destroy_cec();

    return 0;
}
