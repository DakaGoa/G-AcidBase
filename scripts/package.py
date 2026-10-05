from pathlib import Path
import hashlib
import json
import re
import shutil
import struct
import subprocess
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
artifacts = root / 'artifacts'
dist = root / 'dist'
build = root / 'build'
assets = root / 'assets'
logo_svg = assets / 'G-AcidBase-Logo.svg'
assert logo_svg.is_file(), 'Run python scripts/make_logo.py to create the logo assets'
for size in (64, 128, 256, 512, 1024):
    assert (assets / f'G-AcidBase-Logo-{size}.png').is_file(), f'Run python scripts/make_logo.py: missing {size} px logo'
header = (assets / 'G-AcidBase-Logo-512.png').read_bytes()
assert header[:8] == b'\x89PNG\r\n\x1a\n' and struct.unpack('>II', header[16:24]) == (512, 512), 'Logo PNG must be a valid 512x512 image'
# The update feed the plugin polls must name the version this package is: a
# mismatch means the shipped plugin would offer an update to itself, or miss one
# that exists. Checked here rather than in the plugin because the feed lives on
# the site, and packaging is where the two halves meet.
version = re.search(r'project\(GAcidBase VERSION ([0-9.]+)', (root / 'CMakeLists.txt').read_text()).group(1)
feed_url = 'https://y4m4.github.io/GoaSynth/gacidbase/version.json'
feed = json.loads(urllib.request.urlopen(feed_url, timeout=20).read())
assert feed.get('latest') == version, f'{feed_url} says latest={feed.get("latest")}, this build is {version}'
assert feed.get('url') == 'https://y4m4.github.io/GoaSynth/gacidbase/', 'the feed points somewhere other than the product page'
plugin = build / 'GAcidBase_artefacts/Release/VST3/G-AcidBase.vst3'
standalone = build / 'GAcidBase_artefacts/Release/Standalone/G-AcidBase.exe'
assert plugin.is_dir(), 'Build the VST3 first'
assert standalone.is_file(), 'Build the standalone first'
# Never package merely because a stale report says a previous build passed.
subprocess.run(['ctest', '--test-dir', str(build), '-C', 'Release', '--output-on-failure'], cwd=root, check=True)
for report in ('verification.txt', 'feature-verification.txt'):
    assert 'Failures: 0' in (artifacts / report).read_text(), f'{report}: tests must pass before packaging'
presets = sorted((artifacts / 'Presets').glob('*.gacid'))
assert len(presets) == 50, f'Expected 50 preset files, found {len(presets)}'
dist.mkdir(exist_ok=True)
shutil.copytree(plugin, dist / plugin.name, dirs_exist_ok=True)
shutil.copy2(standalone, dist / standalone.name)
shutil.copytree(artifacts / 'Presets', dist / 'Presets', dirs_exist_ok=True)
shutil.copytree(assets, dist / 'Logo', dirs_exist_ok=True)
shutil.copytree(assets, artifacts / 'Logo', dirs_exist_ok=True)
# One beat of the animated header mark, so the motion can be reviewed without running the plugin.
animation = artifacts / 'G-AcidBase-Logo-Animation.png'
assert animation.exists(), 'missing artifacts/G-AcidBase-Logo-Animation.png'
shutil.copy2(animation, dist / animation.name)
shutil.copy2(root / 'README.md', dist / 'README.md')
shutil.copy2(root / 'FL-STUDIO-FIX.md', dist / 'FL-STUDIO-FIX.md')
shutil.copy2(artifacts / 'verification.txt', dist / 'verification.txt')
shutil.copy2(artifacts / 'feature-verification.txt', dist / 'feature-verification.txt')
shutil.copy2(build / '_deps/juce-src/LICENSE.md', dist / 'JUCE-LICENSE.md')
sdk = build / '_deps/juce-src/modules/juce_audio_processors/format_types/VST3_SDK'
notices = dist / 'ThirdPartyNotices'
notices.mkdir(exist_ok=True)
for subdir in ('', 'base', 'pluginterfaces', 'public.sdk'):
    source = sdk / subdir / 'LICENSE.txt'
    shutil.copy2(source, notices / ((subdir or 'VST3-SDK') + '-LICENSE.txt'))
