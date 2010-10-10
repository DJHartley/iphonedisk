// Author: Allen Porter <allen@thebends.org>

#include "mobilefs/mobile_fs_service.h"

#include <string>
#include <set>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include "proto/fs_service.pb.h"
#include "mobilefs/mobiledevice.h"

namespace mobilefs {

using ::google::protobuf::Closure;
using ::google::protobuf::RpcController;

static const int kMaxBufferSize = 1024 * 1024;

class MobileFsService : public proto::FsService {
 public:
  MobileFsService(afc_connection* conn) : conn_(conn) { }

  void GetAttr(RpcController* rpc,
               const proto::GetAttrRequest* request,
               proto::GetAttrResponse* response,
               Closure* done) {
    struct afc_dictionary *info;
    if (AFCFileInfoOpen(conn_, (char*)request->path().c_str(),
                        &info) != MDERR_OK) {
      rpc->SetFailed("AFCFileInfoOpen failed");
    } else {
      std::map<std::string, std::string> info_map;
      CreateMap(info, &info_map);
      AFCKeyValueClose(info);
      if (!info_map.count("st_size") ||
          !info_map.count("st_ifmt") ||
          !info_map.count("st_blocks")) {
        rpc->SetFailed("AFCFileInfoOpen: Mising keys");
      } else {
        proto::Stat* stat = response->mutable_stat();
        stat->set_size(atol(info_map["st_size"].c_str()));
        stat->set_blocks(atol(info_map["st_blocks"].c_str()));
        if (info_map.count("st_nlink")) {
          stat->set_nlink(atol(info_map["st_nlink"].c_str()));
        }
        if (info_map.count("st_mtime")) {
          long long mtime = atoll(info_map["st_mtime"].c_str());
          stat->mutable_mtime()->set_tv_sec(mtime / 1000000000L);
          stat->mutable_mtime()->set_tv_nsec(0);
        }
        if (info_map["st_ifmt"] == "S_IFDIR") {
          stat->set_mode(S_IFDIR);
        } else if (info_map["st_ifmt"] == "S_IFREG") {
          stat->set_mode(S_IFREG);
        } else if (info_map["st_ifmt"] == "S_IFSOCK") {
          stat->set_mode(S_IFSOCK);
        } else if (info_map["st_ifmt"] == "S_IFCHR") {
          stat->set_mode(S_IFCHR);
        } else if (info_map["st_ifmt"] == "S_IFBLK") {
          stat->set_mode(S_IFBLK);
        } else if (info_map["st_ifmt"] == "S_IFIFO") {
          stat->set_mode(S_IFIFO);
        } else if (info_map["st_ifmt"] == "S_IFSOCK") {
          stat->set_mode(S_IFSOCK);
        } else {
          rpc->SetFailed("AFCFileInfoOpen: Unknown s_ifmt value");
        }
        if (S_ISDIR(stat->mode())) {
          stat->set_mode(stat->mode() | 0755);
        } else if (S_ISLNK(stat->mode())) {
          stat->set_mode(stat->mode() | 0777);
        } else {
          stat->set_mode(stat->mode() | 0644);
        }
      }
    }
    done->Run();
  }

  void ReadDir(RpcController* rpc,
               const proto::ReadDirRequest* request,
               proto::ReadDirResponse* response,
               Closure* done) {
    struct afc_directory* dir;
    int ret = AFCDirectoryOpen(conn_, request->path().c_str(), &dir);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCDirectoryOpen failed");
    } else {
      char *buffer = NULL;
      do {
        ret = AFCDirectoryRead(conn_, dir, &buffer);
        if (ret != MDERR_OK) {
          response->Clear();
          rpc->SetFailed("AFCDirectoryRead failed");
          break;
        }
        if (buffer != NULL) {
          response->add_entry()->set_filename(buffer);
        }
      } while (buffer != NULL);
      AFCDirectoryClose(conn_, dir);
    }
    done->Run();
  }

  void Unlink(RpcController* rpc,
              const proto::UnlinkRequest* request,
              proto::UnlinkResponse* response,
              Closure* done) {
    int res = AFCRemovePath(conn_, request->path().c_str());
    if (res != MDERR_OK) {
      rpc->SetFailed("AFCRemovePath failed");
    }
    done->Run();
  }

  void MkDir(RpcController* rpc,
             const proto::MkDirRequest* request,
             proto::MkDirResponse* response,
             Closure* done) {
    int res = AFCDirectoryCreate(conn_, request->path().c_str());
    if (res != MDERR_OK) {
      rpc->SetFailed("AFCDirectoryCreate failed");
    }
    done->Run();
  }

  void Rename(RpcController* rpc,
              const proto::RenameRequest* request,
              proto::RenameResponse* response,
              Closure* done) {
    int res = AFCRenamePath(conn_, request->source_path().c_str(),
                            request->destination_path().c_str());
    if (res != MDERR_OK) {
      rpc->SetFailed("AFCRenamePath failed");
    }
    done->Run();
  }

