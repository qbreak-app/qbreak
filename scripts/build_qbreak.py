#!/usr/bin/python3

import typing
import platform
import os
import shutil
import glob
import sys
from pathlib import Path
import multiprocessing
import build_utils

EXIT_OK = 0
EXIT_ERROR = 1

# Check if Qt is specified
if not 'QT_HOME' in os.environ:
    print('Qt location must be set in QT_HOME environment variable.')
    exit(1)

# Prepare build directory
build_dir = Path('build')
if build_dir.exists():
    shutil.rmtree(build_dir)
os.mkdir(build_dir)

app_source = Path('../app').resolve()
version_suffix = build_utils.get_version(app_source / 'config.h', 'QBREAK_VERSION_SUFFIX') 
version_minor = build_utils.get_version(app_source / 'config.h', 'QBREAK_VERSION_MINOR')
version_major = build_utils.get_version(app_source / 'config.h', 'QBREAK_VERSION_MAJOR')

if version_major is None or version_minor is None or version_suffix is None:
    print('App version is not found, exiting.')
    exit(EXIT_OK)
    
app_version = f'{version_major}.{version_minor}.{version_suffix}'
print (f'Found QBreak version: {app_version}')

# Go to build directory
os.chdir(build_dir)

if platform.system() == 'Linux':
    print('Linux detected')
    print('Configure...')

    qmake_path = Path(os.environ['QT_HOME']) / 'bin' / 'qmake'
    retcode = os.system(f'{qmake_path} ../../app')
    if retcode != 0:
        print(f'qmake call failed with code {retcode}')
        exit(retcode)

    # Check the requested type of build - debug or release
    build_type = 'release'
    if len(sys.argv) == 2:
        build_type = sys.argv[1]

    print('Build...')
    retcode = os.system(f'make {build_type} -j4')
    if retcode != 0:
        print(f'make call failed with code {retcode}')
        exit(retcode)

    # Build appimage
    print('Assembling app...')
    os.chdir('..')

    # Remove possible old image
    if os.path.exists('appimage_dir'):
        shutil.rmtree('appimage_dir')
    
    # Expand image template
    retcode = os.system('tar -xvzf appimage_dir.tar.gz')
    if retcode != 0:
        print(f'Failed to expand template directory, code {retcode}')
        exit(retcode)

    # Copy binary file
    shutil.copy('build/qbreak', 'appimage_dir/usr/bin')

    deploy_options = [
        '-always-overwrite', 
        '-verbose=2', 
        '-appimage', 
        '-qmake=' + os.environ['QT_HOME'] + '/bin/qmake', 
        '-unsupported-allow-new-glibc', 
        #'-no-translations', 
        '-extra-plugins=iconengines,platformthemes/libqgtk3.so,platforms/libqxcb.so'
    ]

    desktop_path = 'appimage_dir/usr/share/applications/qbreak.desktop'
    cmd_deploy = f'./linuxdeployqt {desktop_path} {" ".join(deploy_options)}'
    retcode = os.system(cmd_deploy)
    if retcode != 0:
        print(f'linuxdeployqt failed with code {retcode}')
        print(cmd_deploy)
        exit(retcode)

    releases_dir = Path('releases')
    if not releases_dir.exists():
        os.mkdir(releases_dir)
    for f in os.listdir():
        if f.endswith('x86_64.AppImage') and f.startswith('QBreak'):
            shutil.move(f, releases_dir / f'qbreak-{app_version}-x86_64.AppImage')
            
    exit(0)