# notes on haRness design and impl

Turns out that `Rf_initEmbedded_R` calls `run_setupMainLoop` (something like that)
which is incompatible with `Rf_mainloop`, but not with `run_mainloop` because
`Rf_mainloop` *also* calls `run_setupMainLoop`.

Therefore, we are gonna use `Rf_initEmbedded_R` with `run_mainLoop` *if* we wanted
REPL which we don't, but it shows where the segfault was from. No fundamental problem.

CMake setup also now finds the locally installed version of R with a custom FindR.cmake
script in the `cmake/` folder.

## architecture of messages between network side and R interpreter
Goal should also be to make this easily testable. It should be a nice point of
decouling.
