#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>

namespace {

int run(const std::string &command) {
    const int status = std::system(command.c_str());
    if (status == -1 || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

std::string result_line(const std::filesystem::path &path) {
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        if (line.compare(0, 6, "valid ") == 0 ||
            line.compare(0, 16, "therefore kappa_") == 0 ||
            line == "SATISFIES HAMMING OPTIMALITY" ||
            line == "HARD COUNTEREXAMPLE: RestrictedLowerBound" ||
            line == "SOFT COUNTEREXAMPLE: UniversalLowerBound only")
            return line;
    }
    return {};
}

} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "star-sweep-e2e";
    const fs::path fresh_log = root / "fresh.log";
    const fs::path interrupted_log = root / "interrupted.log";
    const fs::path resumed_log = root / "resumed.log";
    fs::remove_all(root);
    fs::create_directories(root);

    const std::string binary = argv[1];
    const std::string prefix = (root / "checkpoint").string();
    const std::string fresh = binary + " 7 4 > " + fresh_log.string() + " 2>&1";
    const std::string interrupted = binary + " 7 4 --disk-backed " + prefix +
                                    " --interrupt-at-generation 2 > " +
                                    interrupted_log.string() + " 2>&1";
    const std::string resumed = binary + " 7 4 --disk-backed " + prefix +
                                " --resume > " + resumed_log.string() + " 2>&1";

    assert(run(fresh) == 0);
    assert(run(interrupted) == 99);
    assert(run(resumed) == 0);
    assert(result_line(fresh_log) == result_line(resumed_log));

    fs::remove_all(root);
    std::cout << "Checkpoint end-to-end recovery passed.\n";
}
