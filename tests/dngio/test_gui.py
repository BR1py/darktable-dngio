"""Opt-in GUI regression checks; select a Lua-enabled build with DARKTABLE_GUI."""

import json
import os
from pathlib import Path
import re
import shutil
import sqlite3
import subprocess
import tempfile
import time
import unittest


GUI = os.environ.get("DARKTABLE_GUI")


@unittest.skipUnless(GUI, "set DARKTABLE_GUI and provide a desktop display")
class GuiTests(unittest.TestCase):
    def test_catalog_import_keeps_scroll_position(self):
        self._check_catalog_position("lighttable")

    def test_catalog_import_keeps_filmstrip_position(self):
        self._check_catalog_position("darkroom")

    def _check_catalog_position(self, view):
        from PIL import Image

        with tempfile.TemporaryDirectory(prefix="dt-dngio-gui-") as directory:
            work = Path(directory)
            inputs, outputs = work / "input", work / "output"
            inputs.mkdir()
            outputs.mkdir()
            for index in range(50):
                Image.new("RGB", (64, 64), (index * 5, 80, 120)).save(inputs / f"{index:02}.jpg")
            source = inputs / "zz-source.dng"
            shutil.copyfile(Path(__file__).parent / "fixtures/TEST6.dng", source)
            result = work / "result.json"
            error = work / "error.txt"
            script = work / "test.lua"
            script.write_text('''local dt = require "darktable"
local root = ROOT
local ok, message = pcall(function()
dt.control.sleep(1000)
local first
for index = 0, 49 do
  local image = dt.database.import(string.format(root .. "/input/%02d.jpg", index))
  if index == 0 then first = image end
end
local source = dt.database.import(root .. "/input/zz-source.dng")
dt.control.sleep(1500)
dt.gui.selection({source})
if VIEW == "darkroom" then
  dt.gui.action("global/switch views/darkroom", "", "", 1)
end
dt.control.sleep(500)
dt.gui.views.lighttable.set_image_visible(source)
dt.control.sleep(500)
if VIEW == "lighttable" then
  assert(dt.gui.views.lighttable.is_image_visible(source), "source must initially be visible")
  assert(not dt.gui.views.lighttable.is_image_visible(first), "test must initially be scrolled down")
else
  assert(dt.gui.current_view().id == "darkroom", "test must enter darkroom")
end
io.stdout:write("DNGIO_GUI_BEFORE\\n"); io.stdout:flush()
local collection_size = #dt.collection
dt.gui.action("lib/export/start export", "", "", 1)
local exported
for attempt = 1, 600 do
  exported = io.open(root .. "/output/zz-source.dng.xmp", "r")
  if exported then exported:close(); break end
  dt.control.sleep(100)
end
assert(exported, "DNG export did not complete")
dt.control.sleep(1500)
local selection = dt.gui.selection()
io.stdout:write("DNGIO_GUI_AFTER\\n"); io.stdout:flush()
local file = assert(io.open(root .. "/result.json.tmp", "w"))
file:write(string.format('{"visible":%s,"selection_kept":%s,"collection_grew":%s}',
  tostring(VIEW == "darkroom" or dt.gui.views.lighttable.is_image_visible(source)),
  tostring(#selection == 1 and selection[1] == source),
  tostring(#dt.collection == collection_size + 1)))
file:close()
assert(os.rename(root .. "/result.json.tmp", root .. "/result.json"))
end)
if not ok then
  local file = assert(io.open(root .. "/error.txt", "w"))
  file:write(tostring(message))
  file:close()
end
'''.replace("ROOT", json.dumps(str(work))).replace("VIEW", json.dumps(view)))
            command = [GUI, "--configdir", str(work / "config"), "--cachedir", str(work / "cache"),
                       "--library", str(work / "library.db"), "--disable-opencl", "-t", "2", "-d", "lighttable",
                       "--luacmd", f"dofile({json.dumps(str(script))})"]
            options = {"write_sidecar_files": "never", "resourcelevel": "reference",
                       "backthumbs_mipsize": "never",
                       "plugins/lighttable/export/format_name": "dng",
                       "plugins/lighttable/export/storage_name": "disk",
                       "plugins/imageio/storage/disk/file_directory": str(outputs / "$(FILE_NAME)")}
            for key, value in dict(target_version=2, compression=2, effort=1, preview_size=0,
                                   consider_crop="false", clone_xmp="true", import_to_catalog="true").items():
                options[f"plugins/imageio/format/dng_jxl/{key}"] = value
            for key, value in options.items():
                command += ["--conf", f"{key}={value}"]
            with (work / "gui.log").open("w+") as log:
                process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
                try:
                    deadline = time.monotonic() + 120
                    while not result.exists() and not error.exists() and process.poll() is None and time.monotonic() < deadline:
                        time.sleep(0.1)
                finally:
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=15)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait()
                log.seek(0)
                output_log = log.read()
                self.assertFalse(error.exists(), error.read_text() if error.exists() else "")
                self.assertTrue(result.exists(), output_log[-4000:])
            state = json.loads(result.read_text())
            self.assertTrue(state["selection_kept"], "export changed the selected image")
            self.assertTrue(state["collection_grew"], "GUI collection did not include the exported image")
            with sqlite3.connect(work / "library.db") as database:
                count = database.execute("SELECT count(*) FROM images JOIN film_rolls "
                                         "ON images.film_id = film_rolls.id WHERE folder = ?",
                                         (str(outputs),)).fetchone()[0]
            self.assertEqual(count, 1, "completed export was not imported")
            self.assertTrue(state["visible"], "catalog import scrolled away from the current image")
            if view == "darkroom":
                # Lua's visibility API excludes filmstrips; use DT's existing redraw diagnostics.
                before, after = output_log.split("DNGIO_GUI_BEFORE\n", 1)
                after = after.split("DNGIO_GUI_AFTER\n", 1)[0]
                offsets_before = re.findall(r"offset=(\d+)", before)
                offsets_after = re.findall(r"offset=(\d+)", after)
                self.assertTrue(offsets_before and offsets_after, output_log[-4000:])
                self.assertGreater(int(offsets_before[-1]), 1, "filmstrip must initially be scrolled down")
                self.assertEqual(offsets_after[-1], offsets_before[-1], "catalog import moved the filmstrip")


if __name__ == "__main__":
    unittest.main()
