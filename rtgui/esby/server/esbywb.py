#!/usr/bin/env python3
#
#  This file is part of the esby fork of RawTherapee.
#
#  RawTherapee is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
"""esbywb: series white balance server and command line tool (step 2 of SPEC_series_wb.md).

The server keeps, per folder, a white balance shift (mireds, tint factor) that the folders
below inherit. RawTherapee (TTSeriesWB) and this command line tool talk to it through a local
Unix socket, with one JSON message per line.

    esbywb serve                             start the server
    esbywb list                              rules
    esbywb get  <folder>                     resolved value and its source
    esbywb set  <folder> <mired> [--green G] [--all] [--comment TEXT]
    esbywb unset <folder>                    back to the inherited value
    esbywb move <old> <new>                  after renaming a folder
    esbywb orphans                           rules whose folder does not exist anymore

Only the Python standard library is used.
"""

import argparse
import asyncio
import json
import os
import signal
import socket
import sys
import tempfile

PROTOCOL_VERSION = 1

# equal: factor applied to the blue/red equalizer of the white balance (1.0 for the camera)
DEFAULT_RULE = {"mired": 0.0, "green": 1.0, "equal": 1.0, "flash_only": True, "comment": ""}

# ranges of the TTSeriesWB tool: a value outside them is refused
LIMITS = {"mired": (-100.0, 100.0), "green": (0.5, 2.0), "equal": (0.5, 2.0)}


def check_limits(**values):
    for name, value in values.items():
        low, high = LIMITS[name]
        if not (low <= float(value) <= high):
            raise ValueError("%s %s out of range [%s, %s]" % (name, value, low, high))


# ---------------------------------------------------------------------------------------
# locations
# ---------------------------------------------------------------------------------------

def default_socket_path():
    runtime = os.environ.get("XDG_RUNTIME_DIR")
    if runtime:
        return os.path.join(runtime, "esby-wb.sock")
    return os.path.join(tempfile.gettempdir(), "esby-wb-%d.sock" % os.getuid())


def default_state_path():
    data = os.environ.get("XDG_DATA_HOME") or os.path.join(os.path.expanduser("~"), ".local", "share")
    return os.path.join(data, "esby-wb", "state.json")


def normalize(path):
    """absolute path, symbolic links resolved, no trailing slash (the root stays '/')"""
    path = os.path.realpath(os.path.expanduser(path))
    if len(path) > 1:
        path = path.rstrip("/")
    return path


def parents(path):
    """the path itself, then its parent folders up to the root"""
    while True:
        yield path
        parent = os.path.dirname(path)
        if parent == path:
            return
        path = parent


def is_below(path, folder):
    """path is the folder itself or inside it"""
    return path == folder or folder == "/" or path.startswith(folder + "/")


# ---------------------------------------------------------------------------------------
# state: rules per folder and white balance applied per file
# ---------------------------------------------------------------------------------------

class State:
    def __init__(self, filename):
        self.filename = filename
        self.rules = {}   # folder -> rule
        self.files = {}   # file -> {"T": int, "G": float, "source": folder or None}
        self.load()

    def load(self):
        try:
            with open(self.filename, "r", encoding="utf-8") as f:
                data = json.load(f)
        except FileNotFoundError:
            return
        self.rules = data.get("rules", {})
        self.files = data.get("files", {})

    def save(self):
        """atomic write: temporary file in the same folder, then rename"""
        folder = os.path.dirname(self.filename)
        os.makedirs(folder, exist_ok=True)
        fd, tmp = tempfile.mkstemp(prefix=".state-", dir=folder)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                json.dump({"version": PROTOCOL_VERSION, "rules": self.rules, "files": self.files},
                          f, indent=1, sort_keys=True, ensure_ascii=False)
            os.replace(tmp, self.filename)
        except BaseException:
            try:
                os.unlink(tmp)
            except OSError:
                pass
            raise

    def resolve(self, folder):
        """value of the closest folder having a rule, or the default value (source None)"""
        folder = normalize(folder)
        for p in parents(folder):
            if p in self.rules:
                rule = dict(DEFAULT_RULE)
                rule.update(self.rules[p])
                rule["source"] = p
                return rule
        rule = dict(DEFAULT_RULE)
        rule["source"] = None
        return rule

    def set_rule(self, folder, mired, green=1.0, flash_only=True, comment="", equal=1.0):
        check_limits(mired=mired, green=green, equal=equal)
        folder = normalize(folder)
        self.rules[folder] = {"mired": float(mired), "green": float(green), "equal": float(equal),
                              "flash_only": bool(flash_only), "comment": str(comment)}
        self.save()
        return folder

    def unset_rule(self, folder):
        folder = normalize(folder)
        if folder not in self.rules:
            raise KeyError("no rule for " + folder)
        del self.rules[folder]
        self.save()
        return folder

    def move(self, old, new):
        """rules and followed files of old (and below) are moved to new"""
        old, new = normalize(old), normalize(new)

        def moved(path):
            return new + path[len(old):] if is_below(path, old) else path

        count = 0
        for table in (self.rules, self.files):
            for key in list(table):
                if is_below(key, old):
                    table[moved(key)] = table.pop(key)
                    count += 1
        for info in self.files.values():
            if info.get("source") and is_below(info["source"], old):
                info["source"] = moved(info["source"])
        self.save()
        return old, new, count

    def record_applied(self, filename, temperature, green, source, equal=1.0):
        filename = normalize(filename)
        self.files[filename] = {"T": int(temperature), "G": float(green), "E": float(equal), "source": source}
        self.save()

    def file_state(self, filename):
        return self.files.get(normalize(filename))


