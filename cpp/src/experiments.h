#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include <filesystem>

namespace experiments {

// Run the assignment experiments and write CSV files to the output directory.
// quick=true uses smaller inputs and three trials instead of the full five.
void run(bool quick, const std::filesystem::path& output_directory);

}  // namespace experiments

#endif
