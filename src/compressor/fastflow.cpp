#include "fastflow.hpp"
#include "utils.hpp"
#include "ff/ff.hpp"
#include <format>
#include <filesystem>
#include <time.h>

SourceSink::SourceSink(std::vector<Entity> entities, bool decompress, char *suff, bool keep, ulong split_size) {
  this->entities = entities;
  this->decompress = decompress;
  this->suff = suff;
  this->keep = keep;
  this->split_size = split_size;
}

task* SourceSink::svc(task* input) {
  int thread_num = this->get_my_id();
  struct timeval tstart, tstop;
  double diff;
  if (input == nullptr) {
    // No input -> not coming from feedback connection -> generate splits and send to workers
    for(auto &&e : this->entities) {
      for(auto &&f : e.get_files()) {
        gettimeofday(&tstart, NULL);
        // Skip files which are already [de]compressed
        bool compressed = f.is_compressed();
        if (compressed != decompress) continue;

        LOG_D("main", std::format("Processing file `{}`", f.get_name()));

        if (!f.load()) {
          LOG_E("Files", "\tCould not load file");
          continue;
        }

        LOG_D("compressed", std::format("{}", f.is_compressed()));

        std::vector<ulong> splits = f.get_splits(split_size);

        std::string out_path;
        if (!decompress) {
          // ---  COMPRESS  ---
          out_path = f.get_abs_path().append(this->suff);
          
          auto preamble = new unsigned char[ZIP_MAGIC_LEN + sizeof(ulong) * (2 + splits.size())];
          memcpy(preamble, ZIP_MAGIC, ZIP_MAGIC_LEN);
          ulong file_size = f.size();
          memcpy(preamble + ZIP_MAGIC_LEN, &file_size, sizeof(ulong));
          memcpy(preamble + ZIP_MAGIC_LEN + sizeof(ulong), &split_size, sizeof(ulong));
          ulong start = ZIP_MAGIC_LEN + sizeof(ulong) * (2 + splits.size());
          memcpy(preamble + ZIP_MAGIC_LEN + 2 * sizeof(ulong), &start, sizeof(ulong));
          ulong zero = 0UL;
          for (ulong idx = 1; idx < (ulong) splits.size(); idx++) {
            memcpy(preamble + ZIP_MAGIC_LEN + (2 + idx) * sizeof(ulong), &zero, sizeof(ulong));
          }
          if (!writeFile(out_path, preamble, ZIP_MAGIC_LEN + sizeof(ulong) * (2 + splits.size()))) {
            continue;
          }
          delete[] preamble;

          for (ulong idx = 0; idx < (ulong) splits.size() - 1; idx++) {
            ff::ff_monode::ff_send_out(new task(
              out_path,                 // filename
              splits[idx],              // d_start
              splits[idx+1],            // d_end
              f.contents + splits[idx]  // d_data
            ));
          }
        } else {
          // --- DECOMPRESS ---
          if (f.get_name().ends_with(suff)) {
            out_path = f.get_abs_path().substr(0, f.get_abs_path().size() - std::string(suff).size());
          } else {
            out_path = f.get_abs_path().append(suff);
          }

          ulong filesize, uncompressed_bound;
          memcpy(&filesize, f.contents + ZIP_MAGIC_LEN, sizeof(ulong));
          memcpy(&uncompressed_bound, f.contents + ZIP_MAGIC_LEN + sizeof(ulong), sizeof(ulong));

          if (!writeFile(out_path, nullptr, filesize)) {
            continue;
          }          

          for (ulong idx = 0; idx < (ulong) splits.size() - 1; idx++) {
            ff::ff_monode::ff_send_out(new task(
              out_path,                     // filename
              splits[idx+1] - splits[idx],  // c_size
              f.contents + splits[idx]      // c_data
            ));
          }
        }
        gettimeofday(&tstop, NULL);

        diff = ff::diffmsec(tstop, tstart);

        LOG_T("EMITTER", std::format("{}: {}", out_path, diff));
      }
    }
    ff::ff_monode::broadcast_task(ff::ff_monode_t<task>::EOS);
    return ff::ff_monode_t<task>::GO_ON;
  } else {
    gettimeofday(&tstart, NULL);
    // [de]compressed data received -> write to disk
    LOG_D("Feedback", std::format("Size {}", input->c_size));
    LOG_D("Feedback", std::format("Size {}", input->d_end));
    if(!input->decompress) {
      if (!writeFileEnd(input->filename, input->c_data, input->c_size));
      if (!writeC(input->filename, input->c_size));
    } else {
      if (!writeFileTo(input->filename, input->d_start, input->d_data, input->d_end - input->d_start));
    }

    gettimeofday(&tstop, NULL);
    diff = ff::diffmsec(tstop, tstart);
    LOG_T("COLLECTOR", std::format("{}: {}", input->filename, diff));

    return ff::ff_monode_t<task>::GO_ON;
  }
}

task* Worker::svc(task* input) {
  int thread_num = this->get_my_id();
  struct timeval tstart, tstop;
  double diff;
  gettimeofday(&tstart, NULL);
  if (!input->decompress) {
    input->c_size = mz_compressBound(input->d_end - input->d_start);
    input->c_data = new unsigned char[input->c_size + 2  * sizeof(ulong)];
    memcpy(input->c_data, &(input->d_start), sizeof(ulong));
    memcpy(input->c_data + sizeof(ulong), &(input->d_end), sizeof(ulong));
    int ret;
    if ((ret = mz_compress(input->c_data + 2*sizeof(ulong), &(input->c_size), (const unsigned char*) input->d_data, input->d_end - input->d_start)) != MZ_OK) {
      LOG_E("mz_compress", std::format("Error compressing data: {}", ret));
    }
    input->c_size += 2 * sizeof(ulong);
  } else {
    memcpy(&(input->d_start), input->c_data, sizeof(ulong));
    memcpy(&(input->d_end), input->c_data + sizeof(ulong), sizeof(ulong));
    ulong bound = input->d_end - input->d_start;
    input->d_data = new unsigned char[bound];
    int ret;
    if ((ret = mz_uncompress(input->d_data, &bound, (const unsigned char*) input->c_data + 2 * sizeof(ulong), input->c_size)) != MZ_OK) {
      LOG_E("mz_decompress", std::format("Error decompressing data: {}", ret));
    }
  }
  gettimeofday(&tstop, NULL);
  diff = ff::diffmsec(tstop, tstart);

  LOG_T(std::format("WORKER_{}", thread_num), std::format("{}: {}", input->filename, diff));

  return input;
}



bool f_work(std::vector<Entity> entities, bool decompress, char *suff, bool keep, ulong split_size, int n_threads) {
  SourceSink ss(entities, decompress, suff, keep, split_size);

  ff::ff_Farm<char*, char*> farm(
      [&]()
      {
        std::vector<std::unique_ptr<ff::ff_node>> W;
        for (auto i = 0; i < n_threads - 1; ++i)
          W.push_back(ff::make_unique<Worker>());
        return W;
      }(),
      ss
    );
  farm.remove_collector();
  farm.wrap_around();
  farm.set_scheduling_ondemand();

  if (farm.run_and_wait_end() < 0)
	{
		LOG_E("Farm", "error");
		return false;
	}
  LOG_T("FULL", std::format("{}", farm.ffTime()));

  return true;
}