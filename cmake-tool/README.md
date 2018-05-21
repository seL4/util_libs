<!--
     Copyright 2017, Data61
     Commonwealth Scientific and Industrial Research Organisation (CSIRO)
     ABN 41 687 119 230.

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(DATA61_BSD)
-->

# CMake seL4 Build System

> Description of the CMake based build system for building the seL4 kernel and seL4 based projects

## Using projects

This section is a small tutorial on how to interact with and build a project that is using this build system.
If you are developing a project then should read the 'using in a project' section.

### CMake basics

For a complete guide to CMake you can read the [extensive documentation](https://cmake.org/cmake/help/latest/),
but for the purposes here we will assume a particular workflow with CMake involving out of tree builds.

CMake is not itself a build tool, but rather is a build generator. This means that it generates build scripts,
typically Makefiles or Ninja scripts, which will be used either by a tool like GNU Make or Ninja to perform
the actual build.

#### Pre-requisites

It is assumed that

 * CMake of an appropriate version is installed
 * You are using the Ninja CMake generator 
 * You understand how to checkout projects using the repo tool as described on the
   [Getting started](https://docs.sel4.systems/GettingStarted) page

#### Basic build initialisation

Assuming you are in the root directory of a seL4-based project you should start with

```sh
mkdir build
cd build
```

Then initialise CMake with something like

```sh
cmake -DCROSS_COMPILER_PREFIX=arm-linux-gnueabi- -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja ..
```

Breaking down what each component means

 * `-D` means we are defining a variable in the form `X=Y`
 * `CROSS_COMPILER_PREFIX` is a variable that will be used later on and contains the prefix for the gcc based
    toolchain we want to use. You cannot change your toolchain once you have initialised a build directory
 * `CMAKE_TOOLCHAIN_FILE` is variable understood by CMake and tells it to load the specified file as a
   'toolchain' file. A toolchain file is able to setup the C compiler, linker etc that should be used. In this
   case we assume a typical project layout with the seL4 kernel in a 'kernel' directory at the top level. The
   '[gcc.cmake](https://github.com/seL4/seL4/blob/master/gcc.cmake)' file in it sets up C compilers and linkers
   using the previously supplied `CROSS_COMPILER_PREFIX`
 * `-G Ninja` tells CMake that we want to generate Ninja build scripts as opposed to GNU Makefiles. Currently
   only Ninja scripts are supported by parts of the kernel
 * `..` is the path to the top level `CMakeLists.txt` file that describes this project, generally this is
   placed in the root directory so this parameter is typically `..`, but could be any path

If all goes well you should now be able to build by doing

```sh
ninja
```

And the resulting binaries will be placed in the `images/` directory

### Configuration

Many projects will have some degree of customisation available to them. Assuming a build directory that has been
initialised with CMake you can do either

```sh
ccmake ..
```

for a ncurses based configuration editor or

```sh
cmake-gui ..
```

for a graphical configuration editor.  In both invocations the path `..` should be the same path as was used in the original `cmake` invocation.

CMake itself has two different kinds of options:

 * Booleans: These are either `ON` or `OFF`
 * Strings: These can be set to any value, although they may be restricted to a set of values by whoever wrote the project.

String options can have 'hints' given to them that they should only take on one of several fixed values. The
CMake configuration editors will respect these and provide a radio selection.

As you change configuration options the CMake scripts for the project are not continuously rerun. You can explicitly
rerun by telling it to '(c)onfigure'. This may result in additional options appearing in the configuration editor,
or some options being removed, depending on what their dependencies where. For example if there is option `A` that
is dependent on option `B` being true, and you change `B` to true, `A` will not show up until you (c)onfigure and
the CMake files are reprocessed.

When you are done changing options you can either '(g)enerate and exit' or '(q)uit without generation'. If you
quit without generating then your changes will be discarded, you may do this at any time. You will only be
allowed to generate if you run (c)onfigure after doing any changes and CMake believes your configuration has
reached a fixed point.

After changing any options and generating call

```sh
ninja
```

to rebuild the project.

#### Initial configurations

If a project supports different configurations they will typically provide some configuration `.cmake` files to
allow you to initialise the project in a certain way. Configurations are provided when initialising the build
directory by passing `-C <file>` to `cmake`. For example given some typical project structure the `cmake`
in the last example could become

```sh
cmake -C../projects/awesome_project/configs/arm_debug.cmake -DCROSS_COMPILER_PREFIX=arm-linux-gnueabi- -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja ..
```

Note that multiple `-C` options can be given, although if they try and set the same options only one of the
settings will actually get used. This means in the previous example we might have two different configuration
files for `arm.cmake` and `x86.cmake`, and then two other files for `debug.cmake` and `release.cmake`. We could
now combine `arm.cmake` with either `debug.cmake` or `release.cmake`, similarly with `x86.cmake`. For example

```sh
cmake -C../projects/awesome_project/configs/arm.cmake -C../projects/awesome_project/configs/debug.cmake -DCROSS_COMPILER_PREFIX=arm-linux-gnueabi- -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja ..
```

Nothing stops you from trying to initialise with both `arm.cmake` and `x86.cmake`, but since they are probably
setting some of the same options only one will actually take effect. If the project has multiple configuration
files you should check which can be composed.

#### [sel4test](https://github.com/seL4/sel4test) example

In the previous examples we ended up with some relatively long `cmake` invocations. These can be aliased/scripted
in various ways. One such example is in the [sel4test](https://github.com/seL4/sel4test) project, which has
a script for automatically picking a toolchain and composing configuration files.

Assuming sel4test is correctly checked out and you're in the root directory you would do something like

```sh
./projects/sel4test/configure ia32 debug simulation
```

This will create a `build_ia32_debug_simulation` directory and initialise it with the `ia32.cmake`, `debug.cmake`,
`simulation.cmake` and `sel4test.cmake` files from the `projects/sel4test/configs` directory. It will also
select the system `gcc` as the cross compiler under the assumption you are building on an x86 machine.

If you configured with something like

```sh
./projects/sel4test/configure sabre verification
```

It will create a `build_sabre_verification` directory and initialise with `sabre.cmake`, `verification.cmake`,
and `sel4test.cmake`. In this case it will also set the cross compiler to `arm-linux-gnueabi-`

Not all projects have the configuration complexity of sel4test, but this serves as an example of how a given
project might simplify its configuration process.

#### CMAKE_BUILD_TYPE

The `CMAKE_BUILD_TYPE` option is an option that will appear in the CMake configuration editors that is not
defined by a project, but is rather defined by CMake itself. This option configures the kind of build to do;
release, debug, release with debug information, etc. Note that the seL4 kernel ignores this setting as due
to the way the kernel has to be built it side steps many of the CMake systems.

## Using in a project

This section describes how pieces of the build system fit together and how you might use it in a new project.
There are a few different pieces that can be fit together in different ways depending on your project's needs
and desired customisation. This is reflected in the split of files in the cmake-tool directory.

### Basic structure

The build system here is in two pieces. One piece is in the seL4 kernel repository, which has all of the basic
compiler toolchain and flags settings as well as helpers for generating configurations. The other piece is in seL4_tools/cmake-tool,
which has helpers for putting libraries and binaries together into a final system image (along with the kernel).

This structure means that the kernel is completely responsible for building itself, but exports the settings
it uses and the binaries it creates so that the rest of this build system can use it and build the final image.

The cmake-tool directory has the following files:

 * `README.md` What you are reading
 * `default-CMakeLists.txt` An example CMakeLists.txt file that you could use as the CMakeLists.txt file in
   your top level directory. All this does is include `all.cmake`, under the assumption of a directory structure
   where this repository is in a directory named `tools`. It is the intention that a projects manifest xml
   would symlink this to the top level and call it CMakeLists.txt
 * `all.cmake` Helper file that is just a wrapper around including `base.cmake`, `projects.cmake` and
   `configuration.cmake` This serves convenience for projects that just want to include those three files
   for a default configuration without making any changes between them
 * `base.cmake` Includes the kernel as a subdirectory, includes some files of common helper routines, sets up
   the basic compilation flags as exported by the kernel and then adds libsel4 and the elfloader-tool as
   buildable targets. This file essentially sets up the basic build targets (kernel, libsel4, elfloader) and
   flags after which you could start defining your own build targets through `add_subdirectory` or otherwise
 * `projects.cmake` Adds default build targets through `add_subdirectory` assuming a default project layout.
   Essentially it adds any CMakeLists.txt files it finds in any subdirectories of the projects directory
 * `configuration.cmake` Provides a target for a library called `Configuration` that emulates the legacy
   `autoconf.h` header. Since the `autoconf.h` header contained configuration variables for the *entire* project
   this rule needs to come after all other targets and scripts that might add to the configuration space.
 * `common.cmake` File included by `base.cmake` that has some generic helper routines. There should be no need
   to include this file directly
 * `flags.cmake` Sets up build flags and linker invocations based off the exported kernel flags. This is included
   by `base.cmake` and there should be no need to include this file directly
 * `init-build.sh` shell script that performs the initial configuration and generation for a new CMake build directory.
 * `helpers/*` helper functions that are commonly imported by `common.cmake`

### Kernel directory

For simplicity of the common case `base.cmake` defaults to assuming that the seL4 kernel is in directory called
`kernel` that is in the same directory of wherever `base.cmake` is included from. This means that if you have a
directory structure like

```none
awesome_system/
├── kernel/
│   └── CMakeLists.txt
├── projects/
│   ├── awesome_system/
│   │   └── CMakeLists.txt
│   └── seL4_libs/
│       └── CMakeLists.txt
├── tools/
│   └── cmake-tool/
│       ├── base.cmake
│       ├── all.cmake
│       └── default-CMakeLists.txt
├── .repo/
└── CMakeLists.txt -> tools/cmake-tool/default-CMakeLists.txt
```

Then when `awesome_system/` is used used as the root source directory to initialise a CMake build directory
the `tools/cmake-tool/all.cmake` file is included, that then includes `base.cmake`, which will then look for
`awesome_system/kernel` as the directory of the kernel.

If you decided to put the kernel into a differently named directory, for example:

```none
awesome_system/
├── seL4/
│   └── CMakeLists.txt
├── projects/
│   ├── awesome_system/
│   │   └── CMakeLists.txt
│   └── seL4_libs/
│       └── CMakeLists.txt
├── tools/
│   └── cmake-tool/
│       ├── base.cmake
│       ├── all.cmake
│       └── default-CMakeLists.txt
├── .repo/
└── CMakeLists.txt -> tools/cmake-tool/default-CMakeLists.txt
```

Then you could override the default kernel location by passing `-DKERNEL_PATH=seL4` when first invoking `cmake`

### Advanced structures

Suppose you wanted to completely go away from the normal directory structure and instead have something like

```none
awesome_system/
├── seL4/
│   └── CMakeLists.txt
├── awesome/
│   └── CMakeLists.txt
├── seL4_libs/
│   └── CMakeLists.txt
├── buildsystem/
│   └── cmake-tool/
│       ├── base.cmake
│       ├── all.cmake
│       └── default-CMakeLists.txt
└── .repo/
```

In this example there is

 * No `CMakeLists.txt` file in the root directory
 * `tools` directory has been renamed
 * `kernel` directory has been renamed
 * No `projects` directory

If we want the `CMakeLists.txt` in the `awesome_system/awesome` directory then would initialise CMake,
assuming a build directory that is also in the `awesome_system` directory, do something like

```sh
cmake -DCROSS_COMPILER_PREFIX=toolchain-prefix -DCMAKE_TOOLCHAIN_FILE=../seL4/gcc.cmake -DKERNEL_PATH=../seL4 -G Ninja ../awesome
```

What is important here is that the path for `CMAKE_TOOLCHAIN_FILE` is resolved immediately by CMake, and so is
relative to the build directory, where as the `KERNEL_PATH` is resolved whilst processing `awesome_system/awesome/CMakeLists.txt`
and so is relative to that directory.

The contents of `awesome_system/awesome/CMakeLists.txt` would be something like

```cmake
cmake_minimum_required(VERSION 3.7.2)
include(../buildsystem/cmake-tool/base.cmake)
add_subdirectory(../seL4_libs seL4_libs)
include(../buildsystem/cmake-tool/configuration.cmake)
```

This looks pretty much like `all.cmake` except that we do not include `projects.cmake` as we do not have a projects
folder. It wouldn't be harmful to include it since it would just resolve no files, but is redundant. We cannot
simply include `all.cmake` was we need to include our subdirectories (in this case just seL4_libs) between setting
up the base flags and environment and finalising the Configuration library. We needed to give an explicit build
directory (the second argument in `add_subdirectory`) as we are giving a directory that is not a subdirectory of
the root source directory.

For simplicity, the kernel path could be encoded directly into the projects CMakeLists.txt, so you could
add

```cmake
set(KERNEL_PATH ../seL4)
```

before

```cmake
include(../buildsystem/cmake-tool/base.cmake)
```

in `awesome_system/awesome/CMakeLists.txt`, removing the need for `-DKERNEL_PATH` in the `cmake` invocation.

### Configuration

To provide a configuration system that was compatible with how the previous build system provided configuration
various helpers and systems exist to:

 * Automate configuration variables that appear in the cmake-gui with various kinds of dependencies
 * Generate C configuration headers that declare these variables in format similar to what Kconfig did
 * Generate 'autoconf.h' headers so old code that does `#include <autoconf.h>` still work

A simple fragment of a CMake script that demonstrates how these three things fit together is

```cmake
set(configure_string "")
config_option(EnableAwesome HAVE_AWESOME "Makes library awesome" DEFAULT ON)
add_config_library(MyLibrary "${configure_string}")
generate_autoconf(MyLibraryAutoconf "MyLibrary")
target_link_libraries(MyLibrary PUBLIC MyLibrary_Config)
target_link_libraries(LegacyApplication PRIVATE MyLibrary MyLibraryAutoconf)
```

Stepping through line by line

 * `set(configure_string "")` for simplicity the various `config_*` helpers automatically add to a variable called
   `configure_string`, so we become by making sure this is blank
 * `config_option(EnableAwesome HAVE_AWESOME "Makes library awesome" DEFAULT ON)` this declares a configuration
   variable that will appear in CMake scripts and the cmake-gui as `EnableAwesome` and will appear in the generated
   C header as `CONFIG_HAVE_AWESOME`
 * `add_config_library(MyLibrary "${configure_string}")` generates a `MyLibrary_Config` target, which is an interface
   library that has a generated C header based on the provided configuration string. It also adds `MyLibrary` to
   a global list of configuration libraries. This global list can be used if you want to generate a library that
   contains "all the configurations in the system" (which is what the original `autoconf.h` was)
 * `generate_autoconf(MyLibraryAutoconf "MyLibrary")` generates a `MyLibraryAutoconf` target, which is an interface
   library that depends upon `MyLibrary_Config` and will provide an `autoconf.h` file that includes the configuration
   header from `MyLibrary_Config`
 * `target_link_libraries(MyLibrary PUBLIC MyLibrary_Config)` allows `MyLibrary` to `#include` the generated
   configuration header by doing `#include <MyLibrary/gen_config.h>`
 * `target_link_libraries(LegacyApplication PRIVATE MyLibrary MyLibraryAutoconf)` allows `LegacyApplication` to
   `#include <autoconf.h>` from `MyLibraryAutoconf`. The `autoconf.h` in this case will contain `#include <MyLibrary/gen_config.h>`

For more details of the different `config_*` helpers read the comments on the functions in `kernel/tools/helpers.cmake`

## Gotchas

List of gotchas and easy mistakes that can be made when using cmake

 * Configuration files passed to to cmake with `-C` *must* end in `.cmake`, otherwise CMake will silently throw
   away your file
