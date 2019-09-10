/** Process launcher for Any UNIX-like OS or Linux
 *
 *------------------------------------------------------*/

#include <iostream>
#include <string>
#include <optional>
#include <cstring> // strtok
#include <vector>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <cassert>

//---- Library Headers -----------------//
#include <CLI/CLI.hpp>

//---- Linux/POSIX specific Headers ---//
#include <linux/limits.h> // PATH_MAX

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

bool is_tty_terminal()
{
    #if __unix__
    return ::ttyname(STDIN_FILENO) != nullptr;
    #else
       return false;
    #endif
}


class AppLauncher
{
    std::string                m_program;
    std::string                m_cwd      = ".";
    std::optional<std::string> m_logfile  = std::nullopt;
    bool                       m_terminal = false;
    bool                       m_exec     = false;

public:

    AppLauncher(std::string program)
        : m_program(std::move(program))
    { }

    void set_cwd(std::string cwd)
    {
        this->m_cwd = std::move(cwd);
    }

    void set_logfile(std::string logfile)
    {
        this->m_logfile = std::move(logfile);
    }

    void set_terminal(bool flag)
    {
        this->m_terminal = flag;
    }

    void set_exec(bool flag)
    {
        this->m_exec = flag;
    }

    std::optional<int>
    launch(std::vector<std::string> const& args = {})
    {
        if(m_exec)
            return this->exec(m_program, args);

        if(!m_terminal)
            return this->launch_impl(m_program, args);
        else
            return this->launch_impl("xterm", {"-hold", "-e", m_program});
    }

private:

    int exec(std::string program, std::vector<std::string> const& args)
    {
        std::vector<const char*> pargs(args.size() + 1);
        pargs[0] = program.c_str();
        std::transform(args.begin(), args.end(), pargs.begin() + 1
                       , [](auto const& s){ return s.c_str(); });
        pargs.push_back(nullptr);

        // Dummy return
        return ::execvp(program.c_str(), (char* const*) &pargs[0]);
    }

    std::optional<int>
    launch_impl(std::string program, std::vector<std::string> const& args)
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
        ::chdir(m_cwd.c_str());

        // Close stdin, stdout and stderr file descriptors to avoid
        // littering the parent process output.
        ::close(STDIN_FILENO);
        ::close(STDOUT_FILENO);
        ::close(STDERR_FILENO);

        if(this->m_logfile)
        {
            FILE* fp = ::fopen(m_logfile.value().c_str(), "w");
            int fd = ::fileno(fp);
            //int fd = open(logfile.value().c_str(), O_WRONLY);
            if(fd == -1){
                std::cerr << " [ERROR] Failed to open file: " << m_logfile.value() << "\n";
            } else
            {
                ::dup2(fd, ::fileno(stdout));
                ::dup2(fd, ::fileno(stderr));
                ::dup2(fd, ::fileno(stdin));
            }
        }
        return this->exec(program, args);
    }
};


/** Print directories listed in PATH environment variable.
 *  Operating Systems.
 *  Requires: <iostream>, <cstdlib> <sstream>
 **********************************************************/
void show_dirs_in_path(std::ostream& os)
{
    std::stringstream ss{ ::getenv("PATH") };
    std::string dirpath;

    while(std::getline(ss, dirpath, ':'))
        os << "\t" << dirpath << std::endl;
}

std::optional<std::string>
get_symlink_realpath(std::string const& path)
{
    auto buffer = std::string(PATH_MAX, 0x00);
    char* result = ::realpath(path.c_str(), buffer.data());
    if(result){ return std::make_optional(buffer); }
    return std::nullopt;
}

