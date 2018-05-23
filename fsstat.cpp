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
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define ANSI_GREEN "\033[32m"
#define ANSI_GRAY  "\033[90m"
#define ANSI_RESET "\033[0m"


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
    auto filter = [](const std::string& s){ return s != ""; };
    auto it = std::copy_if(v1.begin(), v1.end(), std::back_inserter(v2), filter);
    return v2;
}


void
print_fsrow(const std::string& col)
{
    // split info into columns
    std::vector<std::string> cols = split_ws(col);
    // print filesystem
    std::cout << "  " << std::setw(14) << std::left << cols[0];
    
    // convert values (all are in sizes of 512-byte blocks)
    long long size, use, avail;
    size  = std::stoll(cols[1]) * 512;
    use   = std::stoll(cols[2]) * 512;
    avail = std::stoll(cols[3]) * 512;

    // convert disk usage to readable units
    for (const auto& unit : { size, use, avail })
    {
        double unitf;
        std::ostringstream unitstr;
        if (unit >= 1.0e9) {
            unitf = unit / 1.0e9;
            unitstr << std::setprecision(4) << unitf << "GB";
        } else if (unit >= 1.0e6) {
            unitf = unit / 1.0e6;
            unitstr << std::setprecision(4) << unitf << "MB";
        } else {
            unitf = unit / 1.0e3;
            unitstr << std::setprecision(4) << unitf << "kB";
        }
        std::cout << std::setw(10) << std::right << unitstr.str();
    }

    // display disk usage percentage (cutoff at 1.0e-3 precision)
    std::ostringstream diskstr;
    double disk = ((1.0 * use) / size) * 100;
    if (disk < 1.0e-3) disk = 0;
    diskstr << std::setprecision(3) << disk << "%";
    std::cout << std::setw(10) << std::right << diskstr.str();

    // display mount point
    std::cout << std::setw(9) << std::right << cols[8] << std::endl;

    // print filesystem progress bar
    int prog = 60 * (disk / 100);
    std::cout << "  [" << ANSI_GREEN;
    for (int i = 1; i <= prog; i++) std::cout << "=";
    std::cout << ANSI_GRAY;
    for (int i = prog; i <= 60; i++) std::cout << "=";
    std::cout << ANSI_RESET << "]\n";
}


void
print_fsinfo(const std::string& dfout)
{
    // split output into lines
    std::vector<std::string> lines;
    split(dfout, '\n', lines);

    // print first row
    std::cout << std::setw(16) << std::left << "Filesystem";
    for (const auto& x : { "Size", "Used", "Avail", "Used", "Mount\n" })
        std::cout << std::setw(10) << std::right << x;

    // print each filesystem row(s)
    std::for_each(lines.begin() + 1, lines.end(), print_fsrow);
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