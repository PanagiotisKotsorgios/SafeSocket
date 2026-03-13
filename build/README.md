# Build Directory

This directory contains the **compiled binaries, build artifacts, and generated files** for the project.

The application is **cross-platform ** that supports both **Windows and Linux**. All compiled executables, intermediate build files, and platform-specific artifacts are generated here by the build system (ex. Make).

⚠️ **Note:**
Files inside this directory are **generated automatically during the build process** and typically **should not be edited manually**.

---

## Purpose

The `build/` directory is used to:

* Store compiled **executables**
* Store **object files and intermediate build artifacts**
* Store **platform-specific build outputs**
* Keep generated files **separate from source code**

Keeping build outputs separate helps maintain a **clean repository structure** and allows easy cleaning or regeneration of builds.

---

## Building the Project

Typical workflow:

### Linux

```bash
mkdir -p build
cd build
cmake ..
make
```

### Windows (Visual Studio / CMake)

```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

## Cleaning the Build

Since this directory only contains generated files, it can safely be removed and recreated.

Example:

```bash
rm -rf build
mkdir build
```

