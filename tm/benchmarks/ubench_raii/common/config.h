#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

/// config encapsulates all of the configuration behaviors that we require of
/// our benchmarks.  It standardizes the format of command-line arguments,
/// parsing of command-line arguments, and reporting of command-line arguments.
///
/// config is a superset of everything that our individual benchmarks need.  For
/// example, it has a load_factor, even though our linked list benchmark doesn't
/// need it.  The price of such generality is small, and the code savings is
/// large.
///
/// The purpose of config is not to hide information, but to reduce boilerplate
/// code.  We aren't concerned about good object-oriented design, so everything
/// is public.
struct config {
  /// Interval of time the test should run for in seconds, or the number of
  /// transactions to execute
  size_t interval = 5;

  /// Are we in timed mode, or are we in # transactions mode
  bool timed_mode = true;

  /// Our benchmark harness uses the data structures as integer sets or
  /// integer-integer maps.  This is the range for keys in the maps, and for
  /// elements in the sets.
  size_t key_range = 256;

  /// Number of threads that should execute the benchmark code
  size_t nthreads = 1;

  /// Lookup ratio.  The remaining operations will split evenly between inserts
  /// and removes
  size_t lookup = 34;

  /// If the data structure is a non-resizable map, this is the number of
  /// buckets
  size_t buckets = 128;

  /// If the data structure uses chunks, this is the size of each chunk
  size_t chunksize = 8;

  /// Should the benchmark output a lot of data, or just a little?
  bool verbose = false;

  /// Should the benchmark forgo pretty printing entirely, and output as a
  /// comma-separated list for the sake of data processing?
  bool output_raw = false;

  /// The name of the specific data structure to test
  std::string data_structure_name;

  /// The name of the specific data structure variant to test
  std::string bench_name = "tm";

  /// The name of the executable
  std::string program_name;

  /// A description of the program
  std::string bench_description;

  /// All of the possible data structure implementations that can be run
  std::vector<std::string> ds_options;

  /// A statement about any command-line options that don't pertain to a
  /// particular program
  std::string unused_options_statement;

  /// Initialize the program's configuration by setting the strings that are not
  /// dependent on the command-line
  config(std::string prog_name, std::string bench_desc,
         std::vector<std::string> ds_opts, std::string unused_stmt)
      : program_name(prog_name), bench_description(bench_desc),
        ds_options(ds_opts), unused_options_statement(unused_stmt) {}

  /// Usage() reports on the command-line options for the benchmark
  void usage() {
    using std::cout;
    using std::endl;
    cout << program_name << ":" << bench_description << endl
         << "  -b: benchmark                      (default tmlist)" << endl
         << "  -c: # chunks                       (default 8)" << endl
         << "  -i: seconds to run, or # tx/thread (default 5)" << endl
         << "  -h: print this message             (default false)" << endl
         << "  -k: key range                      (default 256)" << endl
         << "  -o: toggle output raw              (default false)" << endl
         << "  -r: lookup ratio                   (default 34%)" << endl
         << "  -s: # buckets in hashmap           (default 128)" << endl
         << "  -t: # threads                      (default 1)" << endl
         << "  -v: toggle verbose output          (default false)" << endl
         << "  -x: toggle 'i' parameter           (default true <timed mode>)"
         << endl
         << "      [ ";
    for (auto i : ds_options) {
      cout << i << " ";
    }
    cout << "]" << endl;
    if (unused_options_statement != "") {
      cout << unused_options_statement << endl;
    }
  }

  /// Parse the command-line options to initialize fields of the config object
  void init_from_args(std::string ds, int argc, char **argv) {
    data_structure_name = ds;
    long opt;
    while ((opt = getopt(argc, argv, "b:c:hi:k:or:s:t:vx")) != -1) {
      switch (opt) {
      case 'b':
        bench_name = std::string(optarg);
        break;
      case 'c':
        chunksize = atoi(optarg);
        break;
      case 'i':
        interval = atoi(optarg);
        break;
      case 'k':
        key_range = atoi(optarg);
        break;
      case 'o':
        output_raw = !output_raw;
        break;
      case 'h':
        usage();
        break;
      case 'r':
        lookup = atoi(optarg);
        break;
      case 's':
        buckets = atoi(optarg);
        break;
      case 't':
        nthreads = atoi(optarg);
        break;
      case 'v':
        verbose = !verbose;
        break;
      case 'x':
        timed_mode = !timed_mode;
        break;
      }
    }
  }

  /// Report the current values of the configuration object
  void report() {
    if (output_raw)
      return;
    using std::cout;
    using std::endl;
    cout << "configuration (bcikrstx), " << bench_name << ", " << chunksize
         << ", " << interval << ", " << key_range << ", " << lookup << ", "
         << buckets << ", " << nthreads << ", " << timed_mode << endl;
  }

  /// Report the current values of the configuration object as a comma-separated
  /// line
  void report_raw() {
    using std::cout;
    using std::endl;
    cout << data_structure_name << ",(bcikrstx)," << bench_name << ","
         << chunksize << "," << interval << "," << key_range << "," << lookup
         << "," << buckets << "," << nthreads << "," << timed_mode;
  }
};