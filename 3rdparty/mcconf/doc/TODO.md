# Tasks and Ideas for Improvement

## Convenience

* -Dsymbol command line arguments to add preprocessor definitions
* select additional modules via command line arguments
* Add a command line option do add additional moduledirs.

## Diagnostics

* Also output a graph with just the modules of the current
  configuration.
* highlight included modules of the configuration in the global dot graph 
* --find to find which modules provide this file and where these
  variants can be found.
* Output a human readable text file (TOML and XML) of the module
  database including all automatically extracted information.
* Find all cycles in the dependency graph and report these. Dependency
  cycles are an indicator for bad design and prevent reuse.
* let --check complain about all selected modules that are not part of
  a conflict clique. We do not need to manually include these because
  they are obvious even to the simplest greedy dependency solver.

## Configuration Process

* add automatic dependency resolution:
  * greedy conservative: For each open dependency add the provider
    module if just a single module provides the dependency. Otherwise,
    tell the user to extend the configuration. With this strategy
    explicit module selection is needed just once per conflict
    clique. This has the advantage that there is no magic and the user
    gets exactly what he requested.
  * brute force: enumerate all feasible solutions. Then we calculate
    the weight of each solution, ie by counting the number of included
    modules or files or the lines of code and select the smallest
    feasible solution. Please keep in mind that the same intermediate
    solutions can be reached via many different pathes and there are
    2^n solutions (n=#modules).
  * SAT solver: each module is modelled as a boolean variable (can be
    either used by the solution or not). The dependencies and
    conflicts are modelled through clauses. Let module m require x and
    x is provided by modules y1...yn, then add the clause "m => y1 or
    ... or y2". Let x be provided by more than one module y1...yn,
    then add a mutual exclusion clause for y1...yn.
	Might need formulation as LP-Problm, see:
	* http://0install.net/solver.html
	* https://github.com/enthought/depsolver and https://speakerdeck.com/cournape/depsolver-pycon-dot-fr
	* http://sahandsaba.com/understanding-sat-by-implementing-a-simple-sat-solver-in-python.html
	* https://pypi.python.org/pypi/PuLP
	* https://github.com/dokelung/msat
	* https://pypi.python.org/pypi/logilab-constraint/0.5.0
	* http://pyeda.readthedocs.org/en/latest/sudoku.html
	* https://pypi.python.org/pypi/pycosat

## Semantics

* Should destdir in the config file be relative to the config file or
  relative to the current working directory when calling mcconf?
  Relative to the config file is consistent with all other file
  references. A path relative to the current working directory can
  always be supplied via the command line.
* Are the moduledirs search pathes relative to the config file or
  relative to the current working directory?
* how to handle files that need rewriting instead of copying (ie
  templates)? for example, in order to insert pathes to external
  tools? autoconf has these .in template files and we could have a
  pythonic variant of this. Generalize the makefile generator into a
  makefile template
