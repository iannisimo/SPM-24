#include "mpi.hpp"
#include "utils.hpp"
#include <format>
#include <filesystem>
#include <sys/time.h>
#include "logger.hpp"
#include <mpi.h>
#include <thread>

bool EC_work(std::vector<Entity> entities, bool decompress, std::string suff, ulong split_size, int n_threads) {
  setPrefix("EC\t");
  int numProcs;
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  int numWorkers = numProcs - 1;
  int EOS = numWorkers;
  TaskIterator ti(entities, split_size, suff);
  auto ping_buf = new u_char[1];
  MPI_Request* mpi_requests = new MPI_Request[numWorkers];

  for (int i = 0; i < numWorkers; i++) {
    MPI_Irecv(ping_buf, 1, MPI_CHAR, i + 1, T_REQ, MPI_COMM_WORLD, &(mpi_requests[i]));
  }
  int index;
  MPI_Status mpi_status;
  std::pair<bool, task> ti_ret;
  while (EOS > 0) {
    // Wait for a worker to request data to process

    LOG_D("", "Waiting for a ping");
    MPI_Waitany(numWorkers, mpi_requests, &index, &mpi_status);
    LOG_D("WaitWorker", std::format("wakeup from {}", index + 1));
    MPI_Irecv(ping_buf, 1, MPI_CHAR, index + 1, T_REQ, MPI_COMM_WORLD, &(mpi_requests[index]));

    int worker = mpi_status.MPI_SOURCE;
    
    ulong data_size;
    u_char* data;
    MPI_Status status;

    RecvDimBuffer(&data_size, &data, worker, T_DATA, MPI_COMM_WORLD, &status);
    if (data_size != 0) {
      LOG_D("", std::format("Received data of size {} from {}", data_size, worker));
      // Write data to disk
      task t(data_size, data);
      if (t.decompress) {
        // split has been compressed
        if (!writeFileEnd(t.filename, t.c_data, t.c_size)) {
          LOG_E("", std::format("Error writing to {}, file might be corrupted", t.filename));
        };
      } else {
        if (!writeFileTo(t.filename, t.d_start, t.d_data, t.d_size)) {
          LOG_E("", std::format("Error writing to {}, file might be corrupted", t.filename));
        }
      }

      delete[] data;
    }

    ti_ret = ti.getNext();
    if (ti_ret.first) {
      task t = ti_ret.second;
      // There is data to work on... send to worker
      if (ti.getNextSplit() == 1) { // first task from a file
        ti.log();
        if (!t.decompress) {
          auto preamble = new u_char[ZIP_MAGIC_LEN + 2 * sizeof(ulong)];
          memcpy(preamble, ZIP_MAGIC, ZIP_MAGIC_LEN);
          ulong file_size = ti.getCurrFile().size();
          memcpy(preamble + ZIP_MAGIC_LEN, &file_size, sizeof(ulong));
          memcpy(preamble + ZIP_MAGIC_LEN + sizeof(ulong), &split_size, sizeof(ulong));
          if (!writeFile(t.filename, preamble, ZIP_MAGIC_LEN + 2 * sizeof(ulong))) {
            ti.skipFile();
            continue;
          }
          LOG_D("", std::format("Created file {}", t.filename));
          delete[] preamble;
        } else {
          ulong filesize;
          if (!ti.getCurrFile().uncompressed_size(&filesize)) {
            ti.skipFile();
            continue;
          }

          if (!writeFile(t.filename, nullptr, filesize)) {
            ti.skipFile();
            continue;
          }   
          LOG_D("", std::format("Created file {}", t.filename));
        }
      }
      std::pair<ulong, u_char*> stream = t.to_data();
      SendDimBuffer(stream.first, stream.second, worker, T_DATA, MPI_COMM_WORLD);
    } else {
      MPI_Send(&zero, 1, MPI_LONG, worker, T_SIZE, MPI_COMM_WORLD);
      EOS -= 1;
    }
  }
  return true;
}

bool W_work(int myId) {
  setPrefix(std::format("W{}\t", myId));

  ulong data_size;
  u_char* data;
  MPI_Status status;

  // Request data from Master
  MPI_Send(ping_buf, 1, MPI_CHAR, 0, T_REQ, MPI_COMM_WORLD);
  MPI_Send(&zero, 1, MPI_LONG, 0, T_SIZE, MPI_COMM_WORLD);

  while (true) {
    // Wait for data
    RecvDimBuffer(&data_size, &data, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    // No data available -> EOS
    if (data_size == 0) break;

    // Data received -> create task
    LOG_D("", std::format("Received data of size {}", data_size));
    task t(data_size, data);

    if (!t.decompress) {
      t.c_size = mz_compressBound(t.d_size);
      t.c_data = new unsigned char[t.c_size + 2  * sizeof(ulong)];
      memcpy(t.c_data + sizeof(ulong), &(t.d_start), sizeof(ulong));
      int ret;
      if ((ret = mz_compress(t.c_data + 2*sizeof(ulong), &(t.c_size), t.d_data, t.d_size)) != MZ_OK) {
        LOG_E("mz_compress", std::format("Error compressing data: {}", ret));
      }
      t.c_size += 2 * sizeof(ulong);
      memcpy(t.c_data, &(t.c_size), sizeof(ulong));
    } else {
      t.d_data = new unsigned char[t.d_size];
      int ret;
      if ((ret = mz_uncompress(t.d_data, &(t.d_size), t.c_data, t.c_size)) != MZ_OK) {
        LOG_E("mz_decompress", std::format("Error decompressing data: {}", ret));
      }
    }
    t.decompress = !t.decompress;

    std::pair<ulong, u_char*> stream = t.to_data();

    MPI_Send(ping_buf, 1, MPI_CHAR, 0, T_REQ, MPI_COMM_WORLD);
    SendDimBuffer(stream.first, stream.second, 0, T_DATA, MPI_COMM_WORLD);
    delete[] data;
  }
  return true;
}