# ---------------------------------------------------------------------------------------
# server
# ---------------------------------------------------------------------------------------

class Server:
    def __init__(self, state, socket_path):
        self.state = state
        self.socket_path = socket_path
        self.subscribers = set()
        self.connections = set()   # writers of the open connections, closed by stop()
        self.handlers = set()      # tasks handling the connections, awaited by stop()
        self.server = None
        self.stopping = None       # shutdown task, shared by the callers of stop()

    async def broadcast(self, path):
        message = (json.dumps({"event": "rule_changed", "path": path}) + "\n").encode()
        for writer in list(self.subscribers):
            try:
                writer.write(message)
                await writer.drain()
            except (ConnectionError, OSError):
                self.subscribers.discard(writer)

    async def handle_request(self, request, writer):
        op = request.get("op")
        if op == "hello":
            return {"version": PROTOCOL_VERSION}
        if op == "get":
            return self.state.resolve(request["path"])
        if op == "set":
            folder = self.state.set_rule(request["path"], request["mired"], request.get("green", 1.0),
                                         request.get("flash_only", True), request.get("comment", ""),
                                         request.get("equal", 1.0))
            await self.broadcast(folder)
            return {"path": folder}
        if op == "unset":
            folder = self.state.unset_rule(request["path"])
            await self.broadcast(folder)
            return {"path": folder}
        if op == "list":
            return {"rules": [dict(rule, path=path) for path, rule in sorted(self.state.rules.items())]}
        if op == "move":
            old, new, count = self.state.move(request["from"], request["to"])
            await self.broadcast(old)
            await self.broadcast(new)
            return {"from": old, "to": new, "moved": count}
        if op == "applied":
            self.state.record_applied(request["file"], request["T"], request["G"], request.get("source"),
                                      request.get("E", 1.0))
            return {}
        if op == "file_state":
            return {"state": self.state.file_state(request["file"])}
        if op == "subscribe":
            self.subscribers.add(writer)
            return {}
        raise ValueError("unknown operation: %r" % op)

    async def handle_client(self, reader, writer):
        self.connections.add(writer)
        self.handlers.add(asyncio.current_task())
        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                request_id = None
                try:
                    request = json.loads(line)
                    request_id = request.get("id")
                    answer = await self.handle_request(request, writer)
                    answer.update({"id": request_id, "ok": True})
                except Exception as e:  # any error is sent back, the server keeps running
                    answer = {"id": request_id, "ok": False, "error": "%s: %s" % (type(e).__name__, e)}
                writer.write((json.dumps(answer) + "\n").encode())
                await writer.drain()
        except (ConnectionError, OSError):
            pass
        finally:
            self.subscribers.discard(writer)
            self.connections.discard(writer)
            self.handlers.discard(asyncio.current_task())
            writer.close()
            try:
                await writer.wait_closed()
            except (ConnectionError, OSError, asyncio.CancelledError):
                pass

    async def run(self, ready=None):
        prepare_socket(self.socket_path)
        self.server = await asyncio.start_unix_server(self.handle_client, path=self.socket_path)
        os.chmod(self.socket_path, 0o600)  # the user only
        # SIGTERM (systemctl stop) and SIGINT (Ctrl+C): clean shutdown, the socket file is removed.
        # Only possible in the main thread (the tests run the server in another thread).
        loop = asyncio.get_running_loop()
        for sig in (signal.SIGTERM, signal.SIGINT):
            try:
                loop.add_signal_handler(sig, lambda: asyncio.ensure_future(self.stop()))
            except (NotImplementedError, RuntimeError, ValueError):
                pass
        if ready is not None:
            ready.set()
        try:
            await self.server.serve_forever()
        except asyncio.CancelledError:
            pass
        finally:
            await self.stop()

    async def stop(self):
        """closes the listening socket, then the open connections, and waits for them.
        Every caller (run() and an explicit stop) waits for the same shutdown."""
        if self.stopping is None:
            self.stopping = asyncio.ensure_future(self._shutdown())
        await asyncio.shield(self.stopping)

    async def _shutdown(self):
        if self.server is None:
            return
        server = self.server
        server.close()
        # a connection accepted just before may not have started its handler yet: accepting it,
        # creating its transport and starting its handler take several iterations of the loop
        await asyncio.sleep(0.05)
        for writer in list(self.connections):
            writer.close()
        handlers = [task for task in self.handlers if task is not asyncio.current_task()]
        if handlers:
            await asyncio.gather(*handlers, return_exceptions=True)
        await server.wait_closed()
        try:
            os.unlink(self.socket_path)
        except FileNotFoundError:
            pass


