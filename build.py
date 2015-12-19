# 
# Copyright (c) 2015 Christoph Husse
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
# IN THE SOFTWARE.
# 

import os, sys, shutil, subprocess, argparse, time, re, traceback
import platform as modPlatform

currDir = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/").replace("\\", "/")
platform = ""

###################################################################################################
######################### UTILITY FUNCTIONS
################################################################################################### 

def logInfo(msg):
    sys.stdout.write(str(msg) + "\n")
    sys.stdout.flush()

def logError(msg):
    sys.stderr.write("[ERROR]: " + str(msg) + "\n")
    sys.stderr.flush()

def logFatal(msg):
    sys.stderr.write("[FATAL]: " + str(msg) + "\n")
    sys.stderr.flush()
    exit(-1)

def findExeOrNone(program):

    for ext in ["", ".exe"]:
        def is_exe(exePath):
            return os.path.isfile(exePath) and os.access(exePath, os.X_OK)

        fpath, fname = os.path.split(program + ext)
        if fpath:
            if is_exe(program + ext):
                return program + ext
        else:
            for path in os.environ["PATH"].split(os.pathsep):
                path = path.strip('"')
                exe_file = os.path.join(path, program + ext)
                if is_exe(exe_file):
                    return exe_file

    return None

def findExeOrFail(program):

    result = findExeOrNone(program)
    if(result):
        return result

    logFatal("Unable to find program '" + program + "'.")
    exit(-1)

def testFolderOrFail(folder, fileListToTest):
    if(not folder or len(folder) == 0):
        return None

    folder = os.path.abspath(folder) + "/"
    for f in fileListToTest:
        testPath = os.path.abspath(folder + f);
        if(not os.path.isfile(testPath)):
            logFatal("Folder '" + folder + "' is invalid, since it does not contain the file '" + testPath + "'.")

    return folder

def call(exe, args):

    #logInfo("Current directory: '" + os.getcwd() + "'")
    #logInfo("Running command '" + exe + " \"" + "\" \"".join(args) + "\"'")
    #logInfo("")

    try:
        exitCode = subprocess.call([exe] + args)

        if(exitCode != 0):
            logFatal("Command '" + exe + " \"" + "\" \"".join(args) + "\"' failed with error code " + str(exitCode) + ".")

    except Exception as e:
        logFatal("Command '" + exe + " \"" + "\" \"".join(args) + "\"' failed with exception '" + str(e) + "'.")

def callCMake(args): 
    if("windows" in platform):
        call(depsPlatformDir + "/tools/CMakeRunner.exe", [cmake] + args)
    else:
        call(cmake, args)

###################################################################################################
######################### BUILD DEPENDENCY TREE
###################################################################################################