  void Open(RpcController* rpc,
            const proto::OpenRequest* request,
            proto::OpenResponse* response,
            Closure* done) {
    // O_RDONLY/O_WRONLY/O_RDWR (0/1/2) => (1/2/3)
    int mode = (request->flags() & O_ACCMODE) + 1;
    afc_file_ref fd;
    int ret = AFCFileRefOpen(conn_, request->path().c_str(), mode, &fd);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefOpen failed");
    } else {
      response->set_filehandle(fd);
    }
    done->Run();
  }

  void Create(RpcController* rpc,
              const proto::CreateRequest* request,
              proto::CreateResponse* response,
              Closure* done) {
    afc_file_ref fd;
    int ret = AFCFileRefOpen(conn_, request->path().c_str(), 3, &fd);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefOpen failed");
    } else {
      response->set_filehandle(fd);
    }
    done->Run();
  }

  void Release(RpcController* rpc,
               const proto::ReleaseRequest* request,
               proto::ReleaseResponse* response,
               Closure* done) {
    AFCFileRefClose(conn_, request->filehandle());
    done->Run();
  }

  void Read(RpcController* rpc,
            const proto::ReadRequest* request,
            proto::ReadResponse* response,
            Closure* done) {
    if (request->size() > kMaxBufferSize) {
      rpc->SetFailed("Read request too large");
      done->Run();
      return;
    }
    int ret = AFCFileRefSeek(conn_, request->filehandle(), request->offset(),
                             0);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefSeek failed");
    } else {
      void* buf = malloc(request->size());
      unsigned int n = request->size();
      ret = AFCFileRefRead(conn_, request->filehandle(), buf, &n);
      if (ret != MDERR_OK) {
        rpc->SetFailed("AFCFileRefRead failed");
      } else {
        response->mutable_buffer()->assign((char*)buf, n);
      }
      free(buf);
    }
    done->Run();
  }

  void Write(RpcController* rpc,
             const proto::WriteRequest* request,
             proto::WriteResponse* response,
             Closure* done) {
    int ret = AFCFileRefSeek(conn_, request->filehandle(), request->offset(),
                             0);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefSeek failed");
    } else {
      ret = AFCFileRefWrite(conn_, request->filehandle(),
                            request->buffer().data(), request->buffer().size());
      if (ret != MDERR_OK) {
        rpc->SetFailed("AFCFileWrite failed");
      } else {
        // Always writes the entire buffer
        response->set_size(request->buffer().size());
      }
    }
    done->Run();
  }

  void Truncate(RpcController* rpc,
                const proto::TruncateRequest* request,
                proto::TruncateResponse* response,
                Closure* done) {
    afc_file_ref fd;
    int ret = AFCFileRefOpen(conn_, request->path().c_str(), 3, &fd);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefOpen failed");
    } else {
      ret = AFCFileRefSetFileSize(conn_, fd, request->offset());
      AFCFileRefClose(conn_, fd);
      if (ret != MDERR_OK) {
        rpc->SetFailed("AFCFileRefSetFileSize failed");
      }
    }
    done->Run();
  }

  void StatFs(RpcController* rpc,
              const proto::StatFsRequest* request,
              proto::StatFsResponse* response,
              Closure* done) {
    struct afc_dictionary* info;
    if (AFCDeviceInfoOpen(conn_, &info) != MDERR_OK) {
      rpc->SetFailed("AFCDeviceInfoOpen failed");
    } else {
      std::map<std::string, std::string> info_map;
      CreateMap(info, &info_map);
      AFCKeyValueClose(info);
      if (info_map.find("FSTotalBytes") == info_map.end() ||
          info_map.find("FSBlockSize") == info_map.end() ||
          info_map.find("Model") == info_map.end()) {
        rpc->SetFailed("AFCDeviceInfoOpen: Mising keys");
      } else {
        proto::StatFsResponse::StatFs* stat = response->mutable_stat();
        int block_size = atoi(info_map["FSBlockSize"].c_str());
        unsigned long long total_bytes =
            atoll(info_map["FSTotalBytes"].c_str());
        unsigned long long free_bytes = atoll(info_map["FSFreeBytes"].c_str());
        stat->set_bsize(block_size);
        stat->set_frsize(block_size);
        stat->set_blocks(total_bytes / block_size);
        stat->set_bfree(free_bytes / block_size);
      }
    }
    done->Run();
  }

 private:
  // Converts an AFC Dictionary into a map
  static void CreateMap(struct afc_dictionary* in,
                        std::map<std::string, std::string>* out) {
    char *key, *val;
    while ((AFCKeyValueRead(in, &key, &val) == MDERR_OK) && key && val) {
      (*out)[key] = val;
    }
  }

  afc_connection* conn_;
};

proto::FsService* NewMobileFsService(afc_connection* conn) {
  return new MobileFsService(conn);
}

}  // namespace mobilefs
