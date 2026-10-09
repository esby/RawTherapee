#!/usr/bin/env python3
#
#  This file is part of the esby fork of RawTherapee.
#
#  RawTherapee is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
"""esbywb: esby server (series white balance, shared variables) and command line tool.

The server keeps, per folder, a white balance shift (mireds, tint factor) that the folders
below inherit, and named variables shared by every RawTherapee instance: a variable is set on
a sequence, on a folder (inherited by the folders below) or globally, the nearest one wins.
RawTherapee and this command line tool talk to it through a local Unix socket, with one JSON
message per line.

    esbywb serve                             start the server
    esbywb list                              rules
    esbywb get  <folder>                     resolved value and its source
    esbywb set  <folder> <mired> [--green G] [--all] [--comment TEXT]
    esbywb unset <folder>                    back to the inherited value
    esbywb move <old> <new>                  after renaming a folder
    esbywb orphans                           rules whose folder does not exist anymore
    esbywb observations [--exif [TAGS]]      corrections observed by TTSeriesWB, as CSV
    esbywb var-set <where> <name> <value> [--sequence S]   set a variable
    esbywb var-unset <where> <name> [--sequence S]         remove it (back to the inherited one)
    esbywb vars                              every variable set, where it is set
    esbywb show <folder> [--sequence S]      what applies to a folder: white balance and variables
    esbywb exposure-apply <folder> [--dry-run]   exposure of the closed images (TTSeriesExposure)

<where> is a folder, or "global" for every image. A sequence is a part of a folder (the folder
is split by the pauses between the shots), named by RawTherapee.

Only the Python standard library is used.
"""

import argparse
import asyncio
import csv
import json
import os
import signal
import socket
import subprocess
import sys
import tempfile
import time

PROTOCOL_VERSION = 1

# equal: factor applied to the blue/red equalizer of the white balance (1.0 for the camera)
# mode: "shift" (shift from the camera white balance) or "auto" (fixed white balance of the series,
# exposure model); auto: the model of the auto mode, serialized by TTSeriesWB (kept as it is)
DEFAULT_RULE = {"mired": 0.0, "green": 1.0, "equal": 1.0, "flash_only": True, "comment": "",
                "mode": "shift", "auto": ""}

# ranges of the TTSeriesWB tool: a value outside them is refused
LIMITS = {"mired": (-100.0, 100.0), "green": (0.5, 2.0), "equal": (0.5, 2.0)}


# variables: name made of letters, digits and . _ : - ; value: number, text or boolean
GLOBAL_KEY = "*"
SEQUENCE_SEPARATOR = "#"


def check_variable(name, value):
    if not name or any(not (c.isalnum() or c in "._:-") for c in name):
        raise ValueError("invalid variable name: %r" % name)
    if not isinstance(value, (bool, int, float, str)):
        raise ValueError("invalid value for %s: %r" % (name, value))


def parse_value(text):
    """value typed on the command line: boolean, integer, real number, or text"""
    low = text.lower()
    if low in ("true", "false"):
        return low == "true"
    for kind in (int, float):
        try:
            return kind(text)
        except ValueError:
            pass
    return text


def variable_key(where, sequence=None):
    """key of the variables: GLOBAL_KEY, a folder, or a folder and a sequence ("folder#sequence")"""
    if where in (GLOBAL_KEY, "global"):
        return GLOBAL_KEY
    key = normalize(where)
    if sequence:
        key += SEQUENCE_SEPARATOR + str(sequence)
    return key


