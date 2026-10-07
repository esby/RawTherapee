#!/usr/bin/env python3
"""Tests of esbywb (run from rtgui/esby/server: python3 -m unittest discover tests)."""

import asyncio
import json
import os
import socket
import sys
import tempfile
import threading
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import esbywb  # noqa: E402


class StateTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = esbywb.normalize(self.tmp.name)
        self.state = esbywb.State(os.path.join(self.root, "state", "state.json"))
        self.lucca = os.path.join(self.root, "photos", "Lucca 2026")
        self.hall = os.path.join(self.lucca, "Samedi - Hall 3")
        self.group = os.path.join(self.hall, "Groupe Genshin")
        os.makedirs(self.group)

    def tearDown(self):
        self.tmp.cleanup()

    def test_default_value(self):
        rule = self.state.resolve(self.group)
        self.assertEqual(rule["mired"], 0.0)
        self.assertEqual(rule["green"], 1.0)
        self.assertEqual(rule["equal"], 1.0)
        self.assertIsNone(rule["source"])

    def test_equalizer(self):
        self.state.set_rule(self.lucca, 15, green=0.95, equal=1.08)
        self.assertEqual(self.state.resolve(self.group)["equal"], 1.08)
        self.state.record_applied(os.path.join(self.hall, "P1.RW2"), 4679, 0.856, self.lucca, equal=1.08)
        self.assertEqual(self.state.file_state(os.path.join(self.hall, "P1.RW2"))["E"], 1.08)

    def test_limits(self):
        for kwargs in ({"mired": 250}, {"mired": -101}, {"mired": 0, "green": 3}, {"mired": 0, "equal": 0.2}):
            with self.assertRaises(ValueError):
                self.state.set_rule(self.lucca, **kwargs)
        self.assertNotIn(self.lucca, self.state.rules)

    def test_no_negative_zero(self):
        self.assertTrue(esbywb.format_rule({"mired": -0.00001, "green": 1.0}).startswith("+0.0 mireds"))

    def test_rule_without_equal(self):
        # rules written before the equalizer was handled: the default factor applies
        self.state.rules[self.lucca] = {"mired": 15.0, "green": 1.0, "flash_only": True, "comment": ""}
        self.assertEqual(self.state.resolve(self.group)["equal"], 1.0)

    def test_inheritance(self):
        self.state.set_rule(self.lucca, 15)
        self.state.set_rule(self.hall, 22, green=0.95)
        self.assertEqual(self.state.resolve(os.path.join(self.lucca, "Vendredi"))["mired"], 15)
        self.assertEqual(self.state.resolve(os.path.join(self.lucca, "Vendredi"))["source"], self.lucca)
        rule = self.state.resolve(self.group)
        self.assertEqual((rule["mired"], rule["green"], rule["source"]), (22, 0.95, self.hall))

    def test_unset_back_to_inheritance(self):
        self.state.set_rule(self.lucca, 15)
        self.state.set_rule(self.hall, 22)
        self.state.unset_rule(self.hall)
        self.assertEqual(self.state.resolve(self.group)["source"], self.lucca)
        with self.assertRaises(KeyError):
            self.state.unset_rule(self.hall)

    def test_trailing_slash_and_symlink(self):
        self.state.set_rule(self.lucca + "/", 15)
        link = os.path.join(self.root, "lien")
        os.symlink(self.lucca, link)
        self.assertEqual(self.state.resolve(link)["source"], self.lucca)

    def test_persistence(self):
        self.state.set_rule(self.lucca, 15, comment="hall chaud")
        self.state.record_applied(os.path.join(self.hall, "P1.RW2"), 4679, 1.058, self.lucca)
        again = esbywb.State(self.state.filename)
        self.assertEqual(again.resolve(self.hall)["comment"], "hall chaud")
        self.assertEqual(again.file_state(os.path.join(self.hall, "P1.RW2"))["T"], 4679)

    def test_move(self):
        self.state.set_rule(self.lucca, 15)
        self.state.set_rule(self.hall, 22)
        self.state.record_applied(os.path.join(self.hall, "P1.RW2"), 4600, 1.0, self.hall)
        renamed = os.path.join(self.root, "photos", "Lucca 2026 (tri)")
        old, new, count = self.state.move(self.lucca, renamed)
        self.assertEqual(count, 3)
        new_hall = os.path.join(renamed, "Samedi - Hall 3")
        self.assertEqual(self.state.resolve(new_hall)["mired"], 22)
        info = self.state.file_state(os.path.join(new_hall, "P1.RW2"))
        self.assertEqual(info["source"], new_hall)
        self.assertNotIn(self.lucca, self.state.rules)

    def test_move_does_not_touch_similar_names(self):
        self.state.set_rule(self.lucca, 15)
        other = self.lucca + " bis"
        self.state.set_rule(other, 5)
        self.state.move(self.lucca, os.path.join(self.root, "x"))
        self.assertIn(other, self.state.rules)


