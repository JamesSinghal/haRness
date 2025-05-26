# R harness (haRness) for Arbor
### enables flexible execution of blocks of R code, capturing graphics, viewing tables remotely, and setting up environments

Build setup:

Will take a little bit of time the first build to download and compile R. Later iterations will probably
use system versions of R to avoid certain linking problems and simplify but avoiding that complexity for now
and also to keep the development R environment separate from personal R environment. (likely another way to approach
this too?)

You **need** R already installed. For fedora it's `r-devel` and for ubuntu `r-base-dev`.
```
mkdir build
cd build
cmake -S .. -B .
cmake --build .
```
## TODO:
- [X] Call R with evaluate
- [X] Capture textual output and error output
- [X] Capture graphics (maybe using evaluate())
- [X] Test library installation and see the semantics of R_HOME
- [X] Network loop (two threads?)
- [X] Figure out testing approaches
