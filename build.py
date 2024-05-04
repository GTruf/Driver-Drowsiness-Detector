# +-----------------------------------------+
# |              License: MIT               |
# +-----------------------------------------+
# | Copyright (c) 2024                      |
# | Author: Gleb Trufanov (aka Glebchansky) |
# +-----------------------------------------+

import argparse
import platform
import shutil
import os
import subprocess

argParser = argparse.ArgumentParser(prog="Driver drowsiness detector")
argParser.add_argument("--qt-cmake-prefix-path", required=True, help="Qt CMake prefix path")
argParser.add_argument("--opencv-dir", required=True, help="Directory containing a CMake configuration file for OpenCV")
argParser.add_argument("-j", "--jobs", default=4, help="Number of threads used when building")

args = argParser.parse_args()

cmakeConfigureArgs = ["cmake"]
cmakeBuildArgs = ["cmake", "--build", "."]

if platform.system() == "Linux":
    needShell = False

    # On Windows, the generator is not explicitly specified so that the required Visual Studio version of the
    # generator is detected automatically
    cmakeConfigureArgs.append("-G=Ninja")
    cmakeConfigureArgs.append("-DCMAKE_BUILD_TYPE=Release")
elif platform.system() == "Windows":
    needShell = True

    cmakeConfigureArgs.append("-DCMAKE_CONFIGURATION_TYPES=Release")
    cmakeBuildArgs.append("--config=Release")
else:
    raise Exception(f"Unexpected operating system. Got {platform.system()}, but expected Windows or Linux")

shutil.rmtree("build", ignore_errors=True)
os.mkdir("build")
os.chdir("build")

cmakeConfigureArgs.append(f"-DCMAKE_PREFIX_PATH={args.qt_cmake_prefix_path}")
cmakeConfigureArgs.append(f"-DOpenCV_DIR={args.opencv_dir}")
cmakeConfigureArgs.append("..")

cmakeBuildArgs.append(f"-j={str(args.jobs)}")

subprocess.run(cmakeConfigureArgs, shell=needShell, check=True)  # CMake configure
subprocess.run(cmakeBuildArgs, shell=needShell, check=True)  # Build