class ServerTest(unittest.TestCase):
    """a real server on a temporary socket, in a thread"""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = esbywb.normalize(self.tmp.name)
        self.socket_path = os.path.join(self.root, "esby-wb.sock")
        self.state = esbywb.State(os.path.join(self.root, "state.json"))
        self.server = esbywb.Server(self.state, self.socket_path)
        self.loop = asyncio.new_event_loop()
        ready = threading.Event()

        async def start():
            ev = asyncio.Event()
            task = asyncio.ensure_future(self.server.run(ev))
            await ev.wait()
            ready.set()
            await task

        self.thread = threading.Thread(target=lambda: self.loop.run_until_complete(start()), daemon=True)
        self.thread.start()
        self.assertTrue(ready.wait(5))
        self.folder = os.path.join(self.root, "photos", "Lucca 2026")
        os.makedirs(self.folder)
        self.clients = []

    def tearDown(self):
        for client in self.clients:
            client.close()
        # clean shutdown: listening socket and open connections closed before the loop stops
        asyncio.run_coroutine_threadsafe(self.server.stop(), self.loop).result(5)
        self.thread.join(5)
        self.loop.close()
        self.tmp.cleanup()

    def client(self):
        c = esbywb.Client(self.socket_path)
        self.clients.append(c)
        return c

    def test_protocol(self):
        client = self.client()
        self.assertEqual(client.request("hello")["version"], esbywb.PROTOCOL_VERSION)
        client.request("set", path=self.folder, mired=15, equal=1.05, comment="hall")
        rule = client.request("get", path=os.path.join(self.folder, "Samedi"))
        self.assertEqual((rule["mired"], rule["equal"], rule["source"]), (15, 1.05, self.folder))
        self.assertEqual(len(client.request("list")["rules"]), 1)
        client.request("applied", file=os.path.join(self.folder, "P1.RW2"), T=4679, G=1.058, source=self.folder)
        self.assertEqual(client.request("file_state", file=os.path.join(self.folder, "P1.RW2"))["state"]["T"], 4679)
        self.assertIsNone(client.request("file_state", file=os.path.join(self.folder, "P2.RW2"))["state"])

    def test_observations(self):
        import io
        client = self.client()
        f1 = os.path.join(self.folder, "P1.RW2")
        client.request("observe", file=f1, kind="learn", mired=15.4, iso=200, shutter=0.0125, fnumber=2.8)
        client.request("observe", file=f1, kind="saved-manual", mired=16.0, iso=200, shutter=0.0125, fnumber=2.8)
        client.request("observe", file=os.path.join(self.folder, "P2.RW2"), kind="saved-series", mired=15.4)
        observations = client.request("observations")["observations"]
        self.assertEqual(len(observations), 2)              # one per file, the last one
        self.assertEqual(observations[f1]["kind"], "saved-manual")
        self.assertIn("time", observations[f1])
        out = io.StringIO()
        esbywb.write_observations_csv(observations, None, out)
        lines = out.getvalue().strip().splitlines()
        self.assertEqual(len(lines), 3)
        self.assertTrue(lines[0].startswith("file,kind,mired"))

    def test_errors_do_not_stop_the_server(self):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(self.socket_path)
        f = s.makefile("rwb")
        for line in (b"not json\n", b'{"id": 7, "op": "nothing"}\n', b'{"id": 8, "op": "unset", "path": "/nowhere"}\n'):
            f.write(line)
            f.flush()
            answer = json.loads(f.readline())
            self.assertFalse(answer["ok"])
        self.assertEqual(answer["id"], 8)
        client = self.client()
        self.assertTrue(client.request("hello")["ok"])
        f.close()
        s.close()

    def test_subscribe_receives_changes(self):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(self.socket_path)
        f = s.makefile("rwb")
        f.write(b'{"id": 1, "op": "subscribe"}\n')
        f.flush()
        self.assertTrue(json.loads(f.readline())["ok"])
        client = self.client()
        client.request("set", path=self.folder, mired=12)
        event = json.loads(f.readline())
        self.assertEqual(event, {"event": "rule_changed", "path": self.folder})
        client.request("unset", path=self.folder)
        self.assertEqual(json.loads(f.readline())["path"], self.folder)
        f.close()
        s.close()

    def test_second_server_refused(self):
        with self.assertRaises(SystemExit):
            esbywb.prepare_socket(self.socket_path)

    def test_command_line(self):
        out = []
        real_print = print

        def capture(*args, **kwargs):
            out.append(" ".join(str(a) for a in args))

        import builtins
        builtins.print = capture
        try:
            esbywb.main(["--socket", self.socket_path, "set", self.folder, "15", "--comment", "hall"])
            esbywb.main(["--socket", self.socket_path, "get", os.path.join(self.folder, "x")])
            esbywb.main(["--socket", self.socket_path, "list"])
            os.rmdir(self.folder)
            esbywb.main(["--socket", self.socket_path, "orphans"])
        finally:
            builtins.print = real_print
        text = "\n".join(out)
        self.assertIn("+15.0 mireds", text)
        self.assertIn("source: " + self.folder, text)
        self.assertEqual(out[-1], self.folder)


class ExifTest(unittest.TestCase):
    """--exif reads the tags with exiftool (skipped without exiftool or sample file)"""
    SAMPLE = "/mnt/user-data/uploads/P2655548.RW2"

    @unittest.skipUnless(os.path.isfile(SAMPLE) and __import__("shutil").which("exiftool"), "no exiftool or sample")
    def test_exif_columns(self):
        import io
        sample = esbywb.normalize(self.SAMPLE)
        out = io.StringIO()
        esbywb.write_observations_csv({sample: {"kind": "learn", "mired": 15.4}}, "Model,ISO,Flash,PanasonicRaw_CameraIFD_0x1300", out)
        header, row = out.getvalue().strip().splitlines()
        self.assertIn("exif:PanasonicRaw_CameraIFD_0x1300", header)
        self.assertIn("DC-GH5", row)


class StaleSocketTest(unittest.TestCase):
    def test_stale_socket_is_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "s.sock")
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.bind(path)
            s.close()  # the file stays, nobody listens
            esbywb.prepare_socket(path)
            self.assertFalse(os.path.exists(path))


if __name__ == "__main__":
    unittest.main()
