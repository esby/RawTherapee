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
#include <memory>
#include <string>
#include <giomm.h>
#include <glibmm.h>

struct cJSON;

// minimal asynchronous HTTP client for a local service (127.0.0.1): one POST with a JSON body, one
// connection per request (Connection: close). Everything runs in the GLib main loop: the interface
// never waits. The token, if any, is sent as "Authorization: Bearer <token>".
class EsbyHttpClient
{
public:
    // answer: the parsed JSON body (nullptr on failure: no service, HTTP error, invalid JSON);
    // it is deleted after the callback.
    using Callback = std::function<void(cJSON* answer, const Glib::ustring& error)>;

    EsbyHttpClient();
    ~EsbyHttpClient();

    void setAddress(const Glib::ustring& hostPort); // "127.0.0.1:8421", empty: disabled
    bool isEnabled() const { return port > 0; }
    void setToken(const Glib::ustring& t) { token = t; }

    // takes the ownership of body
    void post(const Glib::ustring& path, cJSON* body, Callback callback);

private:
    struct Request;
    void readMore(std::shared_ptr<Request> request);
    void finish(std::shared_ptr<Request> request, const Glib::ustring& error);

    std::shared_ptr<bool> alive; // the requests may end after the destruction of the client
    Glib::ustring host;
    int port;
    Glib::ustring token;
};
