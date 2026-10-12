import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('wbmc_setup', Path(__file__).resolve().parents[1] / 'tools/setup.py')
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)


class SetupPreservation(unittest.TestCase):
    def test_reinstall_keeps_world_and_options(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); api = root / 'api.jar'; api.write_bytes(b'test API')
            instance = setup.prepare_instance(root / 'Prism', root / 'Java21', api)
            game = instance / 'minecraft'; save = game / 'saves/example/level.dat'
            save.parent.mkdir(parents=True); save.write_bytes(b'world marker')
            (game / 'options.txt').write_text('user preference marker')
            setup.prepare_instance(root / 'Prism', root / 'Java21', api)
            self.assertEqual(save.read_bytes(), b'world marker')
            self.assertEqual((game / 'options.txt').read_text(), 'user preference marker')

    def test_incompatible_instance_is_not_modified(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); instance = root / 'instances/WarbandCraft'; instance.mkdir(parents=True)
            pack = instance / 'mmc-pack.json'
            pack.write_text(json.dumps({'components': [{'uid': 'net.minecraft', 'version': '26.3'}]}))
            before = pack.read_bytes()
            with self.assertRaises(RuntimeError):
                setup.prepare_instance(root, root / 'Java21', root / 'absent.jar')
            self.assertEqual(pack.read_bytes(), before)
            self.assertFalse((instance / 'minecraft').exists())

    def test_dot_minecraft_directory_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); api = root / 'api.jar'; api.write_bytes(b'test API')
            instance = setup.prepare_instance(root / 'Prism', root / 'Java21', api)
            (instance / 'minecraft').rename(instance / '.minecraft')
            setup.prepare_instance(root / 'Prism', root / 'Java21', api)
            self.assertTrue((instance / '.minecraft/mods/warbandcraft-0.1.0.jar').exists())
            self.assertFalse((instance / 'minecraft').exists())

    def test_window_config_backup_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as temp:
            config = Path(temp) / 'rgl_config.txt'
            original = 'start_windowed = 0\ndisplay_width = 1920\ndisplay_height = 1080\nmax_framerate = 120\nmusic_volume = 0.5\n'
            config.write_text(original)
            setup.configure_window(config); setup.configure_window(config)
            self.assertEqual(config.with_name('rgl_config.txt.warbandcraft-backup').read_text(), original)
            self.assertIn('display_width = 1280', config.read_text())
            self.assertIn('music_volume = 0.5', config.read_text())


if __name__ == '__main__':
    unittest.main()
