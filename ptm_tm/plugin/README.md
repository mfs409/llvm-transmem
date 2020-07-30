# Full and Write-Only LLVM Plugins

The `llvm-transmem/plugin` folder is the home for the plugins that clang++ will
use in order to provide transactional instrumentation for software and hybrid TM
builds.  The plugin will transform a program by inserting calls to a
transactional library.  At link time, one of many libraries can be linked to the
program to achieve the desired TM instrumentation.  The use of LTO is
recommended, but not mandatory.

There are two versions of the plugin.  They build from the same codebase,
using a #define to determine the build.  The regular version instruments
loads and stores within transactions.  The "wo" version only instruments the
stores within transactions.

Note that the tests only run against the full version of the plugin, and all of
the benchmarks use the full version too.  The "wo" version exists only for
internal research and testing.

## Learning About the Plugin

For more information about the behavior of the plugin, see `plugin/tm_plugin.h`.
It describes the behavior of the plugin and its structure.

## Customizing the Plugin

The file `plugin/local_config.h` enables users of the plugin to modify its
behavior at compile time.  See `plugin/local_config.h` for more details about
available customizations.

## Notes about Code Quality

The folder `tests/` contains a set of unit tests that ensure that the plugin
behaves correctly across all of the cases we could come up with.  We are not
using a comprehensive test suite, so it is possible that we have less than 100%
coverage.  However, the tests are extensive enough to cover all of the expected
behaviors of the plugin.

This is not production-quality code.  Within the code, comments prefixed with
`WARNING` represent places where production-quality code would need to be
hardened relative to this code.

The plugin also does not currently handle the "-g" flag correctly in all cases.

Finally, please note that there are a few places (most notably handling of `long
double`) where the plugin assumes that it is running on an x86 CPU.
