#!/usr/bin/python
import sys
import os
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "../python-libs"))
import pytoml as toml
import logging
import argparse
import re
#import pydotplus.graphviz as pydot
import pydot
import shutil

from mako.template import Template

MODEXT = '.module'

class Module:
    def __init__(self, name, modulefile):
        self.name = name
        self.modulefile = modulefile
        self.destdir = ""
        # files is a dict from file-type (e.g. incfiles, kernelfiles) to a set of filenames
        # TODO should be a dict from dest-name to source name and so on. But need own MFile class for this with a reference to the owning module etc. Then, all module selection code has to be updated as well...
        self.files = dict()
        self.requires = set()
        self.provides = set()
        self.modules = set()
        self.copyfiles = set()
        self.makefile_head = None
        self.makefile_body = None
        self.extra = dict() # all unknown fields from the configuration
        self.providedFiles = set()
        self.requiredFiles = set()

    def getRequires(self):
        return self.requires | self.requiredFiles

    def getProvides(self):
        return self.provides | self.providedFiles

    def findProvidedFiles(self):
        provfiles = set()
        for ftype in self.files: provfiles |= self.files[ftype]
        return provfiles

    def findRequiredIncludes(self, files):
        incrgx = re.compile('#include\\s+[<\\"]([\\w./-]+)[>\\"]')
        basedir = os.path.dirname(os.path.abspath(self.modulefile))
        includes = set()
        for f in files:
            fpath = basedir+'/'+f
            try:
                with open(fpath) as fin:
                    content = fin.read()
                    for m in incrgx.finditer(content):
                        inc = m.group(1)
                        # if file is locally referenced e.g. 'foo' instead of 'path/to/foo'
                        if os.path.exists(os.path.dirname(fpath)+'/'+inc):
                            inc = os.path.relpath(os.path.dirname(fpath)+'/'+inc, basedir)
                        includes.add(inc)
            except Exception as e:
                logging.warning("could not load %s from %s: %s", fpath, self.modulefile, e)
        return includes - files

    def init(self):
        self.providedFiles = self.findProvidedFiles()
        self.requiredFiles = self.findRequiredIncludes(self.providedFiles)

    def __repr__(self):
        return self.name

def createModulesGraph(moddb):
    graph = pydot.Dot(graph_type='digraph')
    nodes = dict()

    # add modules as nodes
    for mod in moddb.getModules():
        tt = ", ".join(mod.getProvides()) + " "
        node = pydot.Node(mod.name, tooltip=tt)
        nodes[mod.name] = node
        graph.add_node(node)

    # add directed edges from modules to modules that satisfy at least one dependency
    for src in moddb.getModules():
        dstmods = moddb.getSolutionCandidates(src)
        for dst in dstmods:
            tt = ", ".join(dstmods[dst]) + " "
            edge = pydot.Edge(src.name, dst.name, tooltip=tt)
            graph.add_edge(edge)

    # add special directed edges for "modules" inclusion
    for src in moddb.getModules():
        for dstname in src.modules:
            dst = moddb[dstname]
            edge = pydot.Edge(src.name, dst.name, color="green")
            graph.add_edge(edge)

    # add undirected edges for conflicts
    for src in moddb.getModules():
        conflicts = moddb.getConflictingModules(src)
        for dst in conflicts:
            if (dst.name < src.name):
                tt = ", ".join(conflicts[dst]) + " "
                edge = pydot.Edge(src.name, dst.name, color="red", dir="none", tooltip=tt)
                graph.add_edge(edge)


    graph.write('dependencies.dot')

def createConfigurationGraph(modules, selectedmods, moddb, filename):
    graph = pydot.Dot(graph_name="G", graph_type='digraph')
    nodes = dict()

    # add modules as nodes
    for mod in modules:
        tt = ", ".join(mod.getProvides()) + " "
        if mod.name in selectedmods:
            fc = "#BEF781"
        else:
            fc = "white"
        if moddb.getConflictingModules(mod):
            nc = "#DF0101"
        else:
            nc = "black"

        node = pydot.Node(mod.name, tooltip=tt,
                          style='filled', fillcolor=fc, color=nc, fontcolor=nc)
        # node = pydot.Node(mod.name)
        nodes[mod.name] = node
        graph.add_node(node)

    # add directed edges from modules to modules that satisfy at least one dependency
    for src in modules:
        dstmods = moddb.getSolutionCandidates(src)
        #print(str(src) + ' --> ' + str(dstmods))
        # don't show modules that are not in 'modules'
        for dst in dstmods:
            if dst not in modules: continue
            tt = ", ".join(dstmods[dst]) + " "
            edge = pydot.Edge(src.name, dst.name, tooltip=tt)
            # edge = pydot.Edge(src.name, dst.name)
            graph.add_edge(edge)

    # add special directed edges for "modules" inclusion
    for src in modules:
        for dstname in src.modules:
            dst = moddb[dstname]
            edge = pydot.Edge(src.name, dst.name, color="green")
            graph.add_edge(edge)

    graph.write(filename)

