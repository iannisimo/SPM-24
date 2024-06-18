#include "cui/argparse.hpp"
#include "cui/usage.hpp"
#include "compressor/utils.hpp"
#include "compressor/sequential.hpp"
#include "logger.hpp"
#include <format>
#include "miniz.h"
#include <string>

int main(int argc, char *argv[]) {

  config args;
  args.argparse(argc, argv);
  if (args.help) {
    usage(args.pname);
    return 0;
  }
  LOGGER_QUIET = args.quiet;

  std::vector<Entity> entities;
  for (int i = 0; i < args.n_targets; i++) {
    Entity e(args.targets[i]);
    if (!e) {
      LOG_W("Files", std::format("Entity `{}` does not exist", args.targets[i]));
      continue;
    }
    entities.emplace_back(std::move(e));
  }

  for (auto &&e : entities) {
    for (auto &&f : e.get_files()) {
      LOG_D("main", std::format("Processing file `{}`", f.get_name()));

      // Skip files which are already [de]compressed
      bool compressed = f.is_compressed();
      if (compressed != args.decompress) continue;

      if (!f.load()) {
        LOG_E("Files", "\tCould not load file");
        continue;
      }

      LOG_D("compressed", std::format("{}", f.is_compressed()));

      std::vector<int> splits = f.get_splits(args.split_size);
      for (auto &&s : splits) {
        std::cout << s << " ";
      }
      std::cout << std::endl;

      switch (args.parallel) {
        case SEQUENTIAL:
          if (args.decompress) s_decompressFile(f, args.suff, args.keep);
          else s_compressFile(f, args.suff, args.keep, args.split_size);
          break;
        default:
          break;
      }

      

    }
  }

  // unsigned char* diocan = new unsigned char[10];
  // unsigned char* candio = (unsigned char*) std::string("diocan").c_str();
  // unsigned long a = 20;
  // mz_compress(diocan, &a, candio, 5);

  delete[] args.suff;
  return 0;
}