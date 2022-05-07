#!/usr/bin/python3

import os
import sys
import multiprocessing
import shutil
import platform
import re
import glob
import typing
import codecs

# from PyInquirer import prompt, print_json


AR_TEMP_DIR = "ar_temp_dir"

def show_menu(message, lst):
    print()
    # Print selection menu
    for l in range(len(lst)):
        print(f'{l}) {lst[l]}')

    return lst[int(input(message))]


def ask_password(message):
    return input(message)


# Replace substring in a file
def inplace_change(filename, old_string, new_string):
    # Safely read the input filename using 'with'
    with open(filename) as f:
        s = f.read()
        if old_string not in s:
            print('"{old_string}" not found in {filename}.'.format(**locals()))
            return

    # Safely write the changed content, if found in the file
    with open(filename, 'w') as f:
        print('Changing "{old_string}" to "{new_string}" in {filename}'.format(**locals()))
        s = s.replace(old_string, new_string)
        f.write(s)


# Architecture used for VS2019 architecture parameters
def make_build(path: str, profile: str, params: dict, merge_libs :bool, generator: str=None, architecture: str = None):
    cmdline = "cmake "
    if generator is not None:
        cmdline += f'-G "{generator}"'
    if architecture is not None:
        cmdline += f' -A {architecture}'

    for key in params:
        cmdline += f" -D {key}={params[key]}"
    
    cmdline += f" -D CMAKE_BUILD_TYPE={profile}"
    cmdline += f" {path}"
    print(f"Cmake configure call: {cmdline}")

    retcode = os.system(cmdline)

    if retcode != 0:
        return retcode
    
    if platform.system() == "Windows":
        cmdline = "cmake --build . --config %s -j%d" % (profile, multiprocessing.cpu_count()) 
    else:
        cmdline = f"cmake --build . -j{multiprocessing.cpu_count()}"

    retcode = os.system(cmdline)
    if retcode != 0:
        return retcode

    return 0


def get_version(path, name):
    with codecs.open(path, mode='r', encoding='utf-8') as f:
        t = f.read()

    pattern = r"#define " + name + r"[\s]+(?P<number>[\d]+)"
    m = re.search(pattern=pattern, string=t, flags=re.MULTILINE)
    if m is not None:
        return m.group("number")
    else:
        return None


def make_chdir(path):
    # Prepare build directory
    if os.path.exists(path):
        shutil.rmtree(path)
    os.mkdir(path)
    os.chdir(path)


def upload_to_builds(path):
    # Build command line
    if platform.system().lower() == "windows":
        cmdline = f"pscp.exe {path} uploader@f.sevana.biz:/var/www/f.sevana.biz/public_html/"
    else:
        cmdline = f"scp -o \"StrictHostKeyChecking no\" {path} uploader@f.sevana.biz:/var/www/f.sevana.biz/public_html/"

    # Upload    
    retcode = os.system(cmdline)
    
    # Print results
    if retcode == 0:
        print (f"https://f.sevana.biz/{os.path.basename(path)}")
    else:
        print ("Failed to upload. Return code: %d" % retcode)


def merge_static_libs(main_lib, build_dir, use_android_ndk: bool):
    # Find all static libraries
    print ("Looking for additional static libraries to merge...")
    files = glob.glob("**/*.a", recursive=True)
    print (files)

    if platform.system() in ["Linux", "Darwin"]:
        print ("Merging with binutils...")

        if use_android_ndk:
            if platform.system() == "Darwin":
                toolchain_prefix = "darwin-x86_64"
            else:
                toolchain_prefix = "linux-x86_64"

            ar_command = f"{path_android_ndk}/toolchains/llvm/prebuilt/{toolchain_prefix}/bin/aarch64-linux-android-ar"
        else:
            ar_command = "ar"

        # Extract all object files
        os.system("mkdir %s" % AR_TEMP_DIR)
        os.chdir(AR_TEMP_DIR)
        for libname in files:
            cmdline_extract = f"{ar_command} -x {os.path.join('..', libname)}"
            retcode = os.system(cmdline_extract)
            if retcode != 0:
                print(f"Failed: {cmdline_extract}")
                sys.exit(1)

        # Extract object files from main library
        cmdline_extract = f"{ar_command} -x {os.path.join('..', main_lib)}"
        retcode = os.system(cmdline_extract)
        if retcode != 0:
            print(f"Failed: {cmdline_extract}")
            sys.exit(1)

        cmdline = f"{ar_command} -qc {os.path.basename(main_lib)} *.o" 
        retcode = os.system(cmdline)
        if retcode != 0:
            print(f"Failed: {cmdline}")
            sys.exit(1)

        # Replace original library with a new one
        os.chdir("..")
        os.remove(main_lib)
        shutil.move(os.path.join(AR_TEMP_DIR, os.path.basename(main_lib)), main_lib)
        # os.system("strip %s" % main_lib)
        shutil.rmtree(AR_TEMP_DIR)

if 'ANDROID_NDK_HOME' in os.environ:
    path_android_ndk = os.environ['ANDROID_NDK_HOME']
else:
    path_android_ndk = None
    
# "~/Library/Android/sdk/ndk/21.0.6113669"