def parseTomlModule(modulefile):
    modules = list()
    with open(modulefile, 'r') as f:
        content = toml.load(f)
        for name in content['module']:
            fields = content['module'][name]
            mod = Module(name, modulefile)
            for field in fields:
                if field.endswith('files'): mod.files[field.upper()] = set(fields[field])
                elif field == 'copy': mod.copyfiles = set(fields['copy'])
                elif field == 'requires': mod.requires = set(fields['requires'])
                elif field == 'provides': mod.provides = set(fields['provides'])
                elif field == 'modules': mod.modules = set(fields['modules'])
                elif field == 'destdir': mod.destdir = fields['destdir']
                elif field == 'makefile_head': mod.makefile_head = fields['makefile_head']
                elif field == 'makefile_body': mod.makefile_body = fields['makefile_body']
                else: mod.extra[field] = fields[field]
            #mod.provides.add(name) # should not require specific modules, just include them directly!
            mod.init()
            modules.append(mod)
    return modules

def searchFiles(basedir, extension):
    files = list()
    if basedir[-1:] != '/': basedir += '/'
    for f in os.listdir(basedir):
        fname = basedir + f
        if os.path.isdir(fname):
            files.extend(searchFiles(fname, extension))
        elif f.endswith(extension):
            files.append(fname)
    return files



class ModuleDB:
    def __init__(self):
        self.modules = dict()
        self.provides = dict()
        self.requires = dict()

    def addModule(self, mod):
        if mod.name not in self.modules:
            logging.debug('loaded %s from %s', mod.name, mod.modulefile)
            self.modules[mod.name] = mod
            for tag in mod.getProvides(): self.addProvides(tag, mod)
            for tag in mod.getRequires(): self.addRequires(tag, mod)
        else:
            logging.warning('ignoring duplicate module %s from %s and %s',
                            mod.name, mod.modulefile,
                            self.modules[mod.name].filename)

    def addModules(self, mods):
        for mod in mods: self.addModule(mod)

    def loadModulesFromPaths(self, paths):
        for path in paths:
            files = searchFiles(path, MODEXT)
            for f in files:
                try:
                  self.addModules(parseTomlModule(f))
                except:
                  logging.error('parsing  modulefile %s', f)
                  raise

    def addProvides(self,tag,mod):
        """Tell that the tag is provided by that module."""
        if tag not in self.provides:
            self.provides[tag] = set()
        self.provides[tag].add(mod)

    def addRequires(self,tag,mod):
        """Tell that the tag is required by that module."""
        if tag not in self.requires:
            self.requires[tag] = set()
        self.requires[tag].add(mod)

    def __getitem__(self,index):
        return self.modules[index]

    def has(self, key):
        return key in self.modules

    def getProvides(self, tag):
        """Return a set of modules that provide this tag."""
        if tag not in self.provides:
            return set()
        return self.provides[tag]

    def isResolvable(self, mod, provided):
        """ Test whether there is a chance that the dependency are resolvable. """
        for req in mod.getRequires():
            if not (req in provided or len(self.getProvides(req))):
                logging.warning('Discarded module %s because of unresolvable dependency on %s', mod.name, req)
                return False
        return True

    def getResolvableProvides(self, tag, provided):
        return set([mod for mod in self.getProvides(tag) if self.isResolvable(mod, provided)])

    def getRequires(self, tag):
        """Return a set of modules that require this tag."""
        if tag not in self.requires:
            return set()
        return self.requires[tag]

    def getModules(self):
        return self.modules.values()

    def getSolutionCandidates(self, mod):
        """Find all modules that satisfy at least one dependency of the module.
        Returns a dictionary mapping modules to the tags they would satisfy."""
        dstmods = dict()
        for req in mod.getRequires():
            for dst in self.getProvides(req):
                if dst not in dstmods: dstmods[dst] = set()
                dstmods[dst].add(req)
        return dstmods

    def getConflictingModules(self, mod):
        """inefficient method to find all modules that are in conflict with the given one.
        Returns a dictionary mapping modules to the conflicting tags."""
        dstmods = dict()
        for prov in mod.getProvides():
            for dst in self.getProvides(prov):
                if dst.name == mod.name: continue
                if dst not in dstmods: dstmods[dst] = set()
                dstmods[dst].add(prov)
        return dstmods

    def checkConsistency(self):
        for mod in self.getModules():
            includes = mod.requiredFiles
            dups = mod.requires & includes # without required files!
            if dups:
                logging.warning('Module %s(%s) contains unnecessary requires: %s',
                                mod.name, mod.modulefile, str(dups))

        requires = set(self.requires.keys())
        provides = set(self.provides.keys())
        unsat = requires - provides
        if unsat != set():
            for require in unsat:
                mods = self.getRequires(require)
                names = [m.name+'('+m.modulefile+')' for m in mods]
                logging.info('Tag %s required by %s not provided by any module',
                             require, str(names))


