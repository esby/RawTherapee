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
#include "esbywbclient.h"
#include "ttlog.h"
#include <cjson/cJSON.h>
#include <giomm/unixsocketaddress.h>
#include <unistd.h>
#include <sstream>
#include <vector>

// seconds between two connection attempts when the server is not available
static const unsigned int ESBYWB_RETRY_SECONDS = 10;

EsbyWBClient::EsbyWBClient() :
    alive(std::make_shared<bool>(true)),
    connected(false),
    connecting(false),
    nextId(1)
{
}

EsbyWBClient::~EsbyWBClient()
{
    *alive = false;
    retryConnection.disconnect();
    if (cancellable)
        cancellable->cancel();
    if (connection)
    {
        try
        {
            connection->close();
        }
        catch (const Glib::Error&) {}
    }
}

// same rule as esbywb.py: $XDG_RUNTIME_DIR/esby-wb.sock, or <tmp>/esby-wb-<uid>.sock
Glib::ustring EsbyWBClient::socketPath()
{
    const char* runtime = g_getenv("XDG_RUNTIME_DIR");
    if (runtime && *runtime)
        return Glib::build_filename(runtime, "esby-wb.sock");
    return Glib::build_filename(Glib::get_tmp_dir(), Glib::ustring::compose("esby-wb-%1.sock", getuid()));
}

void EsbyWBClient::start()
{
    if (!connected && !connecting)
        connect();
}

void EsbyWBClient::connect()
{
    if (!Glib::file_test(socketPath(), Glib::FILE_TEST_EXISTS))
    {
        scheduleRetry();
        return;
    }
    connecting = true;
    cancellable = Gio::Cancellable::create();
    socketClient = Gio::SocketClient::create();
    std::shared_ptr<bool> isAlive = alive;
    socketClient->connect_async(Gio::UnixSocketAddress::create(socketPath()), cancellable,
        [this, isAlive](Glib::RefPtr<Gio::AsyncResult>& result) {
            if (*isAlive)
                connected_cb(result);
        });
}

void EsbyWBClient::connected_cb(Glib::RefPtr<Gio::AsyncResult>& result)
{
    connecting = false;
    try
    {
        connection = socketClient->connect_finish(result);
    }
    catch (const Glib::Error& e)
    {
        TT_LOG("esbywb: connection failed: %s\n", e.what().c_str());
        connection.reset();
        scheduleRetry();
        return;
    }
    input = Gio::DataInputStream::create(connection->get_input_stream());
    output = connection->get_output_stream();
    connected = true;
    TT_LOG("esbywb: connected to %s\n", socketPath().c_str());

    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "subscribe");
    send(request, nullptr);
    readNext();

    if (onStatus)
        onStatus(true);
}

void EsbyWBClient::scheduleRetry()
{
    retryConnection.disconnect();
    std::shared_ptr<bool> isAlive = alive;
    // connect_seconds_once() returns nothing: a one shot slot is emulated by returning false
    retryConnection = Glib::signal_timeout().connect_seconds([this, isAlive]() {
        if (*isAlive && !connected && !connecting)
            connect();
        return false;
    }, ESBYWB_RETRY_SECONDS);
}

void EsbyWBClient::disconnect()
{
    bool wasConnected = connected;
    connected = false;
    if (connection)
    {
        try
        {
            connection->close();
        }
        catch (const Glib::Error&) {}
    }
    connection.reset();
    input.reset();
    output.reset();

    // the pending requests fail
    std::map<int, AnswerCallback> failed;
    failed.swap(pending);
    for (auto& entry : failed)
    {
        if (entry.second)
            entry.second(nullptr);
    }

    if (wasConnected && onStatus)
        onStatus(false);
    scheduleRetry();
}

void EsbyWBClient::readNext()
{
    if (!input)
        return;
    std::shared_ptr<bool> isAlive = alive;
    input->read_line_async([this, isAlive](Glib::RefPtr<Gio::AsyncResult>& result) {
        if (*isAlive)
            line_cb(result);
    }, cancellable);
}

