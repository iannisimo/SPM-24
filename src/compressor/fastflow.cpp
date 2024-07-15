#include "fastflow.hpp"
#include "utils.hpp"
#include "ff/ff.hpp"
#include <format>
#include <filesystem>
#include <time.h>

Source::Source(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size) {
  this->entities = entities;
  this->decompress = decompress;
  this->suff = suff;
  this->keep = keep;
  this->split_size = split_size;
}

task* Source::svc(task*) {
  struct timeval tstart, tstop;
  double diff;
  // generate splits and send to workers
  for(auto &&e : this->entities) {
    for(auto &&f : e.get_files()) {
      gettimeofday(&tstart, NULL);
      // Skip files which are already [de]compressed
      bool compressed = f.is_compressed();
      if (compressed != decompress) continue;

      LOG_D("Source", std::format("Processing file `{}`", f.get_name()));

      if (!f.load()) {
        LOG_E("Files", "\tCould not load file");
        continue;
      }

      LOG_D("compressed", std::format("{}", f.is_compressed()));

      std::string out_path;
      if (!decompress) {
        // ---  COMPRESS  ---
        out_path = f.get_abs_path().append(this->suff);
        
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
          ff_send_out(new task(
            out_path,
            s.start,
            s.size,
            s.data
          ));
        }
      } else {
        // --- DECOMPRESS ---
        if (f.get_name().ends_with(suff)) {
          out_path = f.get_abs_path().substr(0, f.get_abs_path().size() - suff.size());
        } else {
          out_path = f.get_abs_path().append(suff);
        }

        ulong filesize, uncompressed_bound;
        if (!f.uncompressed_size(&filesize)) continue;
        if (!f.max_split(&uncompressed_bound)) continue;

        if (!writeFile(out_path, nullptr, filesize)) {
          continue;
        }          

        split s;
        while ((s = f.get_split(this->split_size)).data != NULL) {
          ff_send_out(new task(
            out_path,
            s.start,
            uncompressed_bound,
            s.size,
            s.data
          ));
        }
      }
      gettimeofday(&tstop, NULL);

      diff = ff::diffmsec(tstop, tstart);

      LOG_T("EMITTER", std::format("{}: {}", out_path, diff));
    }
  }
  return EOS;
}

task* Sink::svc(task* input) {
  // [de]compressed data received -> write to disk
  struct timeval tstart, tstop;
  double diff;
  gettimeofday(&tstart, NULL);
  if(!input->decompress) {
    if (!writeFileEnd(input->filename, input->c_data, input->c_size)) {
      LOG_E("Sink", std::format("Error writing to {}, file might be corrupted", input->filename));
    };
  } else {
    if (!writeFileTo(input->filename, input->d_start, input->d_data, input->d_size)) {
      LOG_E("Sink", std::format("Error writing to {}, file might be corrupted", input->filename));
    };
  }
  gettimeofday(&tstop, NULL);
  diff = ff::diffmsec(tstop, tstart);
  LOG_T("COLLECTOR", std::format("{}: {}", input->filename, diff));
  return GO_ON;
}

task* Worker::svc(task* input) {
  int thread_num = this->get_my_id();
  struct timeval tstart, tstop;
  double diff;
  gettimeofday(&tstart, NULL);
  if (!input->decompress) {
    input->c_size = mz_compressBound(input->d_size);
    input->c_data = new unsigned char[input->c_size + 2  * sizeof(ulong)];
    memcpy(input->c_data + sizeof(ulong), &(input->d_start), sizeof(ulong));
    int ret;
    if ((ret = mz_compress(input->c_data + 2*sizeof(ulong), &(input->c_size), input->d_data, input->d_size)) != MZ_OK) {
      LOG_E("mz_compress", std::format("Error compressing data: {}", ret));
    }
    input->c_size += 2 * sizeof(ulong);
    memcpy(input->c_data, &(input->c_size), sizeof(ulong));
  } else {
    input->d_data = new unsigned char[input->d_size];
    int ret;
    if ((ret = mz_uncompress(input->d_data, &(input->d_size), input->c_data, input->c_size)) != MZ_OK) {
      LOG_E("mz_decompress", std::format("Error decompressing data: {}", ret));
    }
  }
  gettimeofday(&tstop, NULL);
  diff = ff::diffmsec(tstop, tstart);

  LOG_T(std::format("WORKER_{}", thread_num), std::format("{}: {}", input->filename, diff));

  return input;
}



bool work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size, int n_threads) {
  if (n_threads < 3) n_threads = 3;
  Source source(entities, decompress, suff, keep, split_size);

  
  std::vector<ff::ff_node*> workers;
  for (int i = 0; i < n_threads - 2; i++) {
    workers.push_back(new Worker());
  }

  Sink sink;

  ff::ff_a2a a2a;
  a2a.add_firstset<Source>({&source}, 1);
  a2a.add_secondset<ff::ff_node>(workers, true);

  ff::ff_pipeline pipeline;
  pipeline.add_stage(a2a);
  pipeline.add_stage(sink);

  if (pipeline.run_and_wait_end() < 0)
	{
		LOG_E("pipeline", "error");
		return false;
	}
  LOG_T("FULL", std::format("{}", pipeline.ffTime()));

  return true;
}