def installFile_Softlink(file, srcdir, destdir):
    srcfile = os.path.abspath(srcdir + '/' + file)
    destfile = os.path.abspath(destdir + '/' + file)
    logging.debug('installing file %s to %s', srcfile, destfile)
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    if os.path.exists(destfile) or os.path.islink(destfile):
        os.unlink(destfile)
    os.symlink(os.path.relpath(srcfile, os.path.dirname(destfile)), destfile)

def installFile_Hardlink(file, srcdir, destdir):
    srcfile = os.path.abspath(srcdir + '/' + file)
    destfile = os.path.abspath(destdir + '/' + file)
    logging.debug('installing file %s to %s', srcfile, destfile)
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    if os.path.exists(destfile) or os.path.islink(destfile):
        os.unlink(destfile)
    os.link(srcfile, destfile)

def installFile_Incl(file, srcdir, destdir):
    srcfile = os.path.abspath(srcdir + '/' + file)
    destfile = os.path.abspath(destdir + '/' + file)
    logging.debug('installing file %s to %s', srcfile, destfile)
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    if os.path.exists(destfile) or os.path.islink(destfile):
        os.unlink(destfile)
    with open(destfile, 'w') as f:
        f.write('#include "'+os.path.relpath(srcfile, os.path.dirname(destfile))+'"\n')

def installFile_Copy(file, srcdir, destdir):
    srcfile = os.path.abspath(srcdir + '/' + file)
    destfile = os.path.abspath(destdir + '/' + file)
    logging.debug('installing file %s to %s', srcfile, destfile)
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    if os.path.exists(destfile) or os.path.islink(destfile):
        os.unlink(destfile)
    shutil.copy2(srcfile, destfile)

def installFile(file, srcdir, destdir):
    return installFile_Softlink(file, srcdir, destdir)

def replaceSuffix(str, osuffix, nsuffix):
    return str[:-len(osuffix)] + nsuffix

