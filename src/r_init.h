#pragma once
// contains init sequence of the R environment
//
// first might be handling to execute a set of R snippets for
// loading, checking certain conditions, library init, etc...
// needs to exec these and check each for errors, and handle errors
//
// fatal error handling as well
//
// might be a class that R snippets get registered with and then
// execute in one call
//
// how to register the code? would probably be static but should be testable?

namespace RWorker {
// throws if R is not initialized already
bool exec_R_setup();
} // namespace RWorker
