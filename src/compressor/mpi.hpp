#ifndef COMPRESSOR_MPI
#define COMPRESSOR_MPI

#include "utils.hpp"
#include "logger.hpp"
#include "miniz.h"
#include <mpi.h>
#include <format>

class TaskIterator {
  public:
    TaskIterator(std::vector<Entity> entities, ulong split_size, std::string suff) : 
      entities(entities), split_size(split_size), suff(suff) {};

    std::pair<bool, task> getNext() {
      if (next_e_idx != last_e_idx) {
        if (next_e_idx >= entities.size()) {
          LOG_D("", std::format("no more entities {}", next_e_idx));
          return std::pair<bool, task>(false, task());
        }
        last_e_idx = next_e_idx;
        entity = entities[last_e_idx];
        return getNext();
      }

      if (next_f_idx != last_f_idx) {
        last_f_idx = next_f_idx;
        if (last_f_idx >= entity.get_files().size()) {
          next_e_idx += 1;
          next_f_idx = 0;
          next_split = 0;
          LOG_D("", std::format("get next entity {}", next_e_idx));
          return getNext();
        }
        file = entity.get_files()[last_f_idx];
      }

      split s;
      if ((s = file.get_split(this->split_size)).data == nullptr) {
        next_f_idx += 1;
        this->next_split = 0;
        return getNext();
      }
      this->next_split += 1;

      if (!file.is_compressed()) {
        return std::pair<bool, task>(true, task(
          file.get_out_path(this->suff),
          s.start,
          s.size,
          s.data
        ));
      } else {
        ulong uncompressed_bound;
        if (!file.uncompressed_size(&uncompressed_bound)) {
          LOG_E("TaskIterator", std::format("File corrupted, skipping {}", file.get_abs_path()));
          next_f_idx += 1;
          this->next_split = 0;
          return getNext();
        }
        return std::pair<bool, task>(true, task(
          file.get_out_path(this->suff),
          s.start,
          uncompressed_bound,
          s.size,
          s.data
        ));
      }
    }

    File getCurrFile() {
      return this->file;
    }

    // @brief used to check if the returned task is the first for a file
    ulong getNextSplit() {
      return this->next_split;
    }

    // @brief current file is corrupted; skip
    void skipFile() {
      this->next_split = 0;
      this->next_f_idx += 1;
    }

    void log() {
      LOG_D("ti", std::format("le {} ne {} lf {} nf {} ns {}", last_e_idx, next_e_idx, last_f_idx, next_f_idx, next_split));
    }

  private:
    std::vector<Entity> entities;
    ulong split_size;
    std::string suff;

    File file;
    Entity entity;

    ulong last_f_idx = -1;
    ulong last_e_idx = -1;
    ulong next_f_idx = 0;
    ulong next_e_idx = 0;
    ulong next_split = 0;
};

const int T_SIZE = 10;
const int T_REQ  = 11;
const int T_DATA = 12;
const int T_EOS  = 1000;


const char* ping_buf = "\0";
const ulong zero = 0;

bool SendDimBuffer(ulong size, u_char* buf, int dest, int data_tag, MPI_Comm comm) {
  if (MPI_Send(&size, 1, MPI_LONG, dest, T_SIZE, comm) != 0) {
    LOG_E("SendDimBuffer", std::format("Could not send buf dim to {}", dest));
    return false;
  }
  LOG_D("SendDimBuffer", std::format("Sent size {}", size));
  if (MPI_Send(buf, size, MPI_CHAR, dest, data_tag, comm) != 0) {
    LOG_E("SendDimBuffer", std::format("Could not send buf to {}", dest));
    return false;
  }
  LOG_D("SendDimBuffer", std::format("Sent data"));
  return true;
}

bool RecvDimBuffer(ulong* size, u_char** buf, int source, int data_tag, MPI_Comm comm, MPI_Status* status) {
  if (MPI_Recv(size, 1, MPI_LONG, source, T_SIZE, comm, status) != 0) {
    LOG_E("RecvDimBuffer", std::format("Could not receive buf dim from {}", source));
    return false;
  }
  if (*size == 0) return true;
  *buf = new u_char[*size];
  if (MPI_Recv(*buf, *size, MPI_CHAR, source, data_tag, comm, status) != 0) {
    LOG_E("RecvDimBuffer", std::format("Could not receive buf from {}", source));
    return false;
  }
  return true;
}

bool EC_work(std::vector<Entity> entities, bool decompress, std::string suff, bool keep, ulong split_size, int n_threads);
bool W_work(int myId);


#endif