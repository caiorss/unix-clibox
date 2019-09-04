#include <iostream>
#include <string>
#include <filesystem>
#include <bitset>
#include <cstring> // strtok

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;


class DirectoryNavigator
{
    bool m_directory_only = false;
    bool m_fullpath       = false;
    bool m_file_only      = false;
    bool m_recursive      = false;
    bool m_lastmodified   = false;
    bool m_permission     = false;
public:
    DirectoryNavigator(){}
    void directory_only(bool flag) {  m_directory_only = flag;  }
    void file_only(bool flag)      {  m_file_only = flag;       }
    void fullpath(bool flag)       {  m_fullpath = flag;        }
    void recursive(bool flag)      {  m_recursive = flag;       }
    void lastmodified(bool flag)   {  m_lastmodified = flag;    }
    void permission(bool flag)     {  m_permission = flag; }

    void listdir(std::string path)
    {
        auto& self = *this;

        #if 0
        std::cout << std::boolalpha;
        std::cout << " directory-only = " << m_directory_only << "\n";
        std::cout << " file_only      = " << m_file_only << "\n";
        std::cout << " fullpath       = " << m_fullpath << "\n";
        #endif

        using pred_fun_ptr = bool (*) (fs::path const&);
        using pred_fun = std::function<bool (fs::path const&)>;
        using action_fun = std::function<void (fs::path const&)>;

        // Predicate function
        pred_fun   predicate;
        action_fun action;

        if(self.m_directory_only)
            predicate = static_cast<pred_fun_ptr>(&fs::is_directory);

        if(self.m_file_only)
            predicate = static_cast<pred_fun_ptr>(&fs::is_regular_file);

        if(!self.m_directory_only && !self.m_file_only)
            predicate = [](fs::path const& ) -> bool { return true;  };

        action = [this](fs::path const& p){
            if(m_permission)
            {
                auto pm = fs::status(p).permissions();

                std::cout << ((pm & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
                          << ((pm & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
                          << ((pm & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
                          << ((pm & fs::perms::group_read) != fs::perms::none ? "r" : "-")
                          << ((pm & fs::perms::group_write) != fs::perms::none ? "w" : "-")
                          << ((pm & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
                          << ((pm & fs::perms::others_read) != fs::perms::none ? "r" : "-")
                          << ((pm & fs::perms::others_write) != fs::perms::none ? "w" : "-")
                          << ((pm & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
                          << "  ";
            }

            if(m_lastmodified)
            {
                auto ftime = fs::last_write_time(p);
                auto ctime = decltype(ftime)::clock::to_time_t(ftime);
                auto stime = strtok(std::asctime(std::localtime(&ctime)), "\n");

                std::cout << std::left  << std::setw(25) << stime
                          << " ";
            }

            if(!m_fullpath)
            {
                std::cout  << std::left << std::setw(30) << p.filename().string()
                           << std::endl;
            } else
                std::cout << p.string() << std::endl;
        };

        if(!m_recursive)
            self.iterate_dirlist(path, predicate, action);
        else
            self.iterate_recursive_dirlist(path, predicate, action);

    } //--- End of function listdir() ---- //

private:

    template<typename Predicate, typename Action>
    void iterate_dirlist(std::string path, Predicate&& pred, Action&& act)
    {
        for(auto& p: fs::directory_iterator(path))
            if(pred(p)) { act(p);  }
    }

    template<typename Predicate, typename Action>
    void iterate_recursive_dirlist(std::string path, Predicate&& pred, Action&& act)
    {
        for(auto& p: fs::recursive_directory_iterator(path))
            if(pred(p)) { act(p);  }
    }


};

int main(int argc, char** argv)
{
    CLI::App app{ "listdir"};
    //app.footer("\n Creator: Somebody else.");

    // Sets directory that will be listed
    std::string dirpath = ".";
    app.add_option("directory", dirpath, "Directory to be listed")->required();

    // List only directory
    int flag_list_dir = 0;
    app.add_flag("-d,--dir", flag_list_dir, "List directory only");

    // List only regular files
    int flag_list_file = 0;
    app.add_flag("-f,--file", flag_list_file, "List only regular files");

    // If true, show full path to file
    int fullpath = 0;
    app.add_flag("-p,--fullpath", fullpath, "Show full path.");

    int lastmodified = 0;
    app.add_flag("-t,--time", lastmodified, "Show last modified time.");

    int permission = 0;
    app.add_flag("--perm", permission, "Show file/directory permission.");

    int recursive = 0;
    app.add_flag("-r,--recursive", recursive, "List directory in a recursive way.");

    // ----- Parse Arguments ---------//
    try {
        app.validate_positionals();
        CLI11_PARSE(app, argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    //------ Program Actions ---------//

    DirectoryNavigator dnav;
    dnav.directory_only(flag_list_dir);
    dnav.file_only(flag_list_file);
    dnav.fullpath(fullpath);
    dnav.lastmodified(lastmodified);
    dnav.recursive(recursive);
    dnav.permission(permission);

    try {
        dnav.listdir(dirpath);
    } catch (fs::filesystem_error& ex) {
        std::cerr << " [ERROR] " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