def prepare_socket(path):
    """removes a stale socket file, refuses to start if a server already answers"""
    if not os.path.exists(path):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        return
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.connect(path)
    except (ConnectionRefusedError, FileNotFoundError):
        os.unlink(path)  # no server behind it
        return
    raise SystemExit("esbywb: a server is already running on " + path)


# ---------------------------------------------------------------------------------------
# command line client
# ---------------------------------------------------------------------------------------

class Client:
    def __init__(self, socket_path):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            self.sock.connect(socket_path)
        except (FileNotFoundError, ConnectionRefusedError):
            raise SystemExit("esbywb: no server on %s (start it with 'esbywb serve')" % socket_path)
        self.file = self.sock.makefile("rwb")
        self.next_id = 1

    def close(self):
        self.file.close()
        self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def request(self, op, **params):
        params.update({"op": op, "id": self.next_id})
        self.next_id += 1
        self.file.write((json.dumps(params) + "\n").encode())
        self.file.flush()
        while True:
            answer = json.loads(self.file.readline())
            if answer.get("id") == params["id"]:
                break
        if not answer.get("ok"):
            raise SystemExit("esbywb: " + answer.get("error", "error"))
        return answer


def format_rule(rule):
    mired = rule["mired"] if abs(rule["mired"]) >= 0.05 else 0.0  # no "-0.0"
    text = "%+.1f mireds, tint x%.3f, blue/red x%.3f" % (mired, rule["green"], rule.get("equal", 1.0))
    if not rule.get("flash_only", True):
        text += ", all photos"
    if rule.get("comment"):
        text += "  # " + rule["comment"]
    return text


def main(argv=None):
    parser = argparse.ArgumentParser(prog="esbywb", description=__doc__.split("\n\n")[0])
    parser.add_argument("--socket", default=default_socket_path(), help="socket path")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("serve", help="start the server")
    p.add_argument("--state", default=default_state_path(), help="state file")
    sub.add_parser("list", help="rules")
    p = sub.add_parser("get", help="resolved value of a folder")
    p.add_argument("folder")
    p = sub.add_parser("set", help="declare or change the value of a folder")
    p.add_argument("folder")
    p.add_argument("mired", type=float, help="shift in mireds (positive: cooler rendering)")
    p.add_argument("--green", type=float, default=1.0, help="tint factor (default 1.0)")
    p.add_argument("--equal", type=float, default=1.0, help="blue/red equalizer factor (default 1.0)")
    p.add_argument("--all", action="store_true", help="also for the photos without flash")
    p.add_argument("--comment", default="")
    p = sub.add_parser("unset", help="remove the value of a folder (back to the inherited one)")
    p.add_argument("folder")
    p = sub.add_parser("move", help="move the rules after renaming a folder")
    p.add_argument("old")
    p.add_argument("new")
    sub.add_parser("orphans", help="rules whose folder does not exist anymore")

    args = parser.parse_args(argv)

    if args.command == "serve":
        state = State(args.state)
        print("esbywb: serving on %s, state %s" % (args.socket, args.state), flush=True)
        try:
            asyncio.run(Server(state, args.socket).run())
        except KeyboardInterrupt:
            pass
        return 0

    with Client(args.socket) as client:
        return run_command(client, args)


def run_command(client, args):
    if args.command == "list":
        for rule in client.request("list")["rules"]:
            print("%-60s %s" % (rule["path"], format_rule(rule)))
    elif args.command == "get":
        rule = client.request("get", path=normalize(args.folder))
        source = rule["source"] or "(default)"
        print("%s\n  source: %s" % (format_rule(rule), source))
    elif args.command == "set":
        answer = client.request("set", path=normalize(args.folder), mired=args.mired, green=args.green,
                                equal=args.equal, flash_only=not args.all, comment=args.comment)
        print("set: " + answer["path"])
    elif args.command == "unset":
        print("unset: " + client.request("unset", path=normalize(args.folder))["path"])
    elif args.command == "move":
        answer = client.request("move", **{"from": normalize(args.old), "to": normalize(args.new)})
        print("moved %d entries from %s to %s" % (answer["moved"], answer["from"], answer["to"]))
    elif args.command == "orphans":
        for rule in client.request("list")["rules"]:
            if not os.path.isdir(rule["path"]):
                print(rule["path"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
