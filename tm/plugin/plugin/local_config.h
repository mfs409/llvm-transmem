#pragma once

/// The purpose of this file is to provide a place where users of the plugin can
/// provide information to customize the behavior of the plugin.  For now, there
/// is only one supported customization: providing names of functions that need
/// to be treated as pure.

/// Any function named in this enum will be treated as pure by the plugin
const char *discovery_pure_overrides[] = {
    // These three functions are called by the pbzip2 benchmark, and they are
    // pure.
    "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5c_strEv",
    "_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7_M_dataEv",
    "stat",
};