class Configuration:
    def __init__(self):
        self.moduledirs = list()
        self.provides = set()
        self.requires = set()
        self.modules = set()
        self.copyfiles = set()
        self.destdir = '.'

        self.acceptedMods = set() # set of selected module objects
        self.files = dict()
        self.allfiles = dict()
        self.vars = dict()
        self.mods = None

    def setModuleDB(self, mods):
        self.mods = mods

    def applyModules(self, pendingMods):
        '''add all modules of the list to the configuration, including referenced modules'''
        pendingMods = pendingMods.copy()
        while len(pendingMods) > 0:
            modname = pendingMods.pop()
            # 1) error if module not available
            if not self.mods.has(modname):
                raise Exception("Didn't find module " + modname)
            # 2) ignore if module already selected
            mod = self.mods[modname]
            if mod in self.acceptedMods: continue
            # 3) error if conflict with previously selected module
            # conflict if one of the provides is already provided
            conflicts = self.provides & mod.getProvides()
            if conflicts:
                for tag in conflicts:
                    conflictMods = self.mods.getProvides(tag) & self.acceptedMods
                    cnames = [m.name for m in conflictMods]
                    logging.warning("requested module %s tag %s conflicts with %s",
                                    mod.name, tag, str(cnames))
                    raise Exception("requested module " + modname +
                                    " conflicts with previously selected modules")
            # conflictMods = self.mods.getConflictingModules(mod)
            # conflicts = conflictMods.keys() & self.acceptedMods
            # if conflicts:
            #     # TODO add more diagnostigs: which tags/files do conflict?
            #     cnames = [m.name for m in (conflicts|set(mod))]
            #     logging.warning("selected conflicting modules: "+cnames)

            logging.debug('selecting module %s', mod.name)
            self.acceptedMods.add(mod)
            pendingMods |= mod.modules
            self.requires |= mod.getRequires()
            self.provides |= mod.getProvides()
            self.copyfiles |= mod.copyfiles
            for var in mod.files:
                for f in mod.files[var]:
                    # if f in self.allfiles:
                    #     logging.warning('duplicate file %s from module %s and %s',
                    #                     f, mod.name, self.allfiles[f].name)
                    if var not in self.files: self.files[var] = dict()
                    self.files[var][f] = mod
                    self.allfiles[f] = mod

    def getMissingRequires(self):
        return self.requires - self.provides

    def processModules(self, resolveDeps):
        '''if resolveDeps is true, this method tries to resolve missing dependencies
        by including additional modules from the module DB'''
        self.applyModules(self.modules)

        if resolveDeps:
            additionalMods = self.resolveDependencies()
            names = ", ".join(sorted([m.name for m in additionalMods]))
            logging.info('added modules to resolve dependencies: %s', names)

        missingRequires = self.getMissingRequires()
        for tag in missingRequires:
            reqMods = self.mods.getRequires(tag) & self.acceptedMods
            req = [m.name for m in reqMods]
            prov = [m.name for m in self.mods.getProvides(tag)]
            logging.warning('unresolved dependency %s required by [%s] provided by [%s]',
                            tag, ', '.join(req), ', '.join(prov))

    def resolveDependencies(self):
        additionalMods = set()
        missingRequires = self.getMissingRequires()
        count = 1
        while count:
            count = 0
            for tag in missingRequires:
                solutions = self.mods.getResolvableProvides(tag, self.provides).copy()
                # 1) ignore if none or multiple solutions
                if len(solutions) != 1: continue
                mod = solutions.pop()
                # 2) ignore if conflict with already selected modules
                conflicts = self.provides & mod.getProvides()
                if conflicts: continue
                # select the module
                logging.debug('Satisfy dependency %s with module %s', tag, mod.name)
                additionalMods.add(mod)
                count += 1
                self.applyModules(set([mod.name]))
                missingRequires = self.getMissingRequires()
        # return set of additionally selected modules
        return additionalMods

    def checkConsistency(self):
        selected = set([self.mods[n] for n in self.modules])
        removable = set()
        for mod in selected:
            # 1) modules with conflicts should be selected
            conflictMods = self.mods.getConflictingModules(mod)
            if conflictMods: continue
            # 2) modules that do not satisfy any dependency should be selected
            providesAnything = False
            for tag in mod.getProvides():
                if self.mods.getRequires(tag) & selected:
                    providesAnything = True
            if not providesAnything: continue
            # remember all other modules
            removable.add(mod)
        if removable:
            logging.info("following modules could be resolved automatically: %s",
                         str(removable))

    def buildFileStructure(self):
        if not os.path.exists(self.destdir):
            os.makedirs(self.destdir)
        self.destdir += '/'
        for file in self.allfiles:
            srcpath = os.path.dirname(self.allfiles[file].modulefile)
            srcfile = srcpath + '/' + file
            if not os.path.isfile(srcfile):
                logging.warning('file %s is missing or not regular file, provided by %s from %s',
                                srcfile,
                                self.allfiles[file].name,
                                self.allfiles[file].modulefile)
            if file in self.copyfiles:
                installFile_Hardlink(file, srcpath, self.destdir)
            else:
                installFile(file, srcpath, self.destdir)

    def generateMakefile(self):
        with open(self.destdir + '/Makefile', 'w') as makefile:
            for var in self.files:
                makefile.write(var + ' = ' + ' '.join(sorted(self.files[var].keys())) + '\n')
                makefile.write(var + '_OBJ = $(addsuffix .o, $(basename $('+var+')))\n')
                makefile.write('DEP += $(addsuffix .d, $(basename $('+var+')))\n')
            makefile.write("\n")

            makefile.write("DEPFLAGS += -MP -MMD -pipe\n")
            # makefile.write("DEP := $(addsuffix .d, $(basename")
            # for var in self.files: makefile.write(" $("+var+")")
            # makefile.write("))\n\n")

            makefile.write(".PHONY: all clean cleanall\n\n")

            for mod in self.acceptedMods:
                if mod.makefile_head != None:
                    tmpl = Template(mod.makefile_head)
                    makefile.write(tmpl.render(**config.vars))
                    makefile.write("\n")
            makefile.write("\n")

            makefile.write("all: $(TARGETS)\n\n")

            for mod in self.acceptedMods:
                if mod.makefile_body != None:
                    tmpl = Template(mod.makefile_body)
                    makefile.write(tmpl.render(**config.vars))
                    makefile.write("\n")

            for var in self.files:
                vprefix = replaceSuffix(var, "FILES", "_")
                for f in sorted(self.files[var].keys()):
                    if f.endswith(".cc"):
                        makefile.write(replaceSuffix(f,".cc",".o")+": "+f+"\n")
                        makefile.write("\t$("+vprefix+"CXX) $("+vprefix+"CXXFLAGS) $("+vprefix+"CPPFLAGS) $(DEPFLAGS) -c -o $@ $<\n")
                    if f.endswith(".S"):
                        makefile.write(replaceSuffix(f,".S",".o")+": "+f+"\n")
                        makefile.write("\t$("+vprefix+"AS) $("+vprefix+"ASFLAGS) $("+vprefix+"CPPFLAGS) $(DEPFLAGS) -c -o $@ $<\n")
            makefile.write("\n")


            makefile.write("clean:\n")
            for var in self.files:
                makefile.write("\t- $(RM) $("+var+"_OBJ)\n")
            makefile.write("\t- $(RM) $(TARGETS) $(EXTRATARGETS)\n\n")

            makefile.write("cleanall: clean\n\t- $(RM) $(DEP)\n\n")
            makefile.write("ifneq ($(MAKECMDGOALS),clean)\n")
            makefile.write("ifneq ($(MAKECMDGOALS),cleanall)\n")
            makefile.write("-include $(DEP)\n")
            makefile.write("endif\nendif\n\n")

