# R harness (haRness) for Arbor
## enables flexible execution of blocks of R code, capturing graphics, viewing tables remotely, and setting up environments

Build setup:

Will take a little bit of time the first build to download and compile R. Later iterations will probably
use system versions of R to avoid certain linking problems and simplify but avoiding that complexity for now
and also to keep the development R environment separate from personal R environment. (likely another way to approach
this too?)
```
mkdir build
cd build
cmake -S .. -B .
cmake --build .
```
## TODO:
- [ ] Call R with evaluate
- [ ] Capture textual output and error output
- [ ] Capture graphics (maybe using evaluate())
- [ ] Test library installation and see the semantics of R_HOME
- [ ] Network loop (two threads?)
- [ ] Figure out testing approaches
