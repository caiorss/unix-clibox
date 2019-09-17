#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <cassert>
#include <filesystem>

//---- Library Headers -----------------//
#include <CLI/CLI.hpp>

namespace fs = std::filesystem;

template<typename Predicate, typename Action>
void iterate_dirlist(std::string path, Predicate&& pred, Action&& act)
{
    for(auto& p: fs::directory_iterator(path))
        if(pred(p)) {
            try { act(p); }
            catch(fs::filesystem_error& ex)
            {
                std::cerr << ex.what() << "\n";
            }
        }
}

template<typename Predicate, typename Action>
void iterate_recursive_dirlist(std::string path, Predicate&& pred, Action&& act)
{
    for(auto& p: fs::recursive_directory_iterator(path))
        if(pred(p)) {
            try { act(p); }
            catch(fs::filesystem_error& ex)
            {
                std::cerr << ex.what() << "\n";
            }
        }
}

std::string
replace_string( const std::string& text,
                const std::string& rep,
                const std::string& subst
              )
{
  // Copy paraemter text (Invoke copy constructor)
  std::string out = text;
  // Find position of character matching the string
  size_t i = out.find(rep);
  while(i != std::string::npos){
    out.replace(i, rep.size(), subst);
    i = out.find(rep, i);
  }
  return out;
}

std::string
repladce_string_list(
      std::string const& text
    , std::vector<std::tuple<std::string, std::string>> const& list
    )
{
    auto str = text;
    for(auto const& [rep, sub]: list){
        str = replace_string(str, rep, sub);
    }
    return str;
}

void rename_files_fix(std::string path, bool commit, bool silent, bool recursive)
{
    auto predicate = [](fs::path const& p)
    {
        return fs::is_regular_file(p);
    };

    auto action = [=](fs::path const& p)
    {
        auto txt = repladce_string_list( p.filename().string()
                                            , {   {" ",   "_"}
                                         , {",",   "-"}
                                         , {"&",   "-"}
                                         , {"--",  "-"}
                                         , {"---", "-"}
                                         , {"(",   "" }
                                         , {")",   "" }
                                         , {"[",   "" }
                                         , {"]",   "" }
                                         , {".....",  "_" }
                                         , {"....",  "_" }
                                         , {"..",  "_" }
                                         , {"...",  "_" }
                                        });

        auto path2 = p.parent_path() / txt;

        if(!silent){
            std::cout << p.filename().string()
                      << " =>> " << path2.filename().string() << "\n\n";
        }

        if(!commit){ return; }

        fs::rename(p, path2);
    };

    if(!recursive)
        iterate_dirlist(path, predicate, action);
    else
        iterate_recursive_dirlist(path, predicate, action);

}

int main(int argc, char** argv)
{
    CLI::App app("rename files and fix file names");
    app.footer("Tool for renaming and fixing file names");

    std::string path;
    app.add_option("<DIRECTORY>", path)->required();

    bool flag_recursive = false;
    app.add_flag("--recursive", flag_recursive);

    bool flag_commit = false;
    app.add_flag("--commit", flag_commit);

    bool flag_silent = false;
    app.add_flag("--silent", flag_silent);

    // ----------- Parse Arguments ---------------//

    // app.require_subcommand();

    try
    {
        app.validate_positionals();
        app.parse(argc, argv);
    } catch(const CLI::ParseError &e)
    {
        return (app).exit(e);
    } catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    // ---- Program Actions ------------------//

    rename_files_fix(path, flag_commit, flag_silent, flag_recursive);

    return 0;
}