buildableModules = {
    "SDLViewer": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib", "DRViewerLib"]
    },

    "UnitTests": { 
        "os": [], 
        "deps": [] 
    },

    "DesktopMirror_dxgi": { 
        "os": ["windows"], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "DesktopMirror_gdi": { 
        "os": ["windows"], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "HostProtocol_default": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib", "RawProtocols"] 
    },

    "UserInterface_html": { "os": [], "deps": [] },

    "VideoDecoder_ffmpeg": { 
        "os": ["linux"], 
        "deps": ["CFrameworkLib"] 
    },

    "VideoEncoder_ffmpeg": { 
        "os": ["linux"], 
        "deps": ["CFrameworkLib"] 
    },

    "VideoEncoder_inde_mfx": { 
        "os": ["windows"], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "VideoEncoder_nvenc": { 
        "os": ["windows"], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "ViewerProtocol_default": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib", "RawProtocols"] 
    },
    
    "DRViewerLib": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "ProtocolServer": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "DRHost": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "RawProtocols": { 
        "os": [], 
        "deps": ["CFrameworkLib", "CppFrameworkLib"] 
    },

    "CFrameworkLib": { "os": [], "deps": [] },
    "CppFrameworkLib": { "os": [], "deps": ["CFrameworkLib"] }
}

###################################################################################################
######################### PARSING PARAMETERS
################################################################################################### 

parser = argparse.ArgumentParser(description='Builds DirectRemote Apps.')

# required parameters

# optional parameters
parser.add_argument('--build-directory', dest='buildDir', action='store', default=currDir + "/_build", help='A directory where intermediate build artifacts are created (sort of a temporary directory).')
parser.add_argument('--artifact-directory', dest='artifactDirectory', action='store', default=None, help='A directory where final build artifacts are created.')

parser.add_argument('--target', dest='target', action='store', default="self", help="The target environment to build for.")
parser.add_argument('--verbose', dest='verbose', action='count', default=0, help="Display more info to debug issues.")

parser.add_argument('--enable', dest='enabledModules', nargs="+", type=str, help="Can be used multiple times to enable the build of one module at a time.")
parser.add_argument('--enable-all', dest='flag_EnableAll', action='count', default=0, help="Enable all modules that are supported for the target operating system.")
parser.add_argument('--static-plugins', dest='flag_StaticPlugins', action='count', default=0, help="Hardwire plugins into viewer. Currently, only Windows supports dynamic plugins. This flag only acts on the viewer, not on the host and should only be used for platforms other than Windows.")
parser.add_argument('--static-linkage', dest='flag_StaticLinkage', action='count', default=0, help="Hardwire plugins into viewer. Currently, only Windows supports dynamic plugins. This flag only acts on the viewer, not on the host and should only be used for platforms other than Windows.")
parser.add_argument('--release', dest='flag_Release', action='count', default=0, help="Build with optimizations enabled (full release build, with debug symbols).")

parser.add_argument('--clean', dest='clean', action='count', default=0, help='Delete build directory before building.')
parser.add_argument('--clean-artifacts', dest='cleanOutput', action='count', default=0, help='Delete build artifact directory before building.')

parser.add_argument('--7z', dest='zip7', action='store', default="7z", help='A valid path to your 7zip executable (reqired for deployment).')
parser.add_argument('--make-package', dest='makePackage', action='count', default=0, help='Should the output directory be populated into a deployable package including a 7zip compressed package of itself?.')

parser.add_argument('--clang-suffix', dest='clangSuffix', action='store', default="", help='A suffix to append to clang binaries. "-3.7" would expand "clang-tidy" to "clang-tidy-3.7".')
parser.add_argument('--clang-prefix', dest='clangPrefix', action='store', default="", help='A prefix to prepend to clang binaries. "~/llvm/" would expand "clang-tidy" to "~/llvm/clang-tidy".')
parser.add_argument('--verify-style', dest='verifyStyle', action='count', default=0, help="Format and Lint all code according to Google coding standards and apply auto-fix where possible.")

# GATHER SYSTEM INFO

if sys.platform == "linux" or sys.platform == "linux2":
    osName = "linux"
    platform = "linux-"
elif sys.platform == "darwin":
    osName = "darwin"
    platform = "darwin-"
elif sys.platform == "win32":
    osName = "windows"
    platform = "windows-"
else:
    logFatal("Unable to determine host platform.")

platform += {'AMD64': 'x86_64', 'x86_64': 'x86_64', 'i386': 'i386', 'x86': 'i386'}[modPlatform.machine()]
depsDir = os.path.abspath(currDir + "/dependencies/") + "/";
depsPlatformDir = os.path.abspath(depsDir + platform + "/") + "/";
cmakeGenerator = None

args = parser.parse_args()

target = args.target
toolchainFile = None

if("self" in target):
    target = platform
    targetOsName = osName
else:
    if(not ("windows" in target)):
        toolchainFile = os.path.abspath(currDir + "/cmake/Toolchain-" + target + ".cmake")
        if(not os.path.isfile(toolchainFile)):
            logFatal("Target '" + target + "' requires a toolchain file at '" + toolchainFile + "'.")

platform = target

if(platform == "windows-x86_64"):
    cmakeGenerator = "Visual Studio 12 2013 Win64"
elif(platform == "windows-i386"):
    cmakeGenerator = "Visual Studio 12 2013"
elif("windows" in platform):
    logFatal("Unknown architecture '" + platform + "' for Windows OS.")

if(args.artifactDirectory == None):
    args.artifactDirectory = currDir + "/bin/" + platform + "/"

###################################################################################################
######################### POSTPROCESSING CONFIGURATION
################################################################################################### 

buildDir = os.path.abspath(args.buildDir)
artifactDirectory = os.path.abspath(args.artifactDirectory)
zip7 = findExeOrNone(args.zip7)
flag_StaticPlugins = args.flag_StaticPlugins > 0 
flag_StaticLinkage = args.flag_StaticLinkage > 0 
flag_EnableAll = args.flag_EnableAll > 0
verbose = args.verbose > 0
flag_Release = args.flag_Release > 0 
makePackage = args.makePackage > 0
cmake = findExeOrFail(os.path.abspath(depsPlatformDir + "/tools/cmake/bin/cmake"))
enabledModules = []

clangPrefix = args.clangPrefix
clangSuffix = args.clangSuffix

if(len(clangPrefix) == 0) and (os.name == "nt"):
    clangPrefix = os.path.abspath(depsPlatformDir + "/tools/clang-format/") + "/"

def clangExe(name):
    return clangPrefix + name + clangSuffix

clangFormat = findExeOrNone(clangExe("clang-format"))

if(len(clangSuffix) == 0) and clangFormat == None:
    highest = ""
    for major in range(3,6):
        for minor in range(0,20):
            clangSuffix = "-" + str(major) + "." + str(minor)
            if(findExeOrNone(clangExe("clang-format")) != None):
                highest = clangSuffix

    clangSuffix = highest

clangFormat = findExeOrFail(clangExe("clang-format"))
verifyStyle = args.verifyStyle > 0 

if not (os.name == "nt") and verifyStyle:
    clangC = findExeOrFail(clangExe("clang"))
    clangCpp = findExeOrFail(clangExe("clang++"))
    clangTidy = findExeOrNone(clangExe("clang-tidy"))
    clangModernizer = findExeOrNone(clangExe("clang-modernizer"))
    os.environ["CC"] = clangC
    os.environ["CXX"] = clangCpp
else:
    clangC = None
    clangCpp = None
    clangTidy = None
    clangModernizer = None

logInfo("")
logInfo("Running buildscript with following configuration:")
logInfo("    Project-Directory = '" + currDir + "'")
logInfo("    Target-Environment = '" + target + "'")
logInfo("    Build-Directory = '" + buildDir + "'")
logInfo("    Artifact-Directory = '" + artifactDirectory + "'")
logInfo("    CMake = '" + cmake + "'")
logInfo("    Make Package = '" + str(makePackage) + "'")

###################################################################################################
######################### VALIDATING CONFIGURATION
################################################################################################### 

enabledModulesMap = {}

if not args.enabledModules:
    args.enabledModules = []

for module in args.enabledModules:
    if(not module in buildableModules):
        logFatal("Specificied module '" + module + "' is unknown.")

    modInfo = buildableModules[module]
    enabledModulesMap[module] = True

    for dep in modInfo["deps"]:
        if(not dep in buildableModules):
            logFatal("Internal error. Dependency '" + dep + "' is unknown.")

        enabledModulesMap[dep] = True

if(flag_EnableAll):
    for (module, modInfo) in buildableModules.items():
        if(len(modInfo["os"]) > 0):
            if not (targetOsName in modInfo["os"]):
                continue

        enabledModulesMap[module] = True

        for dep in modInfo["deps"]:
            if(not dep in buildableModules):
                logFatal("Internal error. Dependency '" + dep + "' is unknown.")

            enabledModulesMap[dep] = True   

logInfo("    Building modules:")
enabledModules = []
for (module, flag) in enabledModulesMap.items():
    logInfo("        > " + module)
    enabledModules += [module]

###################################################################################################
######################### GENERATE CMAKE PROJECT
################################################################################################### 

def cmakeDefaultIfEmpty(name, path):
    if(path and len(path) > 0):
        path = os.path.abspath(path)
        logInfo("    " + name + " = '" + path + "'")
    else:
        logInfo("    " + name + " = [CMAKE-DEFAULT]")
        path = None

    return path

if(cmakeGenerator != None):
    logInfo("    CMake-Generator = '" + cmakeGenerator + "'")
else:
    logInfo("    CMake-Generator = [CMAKE-DEFAULT]")
    cmakeGenerator = None

if(zip7 and len(zip7) > 0):
    logInfo("    7Zip = '" + zip7 + "'")
else:
    logInfo("    7Zip = [DISABLED]")
    zip7 = None

logInfo("")

if(args.clean > 0) and os.path.exists(buildDir):
    logInfo("Deleting build directory...")
    shutil.rmtree(buildDir)

if(args.cleanOutput > 0) and os.path.exists(buildArtifactsRoot):
    logInfo("Deleting build artifact directory...")
    shutil.rmtree(buildArtifactsRoot, ignore_errors = True)

if not os.path.exists(buildDir):
    logInfo("Creating build directory (which did not exist).")
    time.sleep(1)
    os.makedirs(buildDir)
else:
    logInfo("Build directory already exists.")

def genWithCMake(srcDir):
    cmakeArgs = [
            "-DARTIFACT_DIRECTORY=" + artifactDirectory,
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ]

    if(cmakeGenerator):
        cmakeArgs += ["-G", cmakeGenerator]

    if(toolchainFile):
        cmakeArgs += ["-DCMAKE_TOOLCHAIN_FILE='" + toolchainFile + "'"]

    for module in enabledModules:
        cmakeArgs += ["-DBUILD_" + module + "=ON"]

    if(flag_StaticPlugins):
        cmakeArgs += ["-DENABLE_STATIC_PLUGINS=ON"]

    if(flag_StaticLinkage):
        cmakeArgs += ["-DENABLE_STATIC_LINKAGE=ON"]

    callCMake(cmakeArgs + [srcDir]) 


logInfo("")
logInfo("###############################################################")
logInfo("### Generate CMake project")
logInfo("###############################################################")
logInfo("")

os.chdir(buildDir)
genWithCMake(currDir)

###################################################################################################
######################### STYLE CHECK
################################################################################################### 

styleCheckIncludes = []
styleCheckExcludes = []

for pattern in ["/(tests|viewers|plugins|include|core)/.*\.(cpp|c|mm|cxx|cc|h|hpp)$"]:
    styleCheckIncludes += [re.compile(pattern)]

for pattern in ["\.git", "/dependencies/", "/sdksample/"]:
    styleCheckExcludes += [re.compile(pattern)]

def runStyleCheckOnFile(path):
    if(verbose):
        logInfo("Processing file at '" + path + "'.")

    if(clangTidy and not (path.endswith(".h") or path.endswith(".hpp"))):
        checkArgs = [clangTidy, path,
             '-checks=*,-google-build-using-namespace',
             '-header-filter=DirectRemote/(core|plugins|tests|include|viewers)/.*.h',
             #'-fix', '-fix-errors',
             '-p',
             buildDir + '/compile_commands.json'];

        try:
            process = subprocess.Popen(" ".join(checkArgs), shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
            content = process.stdout.read()

            if((process.returncode != 0) or re.search("\\n[^\\s]+.*\\:\\d+\\:\\d+\\:\\s(warning|error)\\:\\s", content) != None):
                subprocess.call(checkArgs)
                return False
        except Exception as e:
            logFatal("File at '" + path + "' could not be linted.")

    exitCode = subprocess.call([clangFormat, "-i", path])

    if(exitCode != 0):
        logFatal("File at '" + path + "' could not be formatted.")

    return True

def runStyleCheckOnDirectory(dir):
    hasErrors = False

    dir = os.path.abspath(dir);
    expandedDir = dir.replace("\\", "/")

    for dirName, subdirList, fileList in os.walk(dir):
        for fileName in fileList:
            shouldFormat = False
            dirName = os.path.abspath(dirName)
            path = os.path.abspath(dirName + "/" + fileName).replace("\\", "/")

            for pattern in styleCheckIncludes:
                if(pattern.search(path) != None):
                    shouldFormat = True

            for pattern in styleCheckExcludes:
                if(pattern.search(path) != None):
                    shouldFormat = False

            if(shouldFormat):
                hasErrors = not runStyleCheckOnFile(path) or hasErrors

    return not hasErrors

if(verifyStyle):
    if (not runStyleCheckOnDirectory(currDir)):
        logFatal("Style-check found at least one issue, aborting.")
        exit(-1)

    logInfo("Style-check completed successfully.")

    exit(0)

###################################################################################################
######################### ACTUAL BUILD
################################################################################################### 

logInfo("")
logInfo("###############################################################")
logInfo("### Build full project")
logInfo("###############################################################")
logInfo("")

genWithCMake(currDir)

if(flag_Release):
    callCMake(["--build", "./", "--config", "Debug"])
else:
    callCMake(["--build", "./", "--config", "Release"])

logInfo("")
logInfo("###############################################################")
logInfo("### Run UnitTests")
logInfo("###############################################################")
logInfo("")

#os.chdir(buildArtifactsRoot + "/bin/")
#call("./unittests.exe", ["--gtest_output=xml:unittests.xml"])
#os.chdir(buildDir)


logInfo("")
logInfo("###############################################################")
logInfo("### BUILD COMPLETED SUCCESSFULLY")
logInfo("###############################################################")
logInfo("")

exit(0)