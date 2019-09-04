#include <iostream>
#include <string>
#include <filesystem>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;


class DirectoryNavigator
{
    bool m_directory_only = false;
public:

    DirectoryNavigator()
    {
    }

    void directory_only(bool flag)
    {
        m_directory_only = flag;
    }

    void listdir(std::string path)
    {
        auto& self = *this;

        using pred_fun = std::function<bool (fs::path const&)>;

        pred_fun predicate;

        if(self.m_directory_only)
        {
            predicate = [](fs::path const& p) -> bool
            {
                return fs::is_directory(p);
            };
        }
        else
        {
            predicate = [](fs::path const& ) -> bool { return true;  };
        }

        self.iterate_dirlist(
            path
          , predicate // [&](fs::path const& p){ return true; }
          , [&](fs::path const& p){
                std::cout << p.filename().string() << std::endl;
            });
    }

private:

    template<typename Predicate, typename Action>
    void iterate_dirlist(std::string path, Predicate&& pred, Action&& act)
    {
        for(const auto& p: fs::directory_iterator(path))
            if(pred(p)) { act(p);  }
    }

};

void list_directory(std::string path, bool listDirOnly = false)
{
    for(const auto& p: fs::directory_iterator(path))
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
    dnav.listdir(dirpath);

    return EXIT_SUCCESS;
}
