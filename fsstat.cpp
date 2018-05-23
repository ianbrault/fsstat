/*
 * fsstat.cpp
 * program to query filesystem usage & report over stdout
 * 
 * author: Ian Brault <ianbrault@ucla.edu>
 * created: 22 May 2018
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>


void
error(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}


/*
 * split a string on a deliminator
 * @param s : input string
 * @param delim : character deliminator
 * @param v : output vector, passed by reference
 */
void
split(const std::string& s, const char delim, std::vector<std::string>& v)
{
    auto i = s.begin();
    auto pos = i;
    while (pos != s.end())
    {
        pos = std::find(i, s.end(), delim);
        v.emplace_back(i, pos);
        if (pos == s.end()) break;
        else i = ++pos;
    }
}


/*
 * split @param s on whitespace
 * @returns a vector of the string segments (cannot be passed by reference as
 * with split due to std::copy_if filtering)
 */
std::vector<std::string>
split_ws(const std::string& s)
{
    std::vector<std::string> v1, v2;
    split(s, ' ', v1);

    // filter empty elements
    auto filter = [](std::string s){ return s != ""; };
    auto it = std::copy_if(v1.begin(), v1.end(), v2.begin(), filter);
    v2.resize(std::distance(v2.begin(), it));
    return v2;
}


void
print_fsinfo(const std::string& dfout)
{
    // split output into lines
    std::vector<std::string> lines;
    split(dfout, '\n', lines);

    // print first column
    std::string fs = "Filesystem";
    std::cout << std::setw(fs.length() + 2) << std::left << fs;
    std::cout << std::setw(8) << std::right << "Size";
    std::cout << std::setw(8) << std::right << "Used";
    std::cout << std::setw(8) << std::right << "Avail";
    std::cout << std::setw(8) << std::right << "Use%";
    std::cout << std::setw(8) << std::right << "iUse%";
    std::cout << std::setw(8) << std::right << "Mount\n";

    std::vector<std::string> cols = split_ws(lines[1]);
    for (const auto& c : cols) std::cout << c << std::endl;
}


int 
main()
{
    // create pipe to capture df output
    int pipefd[2];
    if (pipe(pipefd) < 0) error("pipe");

    // fork new process
    pid_t pid = fork();
    if (pid < 0) error("fork");
    else if (pid) {
        // close unused write end, wait for child completion
        close(pipefd[1]);
        if (wait(NULL) < 0) error("wait");
        
        // read in df output
        char buf[1024];
        std::string output;
        while (read(pipefd[0], &buf, sizeof(buf)) > 0)
            output += std::string(buf);

        // print output
        print_fsinfo(output);
    } else {
        // closed unused read end, redirect stdout into write end
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) error("dup2");

        // spawn df process
        if (execlp("df", "-h", "/", NULL) < 0) error("execlp");
    }
}