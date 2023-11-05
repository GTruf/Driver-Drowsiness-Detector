# +-----------------------------------------+
# |              License: MIT               |
# +-----------------------------------------+
# | Copyright (c) 2023                      |
# | Author: Gleb Trufanov (aka Glebchansky) |
# +-----------------------------------------+

import argparse
import platform
import shutil
import os
import subprocess

argParser = argparse.ArgumentParser(prog="Driver drowsiness detector")

qtCmakePrefixPathDefaultValue = "C:\\Qt\\6.5.2\\msvc2019_64\\lib\\cmake"  # Hardcoded default value
opencvDirDefaultValue = "C:\\opencv-4.8.0\\build\\install"  # Hardcoded default value
cmakeConfigureArgs = ["cmake"]

if platform.system() == "Linux":
    # On Windows, the generator is not explicitly specified so that the required Visual Studio version of the
    # generator is detected automatically
    cmakeConfigureArgs.append("-G Ninja")  # TODO: Build with Ninja in Linux

    qtCmakePrefixPath = "qt_pass"  # Hardcoded default value # TODO: Workability in Linux
    opencvDir = "opencv_pass"  # Hardcoded default value # TODO: Workability in Linux
elif platform.system() != "Windows":
    raise Exception(f"Unexpected operating system. Got {platform.system()}, but expected Windows or Linux")

argParser.add_argument("--qt-cmake-prefix-path", default=qtCmakePrefixPathDefaultValue, help="Qt CMake prefix path")
argParser.add_argument("--opencv-dir", default=opencvDirDefaultValue, help="Directory containing a CMake "
                                                                           "configuration file for OpenCV")
argParser.add_argument("-j", "--jobs", default=4, help="Number of threads used when building")
args = argParser.parse_args()

shutil.rmtree("build", ignore_errors=True)
os.mkdir("build")
os.chdir("build")

cmakeConfigureArgs.append(f"-DCMAKE_PREFIX_PATH={args.qt_cmake_prefix_path}")
cmakeConfigureArgs.append(f"-DOpenCV_DIR={args.opencv_dir}")
cmakeConfigureArgs.append("..")

subprocess.run(cmakeConfigureArgs, shell=True, check=True)  # CMake configure
subprocess.run(["cmake", "--build", ".", "--config", "Release", "-j", str(args.jobs)], shell=True, check=True)  # Build
