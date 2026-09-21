# Build Banff Procedures on Unix-like systems  
Building Banff **procedures** on Linux is done with the [GNU Make tool](https://www.gnu.org/software/make/).  


## Setup build environment
Setting up the build environment on your Unix-like system will differ slightly system to system.  
**Required software**  
- `gcc` (typically via *"build-essential"*)  
- `git` (obtaining source code)  
- [if testing] `python3`  

An internet connection will be required to download open-source projects from GitHub.  

**Access the system**
If you are using Windows, the command `ssh` should be installed by default.  
Enter `ssh <server hostname or ip>` into a PowerShell, Command Line, or run box to initiate a terminal connection.  
**Obtaining source code**
Download the code from GitLab using 
`git clone https://gitlab.k8s.cloud.statcan.ca/gensys/banff/banff_redesign.git`  
Alternatively, use an *SFTP* client such as *FileZilla* or *WinSCP*.

## Quick guide: build and test
In the `ssh` session issue the following commands
- `cd <repo root>/Banff/build/unix-like/`
- `make` 

To test
- `make test`

## Build outputs
In project
- the folder `obj` is created and contains all compiled but unlinked source  
- the `bin` folder is created and contains all shared or static library builds
- the empty folders may remain after `make clean` is executed

## Detailed guide: build and test
### Summary  
The term *project* will refer to the set of source files for one library (for instance, the CommonAPI) or procedure (for instance, Donor Imputation).  

Located at `<repo root>/Banff/build/unix-like/Makefile` there are two Make files:  
- `Makefile`: the "*master*" make file which coordinates calls to all other Make files
  - "*other Make files*" being the `Makefile` found in each project
- `MakeDirPaths`: defines paths needed to build (to project roots)
  
While in this directory, issue command:  
- `make` to build all procedures (and dependencies) from a fresh `git clone` of the repo (*no configuration required!!*)
- `make test` to test the procedures with `python3`
- `make clean` to clean up all builds

All/most of the Make files support:
- `make clean` for removing all build outputs.  
- `make diagnose` for printing values of Make variables to terminal
### master `Makefile`
The Makefile located at `<repo root>/Banff/build/unix-like/Makefile` coordinates calls to all other Makefiles required to build the procedures.  
- `make` builds the open-source libraries, in-house libraries, and procedures
  - `make errorloc` and `make donorimp` make only the specific procedure, and dependencies
- `make test` runs tests on the procedures
- `make clean` cleans every build
- `make diagnose` prints values of Make variables to terminal (only from the local script)

#### `MakeDirPaths` Make file
The `MakeDirPaths` Make file defines paths to the various project roots.  Make command `include` is used to essentially call this Make file from other make files, allowing one central location for defining these paths.  The relative path to this file is hard coded into all other Make files.  
##### Hacks: pwd with Make's `include <makefile>`
When Make includes a Make file from another directory, the working directory of the process does not change.  Thus, the relative paths in `MakeDirPaths` resolve relative to `pwd` of the Make process, not to the location of the included file.  To workaround this, the relative path from Make's `pwd`  to the `MakeDirPaths` file is prepended to the relative paths which it defines.  
### open-source libraries
The *Jansson* library differs from other "in-house" libraries as it is taken from GitHub and not modified in any way.  It is included in this repository as a "*git submodule*".  It must be downloaded (via `git submodule ...`) from GitHub and configured prior to being built.  
A `Makefile` in the parent directory of the Jansson submodule now handles the git submodule setup and build.  

- `make` or `make static` will:
  - issue `git submodule init`
  - issue `git submodule update`, downloading the source
  - checkout tag `v2.14` from Jansson library
  - configure Jansson to build with "Position Independent Code (PIC)", as required 
  - execute Jansson build with `make`  
- `make deinit` will deinitialize the jansson submodule, deleting the local files in the jansson_submodule folder
### in-house libraries
For in-house libraries, such as the *CommonAPI* or *IOUtilities*, the `Makefile`'s structure will be nearly identical.  
- `make` or `make static` builds the source into a static library (`.a` file)  
- `make shared` builds the source into a shared library (`.so` file)  
### procedures
Procedures depend on the supporting libraries having been built ahead of time
- `make` or `make shared` builds the procedure as a shared library (`.so` file)  
- `make test` will execute a python script which tests the shared library build
- `make static` builds the procedure as a static library (`.a` file), however this build has never been tested  