shutil.copy2(sdk / 'VST3_License_Agreement.pdf', notices / 'VST3_License_Agreement.pdf')
preview = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>G-AcidBase — Native build preview</title><style>*{box-sizing:border-box}body{margin:0;padding:26px;background:#0b1018;color:#e8eef7;font:15px system-ui}h1{color:#b8ff38;font-size:30px;margin:0 0 8px}p{color:#8798ac;max-width:1100px;line-height:1.5}img{display:block;width:100%;max-width:1280px;border:1px solid #283749;border-radius:12px;margin:24px 0}audio{width:100%;max-width:650px}strong{color:#42e8da}.notice{color:#b290ff}a{color:#b8ff38}section{padding:18px;border:1px solid #283749;border-radius:12px;max-width:1280px}.brand{display:flex;gap:18px;align-items:center;margin-bottom:6px}.brand img{width:76px;height:76px;margin:0;border-radius:18px;max-width:none}</style><div class="brand"><img src="Logo/G-AcidBase-Logo-256.png" alt="G-AcidBase logo"><h1>G-AcidBase</h1></div><p>Windows x64 VST3 + standalone / 50 factory presets / 4× oversampled acid bass for Goa trance.</p><p class="notice">This is a screenshot of the actual native plugin, not a web simulation. Open the standalone executable or load the VST3 in your DAW for interactive controls.</p><img src="G-AcidBase-Interface.png" alt="G-AcidBase native interface"><section><strong>50-preset audio audition</strong><p>Turn down your volume first. Each preset plays for 1.5 seconds, in preset order 01–50. Original 303-inspired synthesis, not circuit-identical emulation.</p><audio controls preload="metadata" src="G-AcidBase-50-Preset-Demo.wav"></audio><p><a href="verification.txt">Verification report</a> · <a href="feature-verification.txt">Feature verification</a> · <a href="Logo/G-AcidBase-Logo.svg">Logo (SVG)</a> · <a href="G-AcidBase-Logo-Animation.png">Logo animation (one beat)</a> · <a href="preset-audio-measurements.csv">Measured preset audio levels</a></p></section></html>'''
(artifacts / 'index.html').write_text(preview, encoding='utf-8')
shutil.copy2(artifacts / 'index.html', dist / 'index.html')
manifest = {}
for path in sorted(dist.rglob('*')):
    if path.is_file() and path.suffix != '.zip' and path.name != 'manifest.json':
        manifest[path.relative_to(dist).as_posix()] = {'bytes': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
(dist / 'manifest.json').write_text(json.dumps(manifest, indent=2))
archive = dist / 'G-AcidBase-Windows-x64.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for path in sorted(dist.rglob('*')):
        if path.is_file() and path.suffix != '.zip':
            z.write(path, 'G-AcidBase/' + path.relative_to(dist).as_posix())
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert sum(name.endswith('.gacid') for name in z.namelist()) == 50
    assert any(name.endswith('Contents/x86_64-win/G-AcidBase.vst3') for name in z.namelist())
    assert 'G-AcidBase/Logo/G-AcidBase-Logo.svg' in z.namelist() and 'G-AcidBase/Logo/G-AcidBase-Logo-256.png' in z.namelist()
    assert 'G-AcidBase/G-AcidBase-Logo-Animation.png' in z.namelist()
    assert 'G-AcidBase/index.html' in z.namelist() and 'G-AcidBase/manifest.json' in z.namelist()
preview = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>G-AcidBase — Native build preview</title><style>*{box-sizing:border-box}body{margin:0;padding:26px;background:#0b1018;color:#e8eef7;font:15px system-ui}h1{color:#b8ff38;font-size:30px;margin:0 0 8px}p{color:#8798ac;max-width:1100px;line-height:1.5}img{display:block;width:100%;max-width:1280px;border:1px solid #283749;border-radius:12px;margin:24px 0}audio{width:100%;max-width:650px}strong{color:#42e8da}.notice{color:#b290ff}a{color:#b8ff38}section{padding:18px;border:1px solid #283749;border-radius:12px;max-width:1280px}</style><img src="Logo/G-AcidBase-Logo-256.png" alt="G-AcidBase logo" style="width:72px;height:72px;float:left;margin:0 16px 8px 0;border-radius:16px"><h1>G-AcidBase</h1><div style="clear:both"></div><p>Windows x64 VST3 + standalone / 50 factory presets / 4× oversampled acid bass for Goa trance.</p><p class="notice">This is a screenshot of the actual native plugin, not a web simulation. Open the standalone executable or load the VST3 in your DAW for interactive controls.</p><img src="G-AcidBase-Interface.png" alt="G-AcidBase native interface"><section><strong>50-preset audio audition</strong><p>Turn down your volume first. Each preset plays for 1.5 seconds, in preset order 01–50. Original 303-inspired synthesis, not circuit-identical emulation.</p><audio controls preload="metadata" src="G-AcidBase-50-Preset-Demo.wav"></audio><p><a href="verification.txt">Verification report</a> · <a href="preset-audio-measurements.csv">Measured preset audio levels</a></p></section></html>'''
print(f'Packaged: {archive}\nSize: {archive.stat().st_size:,} bytes\n50 factory files verified; ZIP integrity passed.')