def split_key(key):
    """(folder, sequence or None) of a key"""
    folder, sep, sequence = key.partition(SEQUENCE_SEPARATOR)
    return folder, (sequence if sep else None)


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
        self.observations = {}  # file -> last correction observed by TTSeriesWB (learn, save)
        self.variables = {}  # key (see variable_key) -> {name: value}
        self.file_data = {}  # file -> {namespace: {field: value}} (ex: exposure state of an image)
        self.load()

    def load(self):
        try:
            with open(self.filename, "r", encoding="utf-8") as f:
                data = json.load(f)
        except FileNotFoundError:
            return
        self.rules = data.get("rules", {})
        self.files = data.get("files", {})
        self.observations = data.get("observations", {})
        self.variables = data.get("variables", {})
        self.file_data = data.get("file_data", {})

    def save(self):
        """atomic write: temporary file in the same folder, then rename"""
        folder = os.path.dirname(self.filename)
        os.makedirs(folder, exist_ok=True)
        fd, tmp = tempfile.mkstemp(prefix=".state-", dir=folder)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                json.dump({"version": PROTOCOL_VERSION, "rules": self.rules, "files": self.files,
                           "observations": self.observations, "variables": self.variables,
                           "file_data": self.file_data},
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

    def set_rule(self, folder, mired, green=1.0, flash_only=True, comment="", equal=1.0, mode="shift", auto=""):
        check_limits(mired=mired, green=green, equal=equal)
        if mode not in ("shift", "auto"):
            raise ValueError("unknown mode: %r" % mode)
        folder = normalize(folder)
        self.rules[folder] = {"mired": float(mired), "green": float(green), "equal": float(equal),
                              "flash_only": bool(flash_only), "comment": str(comment),
                              "mode": mode, "auto": str(auto)}
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
        for table in (self.rules, self.files, self.file_data):
            for key in list(table):
                if is_below(key, old):
                    table[moved(key)] = table.pop(key)
                    count += 1
        for key in list(self.variables):
            folder, sequence = split_key(key)
            if key != GLOBAL_KEY and is_below(folder, old):
                self.variables[variable_key(moved(folder), sequence)] = self.variables.pop(key)
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

    def set_variable(self, key, name, value):
        check_variable(name, value)
        self.variables.setdefault(key, {})[name] = value
        self.save()

    def unset_variable(self, key, name):
        values = self.variables.get(key, {})
        if name not in values:
            raise KeyError("no variable %s for %s" % (name, key))
        del values[name]
        if not values:
            del self.variables[key]
        self.save()

    def resolve_variables(self, folder, sequence=None):
        """every variable applying to a folder (and a sequence of it), the nearest one wins:
        {name: {"value", "scope", "origin"}}; scope: sequence (the sequence, or the folder itself),
        ancestor (a parent folder) or global"""
        folder = normalize(folder)
        chain = []
        if sequence:
            chain.append((variable_key(folder, sequence), "sequence"))
        chain.append((folder, "sequence"))
        chain += [(p, "ancestor") for p in parents(folder) if p != folder]
        chain.append((GLOBAL_KEY, "global"))
        result = {}
        for key, scope in chain:
            for name, value in self.variables.get(key, {}).items():
                if name not in result:
                    result[name] = {"value": value, "scope": scope, "origin": key}
        return result

    def get_file_data(self, filename, namespace):
        return self.file_data.get(normalize(filename), {}).get(namespace, {})

    def set_file_data(self, filename, namespace, fields):
        """merges fields into the data of a file; a field set to None is removed"""
        filename = normalize(filename)
        data = self.file_data.setdefault(filename, {}).setdefault(namespace, {})
        for name, value in fields.items():
            if value is None:
                data.pop(name, None)
            else:
                data[name] = value
        if not data:
            del self.file_data[filename][namespace]
        if not self.file_data[filename]:
            del self.file_data[filename]
        self.save()
        return filename

    def files_below(self, folder, namespace):
        """{file: data} of the files in a folder or below it having data in the namespace"""
        folder = normalize(folder)
        return {f: d[namespace] for f, d in self.file_data.items()
                if is_below(f, folder) and namespace in d}

    def observe(self, observation):
        """keeps the last observation of a file; the server adds the time"""
        observation = dict(observation)
        filename = normalize(observation.pop("file"))
        observation["time"] = time.strftime("%Y-%m-%d %H:%M:%S")
        self.observations[filename] = observation
        self.save()
        return filename


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

    async def broadcast(self, path, event="rule_changed", **fields):
        message = (json.dumps(dict(fields, event=event, path=path)) + "\n").encode()
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
                                         request.get("equal", 1.0), request.get("mode", "shift"),
                                         request.get("auto", ""))
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
        if op == "observe":
            fields = {k: v for k, v in request.items() if k not in ("op", "id")}
            return {"file": self.state.observe(fields)}
        if op == "observations":
            return {"observations": self.state.observations}
        if op == "var_set":
            key = variable_key(request["path"], request.get("sequence"))
            self.state.set_variable(key, request["name"], request["value"])
            await self.broadcast(key, "variable_changed", name=request["name"])
            return {"path": key}
        if op == "var_unset":
            key = variable_key(request["path"], request.get("sequence"))
            self.state.unset_variable(key, request["name"])
            await self.broadcast(key, "variable_changed", name=request["name"])
            return {"path": key}
        if op == "var_get":
            # every variable applying to the folder, or only the one named
            variables = self.state.resolve_variables(request["path"], request.get("sequence"))
            if "name" in request:
                variables = {k: v for k, v in variables.items() if k == request["name"]}
            return {"variables": variables}
        if op == "var_list":
            return {"variables": self.state.variables}
        if op == "file_get":
            return {"data": self.state.get_file_data(request["file"], request["namespace"])}
        if op == "file_set":
            filename = self.state.set_file_data(request["file"], request["namespace"], request["fields"])
            return {"file": filename}
        if op == "file_list":
            return {"files": self.state.files_below(request["path"], request["namespace"])}
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
    if rule.get("mode", "shift") == "auto":
        text = "auto (%s)" % (rule.get("auto") or "no reference")
        if rule.get("comment"):
            text += "  # " + rule["comment"]
        return text
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
    p = sub.add_parser("observations", help="corrections observed by TTSeriesWB, as CSV on stdout")
    p.add_argument("--exif", nargs="?", const=DEFAULT_EXIF_TAGS, default=None, metavar="TAGS",
                   help="add tags read by exiftool in each file (comma separated, default: %s)" % DEFAULT_EXIF_TAGS)

    p = sub.add_parser("var-set", help="set a variable on a folder, a sequence or globally")
    p.add_argument("where", help='folder, or "global"')
    p.add_argument("name")
    p.add_argument("value", help="number, true/false, or text")
    p.add_argument("--sequence", help="sequence of the folder")
    p = sub.add_parser("var-unset", help="remove a variable (back to the inherited one)")
    p.add_argument("where", help='folder, or "global"')
    p.add_argument("name")
    p.add_argument("--sequence", help="sequence of the folder")
    sub.add_parser("vars", help="every variable set, where it is set")
    p = sub.add_parser("show", help="what applies to a folder: white balance and variables")
    p.add_argument("folder")
    p.add_argument("--sequence", help="sequence of the folder")

    p = sub.add_parser("exposure-apply", help="sets the exposure compensation of the pp3 files of the images "
                                              "following a reference (TTSeriesExposure), in a folder and below")
    p.add_argument("folder")
    p.add_argument("--dry-run", action="store_true", help="show the changes, change nothing")

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
    elif args.command == "observations":
        write_observations_csv(client.request("observations")["observations"], args.exif, sys.stdout)
    elif args.command in ("var-set", "var-unset"):
        params = {"path": variable_key(args.where), "name": args.name}
        if args.sequence:
            params["sequence"] = args.sequence
        if args.command == "var-set":
            params["value"] = parse_value(args.value)
            print("set: %s %s" % (client.request("var_set", **params)["path"], args.name))
        else:
            print("unset: %s %s" % (client.request("var_unset", **params)["path"], args.name))
    elif args.command == "vars":
        for key, values in sorted(client.request("var_list")["variables"].items()):
            print("global" if key == GLOBAL_KEY else key)
            for name, value in sorted(values.items()):
                print("  %-30s %s" % (name, format_value(value)))
    elif args.command == "exposure-apply":
        exposure_apply(client, normalize(args.folder), args.dry_run, sys.stdout)
    elif args.command == "show":
        folder = normalize(args.folder)
        print(folder + ("  (sequence %s)" % args.sequence if args.sequence else ""))
        rule = client.request("get", path=folder)
        print("  %-30s %s  <- %s" % ("white balance", format_rule(rule), rule["source"] or "default"))
        params = {"path": folder}
        if args.sequence:
            params["sequence"] = args.sequence
        variables = client.request("var_get", **params)["variables"]
        for name, info in sorted(variables.items()):
            origin = "global" if info["origin"] == GLOBAL_KEY else "%s (%s)" % (info["origin"], info["scope"])
            print("  %-30s %-16s <- %s" % (name, format_value(info["value"]), origin))
    return 0


