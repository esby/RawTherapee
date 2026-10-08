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
#include "esbyhttpclient.h"
#include "ttlog.h"
#include <cjson/cJSON.h>
#include <cstdlib>

struct EsbyHttpClient::Request
{
    Glib::RefPtr<Gio::SocketClient> socketClient;
    Glib::RefPtr<Gio::SocketConnection> connection;
    std::string payload;     // the whole request
    std::string response;    // what was read
    char buffer[8192];
    Callback callback;
    std::shared_ptr<bool> alive;
};

EsbyHttpClient::EsbyHttpClient() : alive(std::make_shared<bool>(true)), port(0)
{
}

EsbyHttpClient::~EsbyHttpClient()
{
    *alive = false;
}

void EsbyHttpClient::setAddress(const Glib::ustring& hostPort)
{
    host.clear();
    port = 0;
    Glib::ustring::size_type colon = hostPort.rfind(':');
    if (colon == Glib::ustring::npos)
        return;
    host = hostPort.substr(0, colon);
    port = atoi(hostPort.substr(colon + 1).c_str());
    if (host.empty() || (port <= 0) || (port > 65535))
    {
        host.clear();
        port = 0;
    }
}

void EsbyHttpClient::post(const Glib::ustring& path, cJSON* body, Callback callback)
{
    char* text = cJSON_PrintUnformatted(body);
    std::string json(text);
    cJSON_free(text);
    cJSON_Delete(body);

    if (!isEnabled())
    {
        if (callback)
            callback(nullptr, "no service");
        return;
    }

    auto request = std::make_shared<Request>();
    request->callback = callback;
    request->alive = alive;
    request->payload = "POST " + std::string(path) + " HTTP/1.1\r\n"
                     + "Host: " + std::string(host) + "\r\n"
                     + "Content-Type: application/json\r\n"
                     + "Content-Length: " + std::to_string(json.size()) + "\r\n"
                     + "Connection: close\r\n"
                     + (token.empty() ? std::string() : "Authorization: Bearer " + std::string(token) + "\r\n")
                     + "\r\n" + json;

    request->socketClient = Gio::SocketClient::create();
    request->socketClient->set_enable_proxy(false);
    request->socketClient->connect_to_host_async(host, port,
        [this, request](Glib::RefPtr<Gio::AsyncResult>& result) {
            if (!*request->alive)
                return;
            try
            {
                request->connection = request->socketClient->connect_to_host_finish(result);
                gsize written = 0;
                // a short request on a local socket: the write does not wait in practice
                request->connection->get_output_stream()->write_all(request->payload, written);
            }
            catch (const Glib::Error& e)
            {
                finish(request, e.what());
                return;
            }
            readMore(request);
        });
}

void EsbyHttpClient::readMore(std::shared_ptr<Request> request)
{
    request->connection->get_input_stream()->read_async(request->buffer, sizeof(request->buffer),
        [this, request](Glib::RefPtr<Gio::AsyncResult>& result) {
            if (!*request->alive)
                return;
            gssize count = 0;
            try
            {
                count = request->connection->get_input_stream()->read_finish(result);
            }
            catch (const Glib::Error& e)
            {
                finish(request, e.what());
                return;
            }
            if (count > 0)
            {
                request->response.append(request->buffer, count);
                readMore(request);
            }
            else
                finish(request, ""); // end of the answer (Connection: close)
        });
}

void EsbyHttpClient::finish(std::shared_ptr<Request> request, const Glib::ustring& error)
{
    if (request->connection)
    {
        try
        {
            request->connection->close();
        }
        catch (const Glib::Error&) {}
    }
    if (!request->callback)
        return;
    if (!error.empty())
    {
        TT_LOG("http: %s\n", error.c_str());
        request->callback(nullptr, error);
        return;
    }
    // status line, then the body after the empty line
    const std::string& r = request->response;
    std::string::size_type end = r.find("\r\n\r\n");
    int status = 0;
    if (r.compare(0, 5, "HTTP/") == 0)
    {
        std::string::size_type space = r.find(' ');
        if (space != std::string::npos)
            status = atoi(r.c_str() + space + 1);
    }
    if ((end == std::string::npos) || (status == 0))
    {
        request->callback(nullptr, "invalid answer");
        return;
    }
    cJSON* answer = cJSON_Parse(r.c_str() + end + 4);
    if (answer == nullptr)
    {
        request->callback(nullptr, Glib::ustring::compose("HTTP %1, no JSON", status));
        return;
    }
    // an HTTP error with a JSON body (ex: 401) is given to the caller, with the status as error
    request->callback(answer, (status >= 200 && status < 300) ? Glib::ustring() : Glib::ustring::compose("HTTP %1", status));
    cJSON_Delete(answer);
}
