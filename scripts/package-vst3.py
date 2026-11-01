#!/usr/bin/env python3
"""Package an existing VST3 bundle without confusing Linux and Windows binaries."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import zipfile
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--bundle', required=True, type=Path)
parser.add_argument('--platform', required=True, choices=['linux-x64', 'windows-x64'])
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
bundle = args.bundle.resolve(strict=True)
if bundle.name != 'ATR-102.vst3' or not bundle.is_dir():
    parser.error('Expected the complete ATR-102.vst3 bundle directory')
platform_dir, extension = ('x86_64-linux', '.so') if args.platform == 'linux-x64' else ('x86_64-win', '.vst3')
binary = bundle / 'Contents' / platform_dir / ('ATR-102' + extension)
data = binary.read_bytes()
if args.platform == 'linux-x64':
    if data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', data, 18)[0] != 62:
        parser.error('Binary is not Linux x86-64 ELF')
else:
    if data[:2] != b'MZ':
        parser.error('Binary is not a Windows PE file')
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if data[offset:offset+4] != b'PE\0\0' or struct.unpack_from('<H', data, offset+4)[0] != 0x8664:
        parser.error('Binary is not Windows x86-64 PE')
module_info = (bundle / 'Contents/Resources/moduleinfo.json').read_text()
# JUCE's moduleinfo.json permits trailing commas; inspect its first name/version.
name = re.search(r'"Name"\s*:\s*"([^"]+)"', module_info)
version = re.search(r'"Version"\s*:\s*"([^"]+)"', module_info)
if not name or name[1] != 'ATR-102' or not version:
    parser.error('Unexpected product metadata')
commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
platform_name = 'Linux x64' if args.platform == 'linux-x64' else 'Windows x64'
install = '''ATR-102 {version} — VST3 {platform}

VERSAO DE DESENVOLVIMENTO / DEVELOPMENT BUILD

{instructions}

Preserve a pasta ATR-102.vst3 inteira, incluindo Contents e Resources.
Reabra sua DAW e execute uma nova busca de plugins VST3.

Esta versao tem a interface e o processamento inicial de fita. O modelo fisico
completo, os modos Eco/High independentes e a calibracao sonora final ainda
estao em desenvolvimento. Consulte DSP_NOTES.md e VALIDATION.md.

O ZIP Linux nao funciona no Windows ou macOS.
'''.format(version=version[1], platform=platform_name, instructions=(
    '1. Extraia o ZIP.\n2. Copie ATR-102.vst3 para ~/.vst3/\n   Exemplo: mkdir -p ~/.vst3 && cp -a ATR-102.vst3 ~/.vst3/'
    if args.platform == 'linux-x64' else
    '1. Extraia o ZIP.\n2. Copie ATR-102.vst3 para C:\\Program Files\\Common Files\\VST3\\\n   A copia pode solicitar permissao de administrador.'))
files = {}
for path in sorted(bundle.rglob('*')):
    if path.is_symlink():
        parser.error('Symlink in bundle; package only verified regular files')
    if path.is_file():
        files['ATR-102.vst3/' + path.relative_to(bundle).as_posix()] = path.read_bytes()
for name in ['DSP_NOTES.md', 'VALIDATION.md']:
    files[name] = (root / 'docs' / name).read_bytes()
files['INSTALL.txt'] = install.encode('utf-8')
metadata = {'product': 'ATR-102', 'version': version[1], 'platform': args.platform,
            'status': 'development', 'commit': commit, 'binary_sha256': hashlib.sha256(data).hexdigest()}
files['BUILDINFO.json'] = (json.dumps(metadata, indent=2) + '\n').encode()
for source, destination in [
    ('build/_deps/juce-src/LICENSE.md', 'licenses/JUCE-LICENSE.md'),
    ('build/_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt', 'licenses/VST3-SDK-LICENSE.txt')]:
    files[destination] = (root / source).read_bytes()
files['SHA256SUMS.txt'] = ''.join(hashlib.sha256(content).hexdigest() + '  ' + name + '\n'
                                 for name, content in sorted(files.items())).encode()
args.output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(args.output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for name, content in sorted(files.items()):
        info = zipfile.ZipInfo(name)
        info.create_system = 3
        info.external_attr = (0o100755 if name.endswith(('.so', '.vst3')) else 0o100644) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(info, content)
with zipfile.ZipFile(args.output) as archive:
    if archive.testzip() is not None:
        raise RuntimeError('ZIP CRC verification failed')
    for name, content in files.items():
        if archive.read(name) != content:
            raise RuntimeError('ZIP content mismatch: ' + name)
checksum = hashlib.sha256(args.output.read_bytes()).hexdigest()
args.output.with_suffix(args.output.suffix + '.sha256').write_text(checksum + '  ' + args.output.name + '\n')
print(json.dumps({'file': str(args.output), 'platform': args.platform, 'bytes': args.output.stat().st_size,
                  'sha256': checksum, 'files': len(files)}, indent=2))