def parseTomlConfiguration(conffile):
    with open(conffile, 'r') as fin:
        configf = toml.load(fin)
        configf = configf['config']
        config = Configuration()
        config.plain = configf
        for field in configf:
            if field == 'vars': config.vars = configf[field]
            elif field == 'moduledirs': config.moduledirs = list(configf['moduledirs'])
            elif field == 'requires': config.requires = set(configf['requires'])
            elif field == 'provides': config.provides = set(configf['provides'])
            elif field == 'modules': config.modules = set(configf['modules'])
            elif field == 'destdir': config.destdir = os.path.abspath(configf['destdir'])
        return config



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', "--configfile", default = 'project.config')
    parser.add_argument('-d', "--destpath")
    parser.add_argument("--check", action = 'store_true')
    parser.add_argument('-v', "--verbose", action = 'store_true')
    parser.add_argument('-g', "--modulegraph", action = 'store_true')
    parser.add_argument("--find")
    parser.add_argument("--depsolve", help = 'Tries to resolve unsatisfied requirements by adding modules from the search path.', action = 'store_true')
    args = parser.parse_args()

    if args.destpath is not None:
        args.destpath = os.path.abspath(args.destpath)

    # configure the logging
    logFormatter = logging.Formatter("%(message)s")
    rootLogger = logging.getLogger()
    fileHandler = logging.FileHandler(args.configfile+'.log')
    fileHandler.setFormatter(logFormatter)
    fileHandler.setLevel(logging.DEBUG)
    rootLogger.addHandler(fileHandler)

    consoleHandler = logging.StreamHandler(sys.stdout)
    consoleHandler.setFormatter(logFormatter)
    if args.verbose:
        consoleHandler.setLevel(logging.DEBUG)
    else:
        consoleHandler.setLevel(logging.INFO)
    rootLogger.addHandler(consoleHandler)
    rootLogger.setLevel(logging.DEBUG)

    currentDir = os.getcwd()
    conffile = os.path.abspath(args.configfile)
    os.chdir(os.path.dirname(conffile))
    logging.info("processing configuration %s", args.configfile)
    config = parseTomlConfiguration(conffile)

    if args.destpath is not None:
        config.destdir = args.destpath

    mods = ModuleDB()
    mods.loadModulesFromPaths(config.moduledirs)

    if(args.check):
        mods.checkConsistency()
        config.setModuleDB(mods)
        config.checkConsistency()
    elif(args.modulegraph):
        createModulesGraph(mods)
    else:
        config.setModuleDB(mods)
        config.processModules(args.depsolve)
        config.buildFileStructure()
        createConfigurationGraph(config.acceptedMods, config.modules, mods, config.destdir+'/config.dot')
        config.generateMakefile()

    sys.exit(0)
