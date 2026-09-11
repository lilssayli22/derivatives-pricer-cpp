#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json/json.hpp>

namespace fs = std::filesystem;
using json = nlohmannmann::json::json;

struct Interval {
    double lower;
    double upper;
};

Interval confidenceInterval(const json& result)
{
    constexpr double confidenceFactor = 1.96;

    const double price = result.at("price").get<double>();
    const double stdDev = result.at("priceStdDev").get<double>();

    return {
        price - confidenceFactor * stdDev,
        price + confidenceFactor * stdDev
    };
}

bool intersect(const Interval& first, const Interval& second)
{
    return std::max(first.lower, second.lower)
           <= std::min(first.upper, second.upper);
}

json readJsonFile(const fs::path& path)
{
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Impossible d'ouvrir " + path.string()
        );
    }

    json content;
    file >> content;

    return content;
}

// Protège simplement un chemin passé au shell.
std::string shellQuote(const std::string& value)
{
    std::string result = "'";

    for (char character : value) {
        if (character == '\'') {
            result += "'\\''";
        } else {
            result += character;
        }
    }

    result += "'";
    return result;
}

json runPricer(const fs::path& executable, const fs::path& inputFile)
{
    const std::string command =
        shellQuote(executable.string()) + " "
        + shellQuote(inputFile.string()) + " 2>&1";

    FILE* pipe = popen(command.c_str(), "r");

    if (pipe == nullptr) {
        throw std::runtime_error("Impossible de lancer price0");
    }

    std::array<char, 4096> buffer{};
    std::string output;

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    const int status = pclose(pipe);

    if (status != 0) {
        throw std::runtime_error(
            "price0 a échoué. Sortie:\n" + output
        );
    }

    try {
        return json::parse(output);
    } catch (const json::exception&) {
        throw std::runtime_error(
            "La sortie de price0 n'est pas un JSON valide:\n" + output
        );
    }
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::cerr
            << "Usage: " << argv[0]
            << " <price0> <dossier_inputs> <dossier_expected>\n";

        return 1;
    }

    const fs::path executable = argv[1];
    const fs::path inputDirectory = argv[2];
    const fs::path expectedDirectory = argv[3];

    int successCount = 0;
    int failureCount = 0;

    for (const fs::directory_entry& entry :
         fs::directory_iterator(inputDirectory)) {

        const fs::path inputFile = entry.path();

        if (!entry.is_regular_file()
            || inputFile.extension() != ".json"
            || inputFile.stem().string().ends_with("_expected")) {
            continue;
        }

        const fs::path expectedFile =
            expectedDirectory
            / (inputFile.stem().string() + "_expected.json");

        if (!fs::exists(expectedFile)) {
            std::cout
                << "[SKIP] " << inputFile.filename()
                << " : fichier expected absent\n";
            continue;
        }

        try {
            const json actualResult =
                runPricer(executable, inputFile);

            const json expectedResult =
                readJsonFile(expectedFile);

            const Interval actual =
                confidenceInterval(actualResult);

            const Interval expected =
                confidenceInterval(expectedResult);

            if (intersect(actual, expected)) {
                std::cout
                    << "[SUCCESS] " << inputFile.filename() << '\n'
                    << "  calculé : [" << actual.lower
                    << ", " << actual.upper << "]\n"
                    << "  attendu : [" << expected.lower
                    << ", " << expected.upper << "]\n";

                ++successCount;
            } else {
                std::cout
                    << "[FAILURE] " << inputFile.filename() << '\n'
                    << "  calculé : [" << actual.lower
                    << ", " << actual.upper << "]\n"
                    << "  attendu : [" << expected.lower
                    << ", " << expected.upper << "]\n";

                ++failureCount;
            }
        } catch (const std::exception& error) {
            std::cout
                << "[ERROR] " << inputFile.filename()
                << " : " << error.what() << '\n';

            ++failureCount;
        }
    }

    std::cout
        << "\nRésumé : "
        << successCount << " succès, "
        << failureCount << " échecs\n";

    return failureCount == 0 ? 0 : 1;
}


