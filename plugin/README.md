# llvm-tm::/plugin

This folder contains the source code for an LLVM plugin that performs
transactional instrumentation.  The plugin will transform a program by inserting
calls to a transactional library.  At link time, one of many libraries can be
linked to the program to achieve the desired TM instrumentation.

## Learning About the Plugin

For more information about the behavior of the plugin, see `tm_plugin.h`.  It
describes the behavior of the plugin and its structure.

## Customizing the Plugin

The file `local_config.h` enables users of the plugin to modify its behavior at
compile time.  See `local_config.h` for more details about available
customizations.

## Notes about Code Quality

The folder `../tests/` contains a set of unit tests that ensure that the plugin
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

## Submitting Patches

We welcome the submission of pull requests and the creation of trackable issues
to aid in the improvement of this code.