void EsbyWBClient::line_cb(Glib::RefPtr<Gio::AsyncResult>& result)
{
    std::string line;
    bool ok = false;
    try
    {
        ok = input && input->read_line_finish(result, line);
    }
    catch (const Glib::Error& e)
    {
        TT_LOG("esbywb: read error: %s\n", e.what().c_str());
        ok = false;
    }
    if (!ok) // end of stream: the server stopped
    {
        TT_LOG("esbywb: disconnected\n");
        disconnect();
        return;
    }
    handleLine(line);
    readNext();
}

void EsbyWBClient::handleLine(const std::string& line)
{
    cJSON* message = cJSON_Parse(line.c_str());
    if (message == nullptr)
        return;

    cJSON* event = cJSON_GetObjectItemCaseSensitive(message, "event");
    if (cJSON_IsString(event))
    {
        cJSON* path = cJSON_GetObjectItemCaseSensitive(message, "path");
        if (onEvent && cJSON_IsString(path))
            onEvent(path->valuestring);
        cJSON_Delete(message);
        return;
    }

    cJSON* id = cJSON_GetObjectItemCaseSensitive(message, "id");
    if (cJSON_IsNumber(id))
    {
        auto it = pending.find(id->valueint);
        if (it != pending.end())
        {
            AnswerCallback callback = it->second;
            pending.erase(it);
            cJSON* ok = cJSON_GetObjectItemCaseSensitive(message, "ok");
            if (callback)
                callback(cJSON_IsTrue(ok) ? message : nullptr);
        }
    }
    cJSON_Delete(message);
}

// takes the ownership of request
void EsbyWBClient::send(cJSON* request, AnswerCallback callback)
{
    if (!connected || !output)
    {
        cJSON_Delete(request);
        if (callback)
            callback(nullptr);
        return;
    }
    int id = nextId++;
    cJSON_AddNumberToObject(request, "id", id);
    char* text = cJSON_PrintUnformatted(request);
    std::string line = std::string(text) + "\n";
    cJSON_free(text);
    cJSON_Delete(request);

    pending[id] = callback;
    try
    {
        // a short message on a local socket: the write does not wait in practice
        gsize written = 0;
        output->write_all(line, written);
    }
    catch (const Glib::Error& e)
    {
        TT_LOG("esbywb: write error: %s\n", e.what().c_str());
        disconnect();
    }
}

static Glib::ustring jsonString(cJSON* object, const char* name)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(object, name);
    return cJSON_IsString(item) ? Glib::ustring(item->valuestring) : Glib::ustring();
}

static double jsonNumber(cJSON* object, const char* name, double defaultValue)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(object, name);
    return cJSON_IsNumber(item) ? item->valuedouble : defaultValue;
}

static bool jsonBool(cJSON* object, const char* name, bool defaultValue)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(object, name);
    return cJSON_IsBool(item) ? cJSON_IsTrue(item) : defaultValue;
}

void EsbyWBClient::get(const Glib::ustring& folder, RuleCallback callback)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "get");
    cJSON_AddStringToObject(request, "path", folder.c_str());
    send(request, [callback](cJSON* answer) {
        EsbyWBRule rule;
        if (answer)
        {
            rule.mired = jsonNumber(answer, "mired", 0.0);
            rule.green = jsonNumber(answer, "green", 1.0);
            rule.equal = jsonNumber(answer, "equal", 1.0);
            rule.flashOnly = jsonBool(answer, "flash_only", true);
            rule.comment = jsonString(answer, "comment");
            rule.source = jsonString(answer, "source"); // null for the default value
            rule.mode = jsonString(answer, "mode");
            if (rule.mode.empty())
                rule.mode = "shift";
            rule.autoModel = jsonString(answer, "auto");
        }
        callback(answer != nullptr, rule);
    });
}