# ---------------------------------------------------------------------------------------
# exposure of the closed images (TTSeriesExposure)
# ---------------------------------------------------------------------------------------

# limits of the exposure compensation of RawTherapee
EXPOSURE_MIN_COMP, EXPOSURE_MAX_COMP = -5.0, 12.0


def sequence_folder(filename):
    """the numbered folder of a photo: <folder>/pp and <folder>/pp/dpp belong to <folder>"""
    folder = os.path.dirname(filename)
    if os.path.basename(folder) == "dpp" and os.path.basename(os.path.dirname(folder)) == "pp":
        return os.path.dirname(os.path.dirname(folder))
    if os.path.basename(folder) == "pp":
        return os.path.dirname(folder)
    return folder


def parse_reference(value):
    """the exposure.reference variable: a JSON text {raw, comp, point?, target?}, None if invalid"""
    try:
        ref = json.loads(value) if isinstance(value, str) else None
    except ValueError:
        return None
    return ref if isinstance(ref, dict) and "comp" in ref else None


def reference_for(client, folder, follow):
    """reference followed by an image of a sequence folder: the nearest one (series, then parents),
    the one of the series only, or the nearest one above the series (parent)"""
    if follow == "series":
        values = client.request("var_list")["variables"].get(folder, {})
        return parse_reference(values.get("exposure.reference"))
    where = os.path.dirname(folder) if follow == "parent" else folder
    variables = client.request("var_get", path=where, name="exposure.reference")["variables"]
    info = variables.get("exposure.reference")
    return parse_reference(info["value"]) if info else None