void relaunch_app_pid(int pid)
{
    using namespace std::string_literals;

    // Executable path
    auto exe = get_symlink_realpath("/proc/"s + std::to_string(pid) + "/exe");
    // Current working directory
    auto cwd  = get_symlink_realpath("/proc/"s + std::to_string(pid) + "/cwd");

    if(!exe || !cwd){
        throw std::runtime_error("Error: process of pid: <"s
                                 + std::to_string(pid) + "> not found. ");
    }

    // Kill process
    ::kill(pid, 9);
    // Restart process
    AppLauncher app(exe.value());
    app.set_cwd(cwd.value());
    auto pid_new = app.launch();
    if(!pid_new){
        throw std::runtime_error("Error: failed to relaunch process");
    }
    std::cout << " [INFO] Relaunched application: "
              << "\n        pid = " << pid_new.value()
              << "\n executable = " << exe.value()
              << "\n  directory = " << cwd.value()
              << "\n";
}


   /*==================================================*
    *     MAIN()  Function                             *
    *==================================================*/

int main(int argc, char** argv)
{

    CLI::App app{ "launcher"};
    app.footer("\n Command line utility for launching applications.");

    //----- Run command settings -----------------//

    CLI::App* cmd_run  = app.add_subcommand(
         "run"
        ,"Run some application"
        );

    // Sets directory that will be listed
    std::string application = ".";
    cmd_run->add_option("<APPLICATION>", application
                        , "Application to be launched as daemon")->required();

    bool flag_exec = false;
    cmd_run->add_flag("-e,--exec", flag_exec
                      , "Run application in the current terminal.");

    bool flag_terminal = false;
    cmd_run->add_flag("-t,--terminal", flag_terminal
                      , "Launch application in terminal");

    std::string cwd = ".";
    cmd_run->add_option("-d,--directory", cwd
                        , "Current directory of launched process");

    std::string logfile = "";
    cmd_run->add_option("--logfile", logfile
                        , "Log file to which the process output will be redirected to.");


    //----- Path command settings -----------------//

    CLI::App* cmd_path = app.add_subcommand(
         "path"
        ,"Show content of $PATH environment variable"
        );

    //----- Relaunch command settings -----------------//

    CLI::App* cmd_relaunch = app.add_subcommand(
         "relaunch-pid"
        ,"Relaunch a process that got frozen given its PID"
        );
    int pid_to_relaunch;
    cmd_relaunch->add_option("<PID>", pid_to_relaunch
                             , "PID of application to be relaunched")->required();

    // ----------- Parse Arguments ---------------//
    app.require_subcommand();


    std::vector<std::string> arguments(argv, argv + argc);
    std::vector<const char*> app_args;
    std::vector<std::string> rest_args;

    auto it = std::find_if(arguments.begin(), arguments.end(),
                           [](std::string const& st)
                           {
                               return st == "--";
                           });

    // std::copy(arguments.begin(), it, std::back_inserter(app_args));
    std::copy(it + 1, arguments.end(), std::back_inserter(rest_args));
    std::transform(arguments.begin(), it, std::back_inserter(app_args),
                   [](std::string& str){ return str.data(); });

    try
    {
        app.validate_positionals();
        app.parse(app_args.size(), app_args.data());
    } catch(const CLI::ParseError &e)
    {
        return (app).exit(e);
    } catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    //------------ Program Actions --------------------//

    // Command: run => Launch a process
    if(*cmd_run){
        // auto pid = launch_as_daemon(application, {}, cwd);
        auto app = AppLauncher(application);
        app.set_cwd(cwd);
        app.set_terminal(flag_terminal);
        app.set_exec(flag_exec);
        if(logfile != "") app.set_logfile(logfile);
        auto pid = app.launch(rest_args);

        if(pid) {
            std::cout << " [INFO] Forked process launched successfully.\n"
                      << " [INFO] Process pid = " << pid.value() << "\n";
        }
        return EXIT_SUCCESS;
    }

    // Command: path show directories in PATH environment variable
    if(*cmd_path)
    {
        show_dirs_in_path(std::cout);
        return  EXIT_SUCCESS;
    }

    if(*cmd_relaunch)
    {
        try {
            relaunch_app_pid(pid_to_relaunch);
        } catch(std::runtime_error& ex)
        {
            std::cerr << " [ERROR] " << ex.what() << "\n";
        }
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
} // * ------ End of main() Function -------------- * //




