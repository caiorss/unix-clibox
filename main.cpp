#include <iostream>
#include <string>
#include <filesystem>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;


class DirectoryNavigator
{
    bool m_directory_only = false;
    bool m_fullpath = false;
public:

    DirectoryNavigator()
    {
    }

    void directory_only(bool flag)
    {
        m_directory_only = flag;
    }

    void fullpath(bool flag)
    {
        m_fullpath = flag;
    }

    void listdir(std::string path)
    {
        auto& self = *this;

        using pred_fun = std::function<bool (fs::path const&)>;
        using action_fun = std::function<void (fs::path const&)>;

        // Predicate function
        pred_fun   predicate;
        action_fun action;

        if(self.m_directory_only)
            predicate = [](fs::path const& p) -> bool
            {
                return fs::is_directory(p);
            };

        if(!self.m_directory_only)
            predicate = [](fs::path const& ) -> bool { return true;  };

        action = [this](fs::path const& p){
            if(!m_fullpath)
                std::cout << p.filename().string() << std::endl;
            else
                std::cout << p.string() << std::endl;
        };

        self.iterate_dirlist(path, predicate, action);
    }

private:

    template<typename Predicate, typename Action>
    void iterate_dirlist(std::string path, Predicate&& pred, Action&& act)
    {
        for(auto& p: fs::directory_iterator(path))
            if(pred(p)) { act(p);  }
    }

};

void list_directory(std::string path, bool listDirOnly = false)
{
    for(auto& p: fs::directory_iterator(path))
    {
        if(!listDirOnly && p.is_directory())
            std::cout << p.path().filename().string() << std::endl;
    }
}

int main(int argc, char** argv)
{
    CLI::App app{ "ListDirectory"};
    //app.footer("\n Creator: Somebody else.");

    // Sets directory that will be listed
    std::string dirpath = ".";
    app.add_option("directory", dirpath, "Directory to be listed")->required();

    // List only directory
    int flag_list_dir = 0;
    app.add_flag("--dir", flag_list_dir, "List directory only");

    // If true, show full path to file
    int fullpath = 0;
    app.add_flag("--full-path", fullpath, "Show full path.");


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
    dnav.fullpath(fullpath);
    dnav.listdir(dirpath);

    return EXIT_SUCCESS;
}
