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
  **********************************************************************************/
std::optional<int>
launch_as_daemon(std::string const& application
                , std::vector<std::string> const& arguments = {})
{
    int pid = ::fork();

    // If the PID of the forked process is negative
    // when the fork operation has fails.
    if(pid < 0) {
        std::cerr << " [ERROR] Unable to fork proces and launch " << application << "\n";
        return std::nullopt;
    }

    // If the PID is greater than zero, the current process
    // is the original or the parent one. Then the PID is returned
    // to the caller.
    if(pid > 0) { return pid; }

    // ---- Forked process (pid == 0) --- //
    assert(pid == 0 && "PID should be zero");
    ::setsid();
    ::umask(0);

    // Close stdin, stdout and stderr file descriptors to avoid
    // littering the parent process output.
    ::close(STDIN_FILENO);
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);

    std::vector<const char*> pargs(arguments.size() + 1);
    pargs[0] = application.c_str();
    std::transform(arguments.begin(), arguments.end(), pargs.begin() + 1
                   , [](auto const& s){ return s.c_str(); });
    pargs.push_back(nullptr);

    // Dummy return
    return ::execvp(application.c_str(), (char* const*) &pargs[0]);
}

void launch_app_terminal(std::string const& application)
{
    launch_as_daemon("xterm", {"-hold", "-e", application });
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
    //app.footer("\n Creator: Somebody else.");

    // Sets directory that will be listed
    std::string application = ".";
    app.add_option("<APPLICATION>", application
                   , "Application to be launched as daemon")->required();

    bool flag_terminal;
    app.add_flag("-t,--terminal", flag_terminal, "Launch application in terminal");

    // ----- Parse Arguments ---------//
    try {
        app.validate_positionals();
        CLI11_PARSE(app, argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    //------ Program Actions ---------//

    if(!flag_terminal)
    {
        auto pid = launch_as_daemon(application);
        if(pid) {
            std::cout << " [INFO] Forked process launched successfully.\n"
                      << " [INFO] Process pid = " << pid.value() << "\n";
        }
    } else {
        launch_app_terminal(application);
    }

#if 0
    auto pid = launch_as_daemon(application);
    if(pid) {
        std::cout << " [INFO] Forked process launched successfully.\n"
                  << " [INFO] Process pid = " << pid.value() << "\n";
    } else {
        std::cout << " [ERROR] Failed to launch process" << "\n";
    }
#endif

    return EXIT_SUCCESS;
} // * ------ End of main() Function -------------- * //




