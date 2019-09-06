/** Process launcher for Any UNIX-like OS or Linux
 *
 *------------------------------------------------------*/

#include <iostream>
#include <string>
#include <optional>
#include <cstring> // strtok
#include <vector>
#include <cstdlib>
#include <cassert>

//---- Library Headers -----------------//
#include <CLI/CLI.hpp>

//---- Linux/POSIX specific Headers ---//
#include <unistd.h>


bool is_tty_terminal()
{
    #if __unix__
    return ::ttyname(STDIN_FILENO) != nullptr;
    #else
       return false;
    #endif
}

/** @brief Launch executable as daemon process.
  *
  * @details It launches some application as a daemon process, without killing current
  *          process or littering the current terminal with stdout and stderr output
  *          of the launched process.
  *
  * @param program Program to be launched
  * @param args    Arguments of the program to be launched
  * @param cwd     Current working directory of the program to be launched
  *
  **********************************************************************************/
std::optional<int>
launch_as_daemon( std::string              const& program
                , std::vector<std::string> const& args
                , std::string              const& cwd = ".")
{
    int pid = ::fork();

    // If the PID of the forked process is negative
    // when the fork operation has fails.
    if(pid < 0) {
        std::cerr << " [ERROR] Unable to fork proces and launch " << program << "\n";
        return std::nullopt;
    }

    // If the PID is greater than zero, the current process
    // is the original or the parent one. Then the PID is returned
    // to the caller.
    if(pid > 0) { return pid; }

    // ---- Forked process (pid == 0) --- //
    //------------------------------------//
    assert(pid == 0 && "PID should be zero");
    ::setsid();
    ::umask(0);

    // Set current directory of daemon process
    ::chdir(cwd.c_str());

    // Close stdin, stdout and stderr file descriptors to avoid
    // littering the parent process output.
    ::close(STDIN_FILENO);
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);

    std::vector<const char*> pargs(args.size() + 1);
    pargs[0] = program.c_str();
    std::transform(args.begin(), args.end(), pargs.begin() + 1
                   , [](auto const& s){ return s.c_str(); });
    pargs.push_back(nullptr);

    // Dummy return
    return ::execvp(program.c_str(), (char* const*) &pargs[0]);
}

void launch_app_terminal(std::string const& application, std::string const& cwd = ".")
{
    launch_as_daemon("xterm", {"-hold", "-e", application }, cwd);
}

/** Print directories listed in PATH environment variable.
 *  Operating Systems.
 *  Requires: <iostream>, <cstdlib> <sstream>
 **********************************************************/
void show_dirs_in_path(std::ostream& os)
{
    std::stringstream ss{ ::getenv("PATH") };
    std::string dirpath;

    while(std::getline(ss, dirpath, ':'))
        std::cout << "\t" << dirpath << std::endl;
}


int main(int argc, char** argv)
{

    CLI::App app{ "launcher"};
    app.footer("\n Command line utility for launching applications.");

    CLI::App* cmd_run  = app.add_subcommand(
        "run",
        "Run some application"
        );

    CLI::App* cmd_path = app.add_subcommand(
         "path"
        ,"Show content of $PATH environment variable"
        );
#if 1
    // Sets directory that will be listed
    std::string application = ".";
    cmd_run->add_option("<APPLICATION>", application
                   , "Application to be launched as daemon")->required();

    bool flag_terminal = false;
    cmd_run->add_flag("-t,--terminal", flag_terminal
                      , "Launch application in terminal");

    std::string cwd = ".";
    cmd_run->add_option("-d,--dir", cwd
                        , "Current directory of launched process");

#endif


    app.require_subcommand();
    // ----- Parse Arguments ---------//
    try {
        app.validate_positionals();
        CLI11_PARSE(app, argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    //------ Program Actions ---------//

    // Check if subcommand processed
    if(*cmd_run){
        if(!flag_terminal)
        {
            auto pid = launch_as_daemon(application, {}, cwd);
            if(pid) {
                std::cout << " [INFO] Forked process launched successfully.\n"
                          << " [INFO] Process pid = " << pid.value() << "\n";
            }
        } else {
            launch_app_terminal(application, cwd);
        }
        return EXIT_SUCCESS;
    }

    if(*cmd_path)
    {
        show_dirs_in_path(std::cout);
        return  EXIT_SUCCESS;
    }


    return EXIT_SUCCESS;
} // * ------ End of main() Function -------------- * //