void EsbyWBClient::set(const Glib::ustring& folder, double mired, double green, double equal, bool flashOnly,
                       const Glib::ustring& mode, const Glib::ustring& autoModel, DoneCallback callback)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "set");
    cJSON_AddStringToObject(request, "path", folder.c_str());
    cJSON_AddNumberToObject(request, "mired", mired);
    cJSON_AddNumberToObject(request, "green", green);
    cJSON_AddNumberToObject(request, "equal", equal);
    cJSON_AddBoolToObject(request, "flash_only", flashOnly);
    cJSON_AddStringToObject(request, "mode", mode.c_str());
    cJSON_AddStringToObject(request, "auto", autoModel.c_str());
    send(request, [callback](cJSON* answer) {
        if (callback)
            callback(answer != nullptr, answer ? Glib::ustring() : Glib::ustring("failed"));
    });
}

void EsbyWBClient::unset(const Glib::ustring& folder, DoneCallback callback)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "unset");
    cJSON_AddStringToObject(request, "path", folder.c_str());
    send(request, [callback](cJSON* answer) {
        if (callback)
            callback(answer != nullptr, answer ? Glib::ustring() : Glib::ustring("failed"));
    });
}

void EsbyWBClient::fileState(const Glib::ustring& file, FileCallback callback)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "file_state");
    cJSON_AddStringToObject(request, "file", file.c_str());
    send(request, [callback](cJSON* answer) {
        EsbyWBFileState state;
        if (answer)
        {
            cJSON* s = cJSON_GetObjectItemCaseSensitive(answer, "state");
            if (cJSON_IsObject(s))
            {
                state.known = true;
                state.temperature = (int) jsonNumber(s, "T", 0);
                state.green = jsonNumber(s, "G", 1.0);
                state.equal = jsonNumber(s, "E", 1.0);
                state.source = jsonString(s, "source");
            }
        }
        callback(answer != nullptr, state);
    });
}

void EsbyWBClient::applied(const Glib::ustring& file, int temperature, double green, double equal, const Glib::ustring& source)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "applied");
    cJSON_AddStringToObject(request, "file", file.c_str());
    cJSON_AddNumberToObject(request, "T", temperature);
    cJSON_AddNumberToObject(request, "G", green);
    cJSON_AddNumberToObject(request, "E", equal);
    if (source.empty())
        cJSON_AddNullToObject(request, "source");
    else
        cJSON_AddStringToObject(request, "source", source.c_str());
    send(request, nullptr);
}

void EsbyWBClient::observe(const Glib::ustring& file, const std::map<std::string, double>& numbers,
                           const std::map<std::string, Glib::ustring>& texts)
{
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "op", "observe");
    cJSON_AddStringToObject(request, "file", file.c_str());
    for (const auto& n : numbers)
        cJSON_AddNumberToObject(request, n.first.c_str(), n.second);
    for (const auto& t : texts)
        cJSON_AddStringToObject(request, t.first.c_str(), t.second.c_str());
    send(request, nullptr);
}

// refT;refG;refE;refShutter;k;hssT;hssG;hssE;sync;hasRef;hasHss (locale independent numbers)
Glib::ustring EsbyWBAuto::serialize() const
{
    const double values[] = {refT, refG, refE, refShutter, k, hssT, hssG, hssE, sync,
                             hasRef ? 1.0 : 0.0, hasHss ? 1.0 : 0.0};
    Glib::ustring text;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        if (i > 0)
            text += ";";
        text += Glib::Ascii::dtostr(values[i]);
    }
    return text;
}

EsbyWBAuto EsbyWBAuto::parse(const Glib::ustring& text)
{
    EsbyWBAuto a;
    std::vector<double> v;
    std::string part;
    std::istringstream list(text);
    while (std::getline(list, part, ';'))
        v.push_back(Glib::Ascii::strtod(part));
    if (v.size() < 11)
        return a; // empty or unknown: no reference
    a.refT = v[0]; a.refG = v[1]; a.refE = v[2]; a.refShutter = v[3]; a.k = v[4];
    a.hssT = v[5]; a.hssG = v[6]; a.hssE = v[7];
    a.sync = (v[8] > 0.0) ? v[8] : 1.0 / 250.0;
    a.hasRef = (v[9] != 0.0) && (a.refT > 0.0);
    a.hasHss = (v[10] != 0.0) && (a.hssT > 0.0);
    return a;
}
