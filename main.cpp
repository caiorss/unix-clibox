#include <iostream>
#include <string>
#include <filesystem>

#include <CLI/CLI.hpp>


int main(int argc, char** argv)
{
    CLI::App app{ "ListDirectory"};
    //app.footer("\n Creator: Somebody else.");

    // Sets the current path that will be served by the http server
    std::string dir = "default";
    app.add_option("directory", dir, "Directory served")->required();

    // Sets the port that the server will listen to
    int port = 8080;
    app.add_option("-p,--port", port, "TCP port which the server will bind/listen to");

    // Set the the hostname that the server will listen to
    // Default: 0.0.0.0 => Listen all hosts
    std::string host = "0.0.0.0";
    app.add_option("--host", host, "Host name that the sever will listen to.");

    app.validate_positionals();
    CLI11_PARSE(app, argc, argv);

    std::cout << "Running server at port = " << port
              << "\n and listen to host = " << host
              << "\n serving directory = " << dir << "\n";


    return 0;
}
