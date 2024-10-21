#include <iostream>
#include <chrono>
#include <thread>
#include <cectypes.h>
#include <cec.h>
#include <cecloader.h>
#include <gio/gio.h>
#include <glib.h>

using namespace std;

void CecLogMessage(void *, const CEC::cec_log_message *message)
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

void CecKeyPress(void *, const CEC::cec_keypress *_)
{
}

void CecCommand(void *, const CEC::cec_command *_)
{
}

void CecAlert(void *, const CEC::libcec_alert type, const CEC::libcec_parameter _)
{
    switch (type)
    {
    case CEC::CEC_ALERT_CONNECTION_LOST:
        cout << "Connection lost" << endl;
        break;
    default:
        break;
    }
}

void SetTVState(CEC::ICECAdapter *adapter, bool on)
{
    if (on)
    {
        cout << "Sending power on" << endl;
        adapter->PowerOnDevices((CEC::cec_logical_address)0);
    }
    else
    {
        cout << "Sending standby" << endl;
        adapter->StandbyDevices((CEC::cec_logical_address)0);
    }
}

static void on_screensaver_signal(
    GDBusProxy *proxy,
    const gchar *sender_name,
    const gchar *signal_name,
    GVariant *parameter,
    CEC::ICECAdapter *adapter)
{
    if (g_strcmp0(signal_name, "ActiveChanged") != 0)
    {
        return;
    }

    gboolean active;
    g_variant_get(parameter, "(b)", &active);
    cout << "Screensaver state changed to " << (active ? "Active" : "Not active") << endl;

    // Turn TV off if screensaver is active and turn it on when not active.
    SetTVState(adapter, !active);
}

int main(int argc, char *argv[])
{
    string applicationName = "GNOME CEC Screensaver";

    cout << applicationName << endl;

    // Setup CEC
    CEC::ICECCallbacks callbacks;
    callbacks.Clear();
    callbacks.logMessage = &CecLogMessage;
    callbacks.keyPress = &CecKeyPress;
    callbacks.commandReceived = &CecCommand;
    callbacks.alert = &CecAlert;

    CEC::libcec_configuration config;
    config.Clear();
    snprintf(config.strDeviceName, LIBCEC_OSD_NAME_SIZE, applicationName.c_str());
    config.clientVersion = CEC::LIBCEC_VERSION_CURRENT;
    config.bActivateSource = 0;
    config.callbacks = &callbacks;

    config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

    cout << "Initialising CEC library" << endl;
    CEC::ICECAdapter *adapter = LibCecInitialise(&config);

    if (adapter == NULL)
    {
        cout << "Failed to initialise CEC library" << endl;
    }
    else
    {
        adapter->InitVideoStandalone();

        cout << "Detecting CEC adapters" << endl;
        CEC::cec_adapter_descriptor adapters[10];
        uint8_t adapterCount = adapter->DetectAdapters(adapters, 10, NULL, true);

        if (adapterCount <= 0)
        {
            cout << "No CEC adapters found" << endl;
        }
        else
        {
            for (int i = 0; i < adapterCount; i++)
            {
                cout << " - [" << i << "]: " << adapters[i].strComName << endl;
            }

            string port = adapters[0].strComName;

            cout << "Opening CEC adapter at " << port << endl;

            if (!adapter->Open(port.c_str()))
            {
                cout << "Failed to open CEC adapter" << endl;
            }
            else
            {
                GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
                GError *error = nullptr;

                // Connect to the session bus
                GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
                if (error)
                {
                    cerr << "Error connecting to session bus: " << error->message << endl;
                    g_error_free(error);
                }
                else
                {
                    // Create a proxy for the IdleMonitor service
                    GDBusProxy *proxy = g_dbus_proxy_new_sync(
                        connection,
                        (GDBusProxyFlags)(G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES),
                        nullptr,
                        "org.gnome.ScreenSaver",  // Name of the remote object
                        "/org/gnome/ScreenSaver", // Path
                        "org.gnome.ScreenSaver",  // Interface
                        nullptr,
                        &error);

                    if (error)
                    {
                        cerr << "Error creating proxy: " << error->message << endl;
                        g_error_free(error);
                        g_object_unref(connection);
                    }
                    else
                    {
                        // Connect to the screensaver event signal
                        g_signal_connect(proxy, "g-signal", G_CALLBACK(on_screensaver_signal), adapter);

                        cout << "Listening for screensaver events" << endl;

                        // Start the main loop
                        g_main_loop_run(loop);

                        g_object_unref(proxy);
                    }

                    g_object_unref(connection);
                }

                // Wait for main loop to complete
                g_main_loop_unref(loop);

                // Clean up
                adapter->Close();
            }
        }
    }

    UnloadLibCec(adapter);
    cout << "CEC library unloaded" << endl;

    return 0;
}
