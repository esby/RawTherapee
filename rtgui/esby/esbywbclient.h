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
#include <vector>
#include <giomm.h>
#include <glibmm.h>

struct cJSON;

// model of the auto mode of TTSeriesWB (fixed white balance of the series, exposure model):
// - normal flash: reference white balance (refT, refG, refE) at the shutter time refShutter, and the
//   exposure coefficient k (mireds per second of exposure: the ambient light adds up with the time);
// - high speed sync (shutter time <= sync): its own white balance (hssT, hssG, hssE), constant.
// A tint (refG, hssG) of 0 means the tint of the camera white balance.
struct EsbyWBAuto
{
    bool hasRef = false;
    double refT = 0.0, refG = 0.0, refE = 1.0, refShutter = 0.0;
    double k = 0.0;
    bool hasHss = false;
    double hssT = 0.0, hssG = 0.0, hssE = 1.0;
    double sync = 1.0 / 250.0;

    bool isHss(double shutter) const { return (shutter > 0.0) && (shutter <= sync * 1.05); }
    Glib::ustring serialize() const;
    static EsbyWBAuto parse(const Glib::ustring& text);
};

// value of a folder, resolved by the server (source: folder defining it, empty for the default value)
struct EsbyWBRule
{
    double mired = 0.0;
    double green = 1.0;
    double equal = 1.0; // factor of the blue/red equalizer
    bool flashOnly = true;
    Glib::ustring comment;
    Glib::ustring source;
    Glib::ustring mode = "shift"; // "shift" or "auto"
    Glib::ustring autoModel;      // EsbyWBAuto::serialize()
};

// white balance applied to a file, as recorded by the server
struct EsbyWBFileState
{
    bool known = false;
    int temperature = 0;
    double green = 1.0;
    double equal = 1.0;
    Glib::ustring source;
};

// shared variable, as resolved by the server (var_get): typed value, scope and origin
// type: RT_VARIABLE_TYPE_INT, _DOUBLE, _STRING or _BOOL (variable.h)
struct EsbyVarValue
{
    Glib::ustring name;
    int type = 2;
    int i = 0;
    double d = 0.0;
    Glib::ustring s;
    bool b = false;
    Glib::ustring scope;  // sequence, ancestor or global
    Glib::ustring origin; // key where it is set: folder, folder#sequence, or * (global)
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
    using VarsCallback = std::function<void(bool ok, const std::vector<EsbyVarValue>& variables)>;
    using VariableEventCallback = std::function<void(const Glib::ustring& path, const Glib::ustring& name)>;

    EsbyWBClient();
    ~EsbyWBClient();

    void start();
    bool isConnected() const { return connected; }

    void get(const Glib::ustring& folder, RuleCallback callback);
    void set(const Glib::ustring& folder, double mired, double green, double equal, bool flashOnly,
             const Glib::ustring& mode, const Glib::ustring& autoModel, DoneCallback callback);
    void unset(const Glib::ustring& folder, DoneCallback callback);
    void fileState(const Glib::ustring& file, FileCallback callback);
    void applied(const Glib::ustring& file, int temperature, double green, double equal, const Glib::ustring& source);
    // a correction observed by TTSeriesWB (learn, save), kept by the server for an analysis
    void observe(const Glib::ustring& file, const std::map<std::string, double>& numbers,
                 const std::map<std::string, Glib::ustring>& texts);

    // shared variables. path: a folder, or "global"; sequence: empty for the folder itself
    void varGet(const Glib::ustring& path, const Glib::ustring& sequence, VarsCallback callback);
    void varSet(const Glib::ustring& path, const Glib::ustring& sequence, const EsbyVarValue& value,
                DoneCallback callback);
    void varUnset(const Glib::ustring& path, const Glib::ustring& sequence, const Glib::ustring& name,
                  DoneCallback callback);

    // data kept by the server for a file, per namespace (ex: "exposure"); fileSet merges the fields,
    // a field of type RT_VARIABLE_TYPE_UNDEF is removed
    void fileGet(const Glib::ustring& file, const Glib::ustring& ns, VarsCallback callback);
    void fileSet(const Glib::ustring& file, const Glib::ustring& ns, const std::vector<EsbyVarValue>& fields,
                 DoneCallback callback);

    // rule_changed (white balance) and variable_changed events are given to different callbacks
    void setEventCallback(EventCallback callback) { onEvent = callback; }
    void setVariableEventCallback(VariableEventCallback callback) { onVariableEvent = callback; }
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
    VariableEventCallback onVariableEvent;
    StatusCallback onStatus;
};
