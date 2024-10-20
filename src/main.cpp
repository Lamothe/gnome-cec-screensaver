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

void SetTVState(bool on)
{
    CEC::ICECAdapter *parser;
    CEC::ICECCallbacks callbacks;
    CEC::libcec_configuration config;

    callbacks.Clear();
    callbacks.logMessage = &CecLogMessage;
    callbacks.keyPress = &CecKeyPress;
    callbacks.commandReceived = &CecCommand;
    callbacks.alert = &CecAlert;

    config.Clear();
    snprintf(config.strDeviceName, LIBCEC_OSD_NAME_SIZE, "GNOME CEC");
    config.clientVersion = CEC::LIBCEC_VERSION_CURRENT;
    config.bActivateSource = 0;
    config.callbacks = &callbacks;

    config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

    cout << "Loading libcec" << endl;
    parser = LibCecInitialise(&config);

    if (parser == NULL)
    {
        cout << "Failed to initialise CEC library" << endl;
    }
    cout << "CEC parser initialised" << endl;

    parser->InitVideoStandalone();

    CEC::cec_adapter_descriptor devices[10];
    uint8_t deviceCount = parser->DetectAdapters(devices, 10, NULL, true);
    if (deviceCount <= 0)
    {
        cout << "No devices found" << endl;
    }
    else
    {
        cout << "Devices" << endl;

        for (int i = 0; i < deviceCount; i++)
        {
            cout << i << ": " << devices[i].strComName << endl;
        }
        string port = devices[0].strComName;

        cout << "Opening device " << port << endl;

        if (!parser->Open(port.c_str()))
        {
            cout << "Failed to open device" << endl;
        }
        else
        {
            if (on)
            {
                cout << "Sending power on ... ";
                parser->PowerOnDevices((CEC::cec_logical_address)0);
            }
            else
            {
                cout << "Sending standby ... ";
                parser->StandbyDevices((CEC::cec_logical_address)0);

                // For some reason my Sony Bravia TV turns back on if I don't set this.
                cout << "Sleeping for 10 seconds" << endl;
                g_usleep(10000);
            }

            cout << "complete" << endl;
        }
    }

    parser->Close();

    UnloadLibCec(parser);
    cout << "CEC parser unloaded" << endl;
}

static void on_active_changed(
    GDBusProxy *proxy,
    const gchar *sender_name,
    const gchar *signal_name,
    GVariant *parameter,
    gpointer user_data)
{
    if (g_strcmp0(signal_name, "ActiveChanged") != 0)
    {
        return;
    }

    gboolean active;
    g_variant_get(parameter, "(b)", &active);
    cout << "Screensaver state changed: " << (active ? "Active" : "Not active") << endl;

    // Turn TV off if screensaver is active and turn it on when not active.
    SetTVState(!active);
}

int main(int argc, char *argv[])
{
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    GError *error = nullptr;

    cout << "GNOME CEC Screensaver" << endl;

    // Connect to the session bus
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error)
    {
        cerr << "Error connecting to session bus: " << error->message << endl;
        g_error_free(error);
        return 1;
    }

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
        return 1;
    }

    // Connect to the "ActiveChanged" signal
    g_signal_connect(proxy, "g-signal", G_CALLBACK(on_active_changed), nullptr);

    cout << "Listening for screensaver events" << endl;

    // Start the main loop
    g_main_loop_run(loop);

    // Clean up
    g_object_unref(proxy);
    g_object_unref(connection);
    g_main_loop_unref(loop);

    return 0;
}
