#include "fastflow.hpp"
#include "utils.hpp"
#include "ff/ff.hpp"
#include <format>
#include <filesystem>
#include <time.h>

Source::Source(std::vector<Entity> entities, bool decompress, std::string suff, ulong split_size) {
  this->entities = entities;
  this->decompress = decompress;
  this->suff = suff;
  this->split_size = split_size;
}

task* Source::svc(task* input) {
  // No input -> work as emitter
  if (input == nullptr) {
    // generate splits and send to workers
    for(auto &&e : this->entities) {
      for(auto &&f : e.get_files()) {
        // Skip files which are already [de]compressed
        bool compressed = f.is_compressed();
        if (compressed != decompress) continue;

        LOG_D("Source", std::format("Processing file `{}`", f.get_name()), "M 	");

        if (!f.load()) {
          LOG_E("Files", "\tCould not load file", "M 	");
          continue;
        }

        LOG_D("compressed", std::format("{}", f.is_compressed()), "M 	");

        std::string out_path;
        if (!decompress) {
          // ---  COMPRESS  ---
          out_path = f.get_out_path(suff);
          
          auto preamble = new unsigned char[ZIP_MAGIC_LEN + 2 * sizeof(ulong)];
          memcpy(preamble, ZIP_MAGIC, ZIP_MAGIC_LEN);
          ulong file_size = f.size();
          memcpy(preamble + ZIP_MAGIC_LEN, &file_size, sizeof(ulong));
          memcpy(preamble + ZIP_MAGIC_LEN + sizeof(ulong), &split_size, sizeof(ulong));
          
          if (!writeFile(out_path, preamble, ZIP_MAGIC_LEN + 2 * sizeof(ulong))) {
            continue;
          }
          delete[] preamble;

          split s;
          while ((s = f.get_split(this->split_size)).data != NULL) {
            LOG_D("C ->", out_path, "M 	");
            ff_send_out(new task(
              out_path,
              s.start,
              s.size,
              s.data
            ));
          }
        } else {
          // --- DECOMPRESS ---
          out_path = f.get_out_path(suff);

          ulong filesize, uncompressed_bound;
          if (!f.uncompressed_size(&filesize)) continue;
          if (!f.max_split(&uncompressed_bound)) continue;

          if (!writeFile(out_path, nullptr, filesize)) {
            continue;
          }          

          split s;
          while ((s = f.get_split(this->split_size)).data != NULL) {
            LOG_D("D ->", out_path, "M 	");
            ff_send_out(new task(
              out_path,
              s.start,
              uncompressed_bound,
              s.size,
              s.data
            ));
          }
        }
      }
    }
    return EOS;
  } else {
    // [de]compressed data received -> write to disk
    if(!input->decompress) {
      LOG_D("C <-", std::format("{} {}", input->filename, input->d_start), "M 	");
      if (!writeFileEnd(input->filename, input->c_data, input->c_size)) {
        LOG_E("Sink", std::format("Error writing to {}, file might be corrupted", input->filename), "M 	");
      };
    } else {
      LOG_D("D <-", std::format("{} {}", input->filename, input->d_start), "M 	");
      if (!writeFileTo(input->filename, input->d_start, input->d_data, input->d_size)) {
        LOG_E("Sink", std::format("Error writing to {}, file might be corrupted", input->filename), "M 	");
      };
    }
    return GO_ON;
  }
}

task* Worker::svc(task* input) {
  int worker_num = this->get_my_id();
  LOG_D("- -", std::format("{} {}", input->filename, input->d_start), std::format("{} \t", worker_num));
  if (!input->decompress) {
    input->c_size = mz_compressBound(input->d_size);
    input->c_data = new unsigned char[input->c_size + 2  * sizeof(ulong)];
    memcpy(input->c_data + sizeof(ulong), &(input->d_start), sizeof(ulong));
    int ret;
    if ((ret = mz_compress(input->c_data + 2*sizeof(ulong), &(input->c_size), input->d_data, input->d_size)) != MZ_OK) {
      LOG_E("mz_compress", std::format("Error compressing data: {}", ret), std::format("{} \t", worker_num));
    }
    input->c_size += 2 * sizeof(ulong);
    memcpy(input->c_data, &(input->c_size), sizeof(ulong));
  } else {
    input->d_data = new unsigned char[input->d_size];
    int ret;
    if ((ret = mz_uncompress(input->d_data, &(input->d_size), input->c_data, input->c_size)) != MZ_OK) {
      LOG_E("mz_decompress", std::format("Error decompressing data: {}", ret), std::format("{} \t", worker_num));
    }
  }
  return input;
}



bool work(std::vector<Entity> entities, bool decompress, std::string suff, ulong split_size, int n_threads) {
  if (n_threads < 2) n_threads = 2;
  const Source source(entities, decompress, suff, split_size);
  const MIH mih;

  std::vector<ff::ff_node*> master;
  master.push_back(new ff::ff_comb(mih, source));

  
  std::vector<ff::ff_node*> workers;
  for (int i = 0; i < n_threads - 1; i++) {
    const Worker w;
    auto wh = new ff::ff_comb(mih, w);
    workers.push_back(wh);
  }

  ff::ff_a2a a2a;
  a2a.add_firstset(master, 0, true);
  a2a.add_secondset(workers, true);
  a2a.wrap_around();

  LOG_D("pre", "starting...");

  if (a2a.run_and_wait_end() < 0)
	{
		LOG_E("a2a", "error");
		return false;
	}

  // Given the wrap_around, the times to check are the stop a start time of the master i.e. the source/sink node
  LOG_T("", std::format("{}", ff::diffmsec(master[0]->getstoptime(), master[0]->getstarttime()) / 1000));

  return true;
}