def pp3_files(filename):
    """the pp3 files of an image: next to the raw file (rt_queue), or of its dng"""
    base = os.path.splitext(filename)[0]
    return [p for p in (filename + ".pp3", base + ".dng.pp3") if os.path.isfile(p)]


def read_compensation(pp3):
    section = None
    with open(pp3, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("["):
                section = line
            elif section == "[Exposure]" and line.startswith("Compensation="):
                try:
                    return float(line.split("=", 1)[1])
                except ValueError:
                    return None
    return None


def write_compensation(pp3, comp):
    """the Compensation line of the [Exposure] section, the rest of the file unchanged"""
    with open(pp3, "r", encoding="utf-8") as f:
        lines = f.readlines()
    section = None
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("["):
            section = stripped
        elif section == "[Exposure]" and stripped.startswith("Compensation="):
            lines[i] = "Compensation=%s\n" % repr(round(comp, 4))
            break
    else:
        raise ValueError("no Compensation in the [Exposure] section of " + pp3)
    fd, tmp = tempfile.mkstemp(prefix=".pp3-", dir=os.path.dirname(pp3))
    with os.fdopen(fd, "w", encoding="utf-8") as f:
        f.writelines(lines)
    os.replace(tmp, pp3)


def exposure_apply(client, folder, dry_run, out):
    """the images following a reference get reference + gap + offset of their sequence, written in
    their pp3; an image changed by hand since (pp3 different from the last applied value) becomes
    not applied, as in RawTherapee"""
    files = client.request("file_list", path=folder, namespace="exposure")["files"]
    counts = {"changed": 0, "unchanged": 0, "skipped": 0}
    for filename in sorted(files):
        data = files[filename]
        name = os.path.basename(filename)
        follow = data.get("follow", "nearest")
        if follow == "none":
            counts["skipped"] += 1
            continue
        if data.get("delta") is None:
            print("%s: not measured" % name, file=out)
            counts["skipped"] += 1
            continue
        seq = sequence_folder(filename)
        ref = reference_for(client, seq, follow)
        if ref is None:
            print("%s: no reference (%s)" % (name, follow), file=out)
            counts["skipped"] += 1
            continue
        offset = client.request("var_get", path=seq, name="exposure.offset")["variables"]
        offset = float(offset["exposure.offset"]["value"]) if offset else 0.0
        comp = max(EXPOSURE_MIN_COMP, min(EXPOSURE_MAX_COMP, float(ref["comp"]) + float(data["delta"]) + offset))
        pp3s = pp3_files(filename)
        if not pp3s:
            print("%s: no pp3 file" % name, file=out)
            counts["skipped"] += 1
            continue
        for pp3 in pp3s:
            current = read_compensation(pp3)
            if current is None:
                print("%s: no exposure compensation" % os.path.basename(pp3), file=out)
                continue
            last = data.get("comp")
            if last is not None and abs(current - float(last)) >= 0.005:
                print("%s: changed by hand (%+.2f, applied %+.2f): not applied anymore"
                      % (os.path.basename(pp3), current, float(last)), file=out)
                if not dry_run:
                    client.request("file_set", file=filename, namespace="exposure", fields={"follow": "none"})
                counts["skipped"] += 1
                break
            if abs(current - comp) < 0.005:
                counts["unchanged"] += 1
                continue
            print("%s: %+.2f -> %+.2f" % (os.path.basename(pp3), current, comp), file=out)
            if not dry_run:
                write_compensation(pp3, comp)
            counts["changed"] += 1
        else:
            if not dry_run:
                client.request("file_set", file=filename, namespace="exposure", fields={"comp": comp})
    print("%d changed, %d unchanged, %d skipped%s" % (counts["changed"], counts["unchanged"], counts["skipped"],
                                                     " (dry run: nothing written)" if dry_run else ""), file=out)


def format_value(value):
    if isinstance(value, bool):
        return "true" if value else "false"
    return str(value)


# ---------------------------------------------------------------------------------------
# observations
# ---------------------------------------------------------------------------------------

# tags looked at for the analysis: settings, flash, and the candidates found in the raw files
# (0x1300: GH5 scene measurement candidate; 0x8007: flash really used, S5 II in TTL)
DEFAULT_EXIF_TAGS = ("Model,ISO,ExposureTime,FNumber,LightValue,Flash,AFAssistLamp,"
                     "PanasonicRaw_CameraIFD_0x1300,Panasonic_0x8007,ColorTempKelvin")


def read_exif(files, tags):
    """tags read by exiftool in the existing files: {file: {tag: value}} (raw values)"""
    existing = [f for f in files if os.path.isfile(f)]
    if not existing:
        return {}
    command = ["exiftool", "-j", "-n", "-u", "-s"] + ["-" + t for t in tags] + existing
    try:
        output = subprocess.run(command, capture_output=True, text=True, check=False).stdout
    except FileNotFoundError:
        raise SystemExit("esbywb: exiftool not found (Debian: apt install libimage-exiftool-perl)")
    result = {}
    for entry in json.loads(output or "[]"):
        result[normalize(entry.get("SourceFile", ""))] = entry
    return result


def write_observations_csv(observations, exif_tags, out):
    fields = []
    for obs in observations.values():
        for key in obs:
            if key not in fields:
                fields.append(key)
    tags = [t for t in exif_tags.split(",") if t] if exif_tags else []
    exif = read_exif(list(observations), tags) if tags else {}

    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(["file"] + fields + ["exif:" + t for t in tags])
    for filename in sorted(observations):
        obs = observations[filename]
        values = exif.get(filename, {})
        writer.writerow([filename] + [obs.get(f, "") for f in fields] + [values.get(t, "") for t in tags])


if __name__ == "__main__":
    sys.exit(main())
