/* 
 * Copyright Evan Spensley
 * A custom shell that uses fork() and execvp() for running commands
 * in serial or in parallel.
 * 
 */

#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include "ChildProcess.h"

// A vector of strings to ease running programs with command-line
// arguments.
using StrVec = std::vector<std::string>;

// A vector of integers to hold child process ID's when operating in
// parallel mode.
using IntVec = std::vector<int>;

void processCmds(std::istream& is, std::ostream& os, bool parMode,
                 const std::string prompt);

/** Convenience method to check if a line should be ignored.

    \param[in] String vector to check.

    \return bool if passed or failed. 
 */
bool checkIgnore(StrVec& line) {
    // check if line is empty or if first character starts with #
    return (line.empty() || line[0][0] == '#');
}

/** Convenience method to split a given line into individual words.

    \param[in] line The line to be split into individual words.

    \return A vector strings containing the list of words. 
 */
StrVec split(const std::string& line) {
    StrVec words;  // The list to be created
    // Use a string stream to read individual words 
    std::istringstream is(line);
    std::string word;
    while (is >> std::quoted(word)) {
        words.push_back(word);
    }
    return words;
}

/** Method to run serial commands

    \param[in] is, os, command
 */
void runSerialCommand(std::istream& is, std::ostream& os, StrVec& command) {
    // print command
    os << "Running: ";
    for (std::string word : command) {
        os << word;
        if (word != command.back())
            os << " ";
    }
    os << std::endl; 
    
    // run command in childProcess
    ChildProcess cp; 
    cp.forkNexec(command);
    
    // wait for process
    int exitCode = cp.wait();
    os << "Exit code: " << exitCode << std::endl;
}


/** Method to run parallel commands

    \param[in] is, os, command, parPids
 */
void runParallelCommand(std::istream& is, std::ostream& os, StrVec& command,
        IntVec& parPids) {
    // print command
    os << "Running: ";
    for (std::string word : command) {
        os << word;
        if (word != command.back())
            os << " ";
    }
    os << std::endl; 
    
    // run command in childProcess
    ChildProcess cp; 
    parPids.push_back(cp.forkNexec(command));
}

/** Method to wait for parallel commands

    \param[in] is, parPids
 */
void waitForParalellCommands(std::ostream& os, IntVec& parPids) {
    for (int pid : parPids) {
        int exitCode; 
        waitpid(pid, &exitCode, 0);
        os << "Exit code: " << exitCode << std::endl;  
    }
}

/** Method to check the first word of input

    \param[in] line the input line to check

    \return bool, true if we have a command to execute, false if we want to
            exit the shell 
 */
bool processFirstWord(StrVec& line) {
    if (line[0] == "exit") { return false;
    } else if (line[0] == "SERIAL") {
        std::ifstream in(line[1]);
        processCmds(in, std::cout, false, "");
        // exit at end-of-file
        return false;
    } else if (line[0] == "PARALLEL") {
        std::ifstream in(line[1]);
        processCmds(in, std::cout, true, "");
        // exit at end-of-file
        return false;
    }
    return true;
}    

/** Method to process commands

    \param[in] is, os, parMode, prompt
 */
void processCmds(std::istream& is, std::ostream& os, bool parMode,
                 const std::string prompt) {
    IntVec parPids;
    std::string line;
    while (os << prompt, std::getline(is, line)) {
        StrVec splitLine = split(line); 
        
        if (checkIgnore(splitLine)) {
            continue;
        } else if (!processFirstWord(splitLine) || is.eof()) {
            break;
        } else {
            if (!parMode)
                runSerialCommand(is, os, splitLine);
            else
                runParallelCommand(is, os, splitLine, parPids);
        }
    }
    if (parMode) {
        waitForParalellCommands(std::cout, parPids);
    }
} 

// main method
int main() {
    processCmds(std::cin, std::cout, false, "> ");
}

