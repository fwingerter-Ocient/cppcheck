/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2009 Daniel Marjamäki and Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Generate Makefile for cppcheck

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "../lib/filelister.h"

std::string objfile(std::string cppfile)
{
    cppfile.erase(cppfile.rfind("."));
    return cppfile + ".o";
}

void getDeps(const std::string &filename, std::vector<std::string> &depfiles)
{
    // Is the dependency already included?
    if (std::find(depfiles.begin(), depfiles.end(), filename) != depfiles.end())
        return;

    std::ifstream f(filename.c_str());
    if (! f.is_open())
    {
        if (filename.compare(0, 4, "cli/") == 0 || filename.compare(0, 5, "test/") == 0)
            getDeps("lib" + filename.substr(filename.find("/")), depfiles);
        return;
    }
    if (filename.find(".c") == std::string::npos)
        depfiles.push_back(filename);

    std::string path(filename);
    if (path.find("/") != std::string::npos)
        path.erase(1 + path.rfind("/"));

    std::string line;
    while (std::getline(f, line))
    {
        std::string::size_type pos1 = line.find("#include \"");
        if (pos1 == std::string::npos)
            continue;
        pos1 += 10;

        std::string::size_type pos2 = line.find("\"", pos1);
        std::string hfile(path + line.substr(pos1, pos2 - pos1));
        if (hfile.find("/../") != std::string::npos)	// TODO: Ugly fix
            hfile.erase(0, 4 + hfile.find("/../"));
        getDeps(hfile, depfiles);
    }
}

static void compilefiles(std::ostream &fout, const std::vector<std::string> &files)
{
    for (unsigned int i = 0; i < files.size(); ++i)
    {
        fout << objfile(files[i]) << ": " << files[i];
        std::vector<std::string> depfiles;
        getDeps(files[i], depfiles);
        for (unsigned int dep = 0; dep < depfiles.size(); ++dep)
            fout << " " << depfiles[dep];
        fout << "\n\t$(CXX) $(CXXFLAGS) -Ilib -c -o " << objfile(files[i]) << " " << files[i] << "\n\n";
    }
}

int main()
{
    // Get files..
    std::vector<std::string> libfiles;
    FileLister::recursiveAddFiles(libfiles, "lib/", true);

    std::vector<std::string> clifiles;
    FileLister::recursiveAddFiles(clifiles, "cli/", true);

    std::vector<std::string> testfiles;
    FileLister::recursiveAddFiles(testfiles, "test/", true);

    std::ofstream fout("Makefile");

    // more warnings.. -Wfloat-equal -Wcast-qual -Wsign-conversion -Wlogical-op
    fout << "CXXFLAGS=-Wall -Wextra -pedantic -g\n";
    fout << "CXX=g++\n";
    fout << "BIN=${DESTDIR}/usr/bin\n\n";

    fout << "\n###### Object Files\n\n";
    fout << "LIBOBJ =     " << objfile(libfiles[0]);
    for (unsigned int i = 1; i < libfiles.size(); ++i)
        fout << " \\" << std::endl << std::string(14, ' ') << objfile(libfiles[i]);
    fout << "\n\n";
    fout << "CLIOBJ =     " << objfile(clifiles[0]);
    for (unsigned int i = 1; i < clifiles.size(); ++i)
        fout << " \\" << std::endl << std::string(14, ' ') << objfile(clifiles[i]);
    fout << "\n\n";
    fout << "TESTOBJ =     " << objfile(testfiles[0]);
    for (unsigned int i = 1; i < testfiles.size(); ++i)
        fout << " \\" << std::endl << std::string(14, ' ') << objfile(testfiles[i]);
    fout << "\n\n";


    fout << "\n###### Targets\n\n";
    fout << "cppcheck:\t$(LIBOBJ)\t$(CLIOBJ)\n";
    fout << "\t$(CXX) $(CXXFLAGS) -o cppcheck $(CLIOBJ) $(LIBOBJ) $(LDFLAGS)\n\n";
    fout << "all:\tcppcheck\ttestrunner\ttools\n\n";
    fout << "testrunner:\t$(TESTOBJ)\t$(LIBOBJ)\n";
    fout << "\t$(CXX) $(CXXFLAGS) -o testrunner $(TESTOBJ) $(LIBOBJ) $(LDFLAGS)\n\n";
    fout << "test:\tall\n";
    fout << "\t./testrunner\n\n";
    fout << "tools:\ttools/dmake\n\n";
    fout << "tools/dmake:\ttools/dmake.cpp\tlib/filelister.cpp\tlib/filelister.h\n";
    fout << "\t$(CXX) $(CXXFLAGS) -o tools/dmake tools/dmake.cpp lib/filelister.cpp $(LDFLAGS)\n\n";
    fout << "clean:\n";
    fout << "\trm -f lib/*.o cli/*.o test/*.o testrunner cppcheck tools/dmake\n\n";
    fout << "install:\tcppcheck\n";
    fout << "\tinstall -d ${BIN}\n";
    fout << "\tinstall cppcheck ${BIN}\n\n";

    fout << "\n###### Build\n\n";

    compilefiles(fout, libfiles);
    compilefiles(fout, clifiles);
    compilefiles(fout, testfiles);

    return 0;
}


