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
#include <chrono>
#include <thread>
#include <cectypes.h>
#include <cec.h>
#include <cecloader.h>
#include <giomm.h>
#include <glibmm.h>

using namespace std;

// Globals
CEC::ICECAdapter *cec = NULL;
CEC::ICECCallbacks callbacks;
CEC::libcec_configuration config;

void cec_log_message(void *, const CEC::cec_log_message *message)
{
    string level;

    if (message->level >= CEC::CEC_LOG_WARNING)
    {
        return;
    }

    switch (message->level)
    {
    case CEC::CEC_LOG_ERROR:
        level = "ERROR:   ";
        break;
    case CEC::CEC_LOG_WARNING:
        level = "WARNING: ";
        break;
    case CEC::CEC_LOG_NOTICE:
        level = "NOTICE:  ";
        break;
    case CEC::CEC_LOG_TRAFFIC:
        level = "TRAFFIC: ";
        break;
    case CEC::CEC_LOG_DEBUG:
        level = "DEBUG:   ";
        break;
    default:
        break;
    }

    cout << level << ": " << message->time << " - " << message->message << endl;
}

void cec_key_press(void *, const CEC::cec_keypress *_)
{
    // Not used.
}

void cec_command(void *, const CEC::cec_command *_)
{
    // Not used.
}

void cec_alert(void *, const CEC::libcec_alert type, const CEC::libcec_parameter _)
{
    switch (type)
    {
    case CEC::CEC_ALERT_CONNECTION_LOST:
        cout << "Connection lost" << endl;

        // Tear it all down so we know to retry later.
        destroy_cec();

        break;
    default:
        break;
    }
}

void on_signal_received(const Glib::ustring &sender_name,
                        const Glib::ustring &signal_name,
                        const Glib::VariantContainerBase &parameters)
{
    try
    {
        if (signal_name != "ActiveChanged")
        {
            return;
        }

        // Get the first parameter which is the bool which indicates if the screensaver is active or not.
        auto screensaverActive = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(parameters.get_child(0)).get();

        // Create the CEC adapter if it hasn't been already.
        if (cec == NULL)
        {
            create_cec();
        }

        // If the cecAdapter is still NULL then we failed to create it, do nothing.
        if (cec != NULL)
        {
            // Turn TV off if screensaver is active and turn it on when not active.
            if (screensaverActive)
            {
                cout << "Sending standby" << endl;
                cec->StandbyDevices((CEC::cec_logical_address)0);
            }
            else
            {
                cout << "Sending power on" << endl;
                cec->PowerOnDevices((CEC::cec_logical_address)0);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << endl;

        destroy_cec();
    }
}

void create_cec()
{
    if (cec != NULL)
    {
        throw runtime_error("CEC library has already been initialised");
    }

    cec = LibCecInitialise(&config);

    try
    {
        if (cec == NULL)
        {
            throw runtime_error("Failed to initialise CEC library");
        }

        cec->InitVideoStandalone();

        cout << "Detecting CEC adapters" << endl;
        CEC::cec_adapter_descriptor adapters[10];
        auto adapterCount = cec->DetectAdapters(adapters, 10, NULL, true);

        if (adapterCount <= 0)
        {
            throw runtime_error("No CEC adapters found");
        }

        // Display all the CEC adapters
        for (int i = 0; i < adapterCount; i++)
        {
            cout << " - [" << i << "]: " << adapters[i].strComName << endl;
        }

        // Select the first adapter by default.
        auto port = adapters[0].strComName;

        cout << "Opening CEC adapter at " << port << endl;

        if (!cec->Open(port))
        {
            throw runtime_error("Failed to open CEC adapter");
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;

        // If anything goes wrong, tear it all down so we know to try again later.
        destroy_cec();
    }
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

int main()
{
    cout << "GNOME CEC Screensaver" << endl;

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
    config.bActivateSource = 1;
    config.callbacks = &callbacks;

    config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

    try
    {
        // Connect to the D-Bus session bus.
        auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);

        if (!connection)
        {
            throw runtime_error("Failed to create connection to the ");
        }

        // Create a proxy for the ScreenSaver service
        auto proxy = Gio::DBus::Proxy::create_sync(
            connection,               // D-Bus connection
            "org.gnome.ScreenSaver",  // Name of the remote object
            "/org/gnome/ScreenSaver", // Path
            "org.gnome.ScreenSaver"   // Interface
        );

        if (!proxy)
        {
            throw runtime_error("Failed to create proxy");
        }

        // Connect to the screensaver event signal and set the event handler.
        proxy->signal_signal().connect(sigc::ptr_fun(&on_signal_received));

        cout << "Listening for screensaver events" << endl;

        // Start the main application loop.
        auto loop = Glib::MainLoop::create();
        loop->run();
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    // Cleanup
    destroy_cec();

    return 0;
}
