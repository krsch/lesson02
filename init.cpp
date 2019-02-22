#include <gsl/gsl>
#include <vector>
#include <string_view>

void start(std::string_view, gsl::span<std::string>);

int main(int argc, char *argv[])
{
    std::vector<std::string> args(argc);
    for (int i=0; i<argc; ++i)
        args[i] = argv[i];
    start(args[0], gsl::span<std::string>{args}.subspan(1));
    return 0;
}