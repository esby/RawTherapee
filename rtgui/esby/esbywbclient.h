/*
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <giomm.h>
#include <glibmm.h>

struct cJSON;

// value of a folder, resolved by the server (source: folder defining it, empty for the default value)
struct EsbyWBRule
{
    double mired = 0.0;
    double green = 1.0;
    bool flashOnly = true;
    Glib::ustring comment;
    Glib::ustring source;
};

// white balance applied to a file, as recorded by the server
struct EsbyWBFileState
{
    bool known = false;
    int temperature = 0;
    double green = 1.0;
    Glib::ustring source;
};

// client of the esbywb server (rtgui/esby/server/esbywb.py, step 3 of SPEC_series_wb.md).
// One connection on the local Unix socket, JSON lines. Everything is asynchronous and runs in
// the GLib main loop (the callbacks are called there): the interface never waits for the server.
// Without server, the requests fail at once, and a new connection is tried every 10 seconds.
class EsbyWBClient
{
public:
    using RuleCallback = std::function<void(bool ok, const EsbyWBRule& rule)>;
    using FileCallback = std::function<void(bool ok, const EsbyWBFileState& state)>;
    using DoneCallback = std::function<void(bool ok, const Glib::ustring& error)>;
    using EventCallback = std::function<void(const Glib::ustring& path)>;
    using StatusCallback = std::function<void(bool connected)>;

    EsbyWBClient();
    ~EsbyWBClient();

    void start();
    bool isConnected() const { return connected; }

    void get(const Glib::ustring& folder, RuleCallback callback);
    void set(const Glib::ustring& folder, double mired, double green, bool flashOnly, DoneCallback callback);
    void unset(const Glib::ustring& folder, DoneCallback callback);
    void fileState(const Glib::ustring& file, FileCallback callback);
    void applied(const Glib::ustring& file, int temperature, double green, const Glib::ustring& source);

    void setEventCallback(EventCallback callback) { onEvent = callback; }
    void setStatusCallback(StatusCallback callback) { onStatus = callback; }

    static Glib::ustring socketPath();

private:
    using AnswerCallback = std::function<void(cJSON* answer)>; // nullptr: failure

    void connect();
    void connected_cb(Glib::RefPtr<Gio::AsyncResult>& result);
    void readNext();
    void line_cb(Glib::RefPtr<Gio::AsyncResult>& result);
    void handleLine(const std::string& line);
    void send(cJSON* request, AnswerCallback callback);
    void disconnect();
    void scheduleRetry();

    // the asynchronous callbacks check it: the client may be destroyed while a request runs
    std::shared_ptr<bool> alive;

    Glib::RefPtr<Gio::SocketClient> socketClient;
    Glib::RefPtr<Gio::SocketConnection> connection;
    Glib::RefPtr<Gio::DataInputStream> input;
    Glib::RefPtr<Gio::OutputStream> output;
    Glib::RefPtr<Gio::Cancellable> cancellable;

    bool connected;
    bool connecting;
    int nextId;
    std::map<int, AnswerCallback> pending;
    sigc::connection retryConnection;

    EventCallback onEvent;
    StatusCallback onStatus;
};
