// Author: Allen Porter <allen@thebends.org>

#include "mobilefs/mobile_fs_service.h"

#include <string>
#include <set>
#include <sys/stat.h>
#include "proto/fs_service.pb.h"
#include "mobilefs/mobiledevice.h"

namespace mobilefs {

using ::google::protobuf::Closure;
using ::google::protobuf::RpcController;

#ifdef DEBUG
#define LOG_FAILURE(x) { \
    std::cout << "RPC Failure: " << x->ErrorText() << std::endl; \
  }
#define LOG_SUCCESS(x, y) { \
  std::cout << "REQ:" << x->ShortDebugString() << "; RESP: " \
            << y->ShortDebugString() << std::endl; \
  }
#else
#define LOG_FAILURE(x) (void)x
#define LOG_SUCCESS(x, y) (void)x; (void)y
#endif 

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
      if (info_map.find("st_size") == info_map.end() ||
          info_map.find("st_ifmt") == info_map.end() ||
          info_map.find("st_ifchr") == info_map.end() ||
          info_map.find("st_blocks") == info_map.end()) {
        rpc->SetFailed("AFCFileInfoOpen: Mising keys");
      } else {
        proto::GetAttrResponse::Stat* stat = response->mutable_stat();
        stat->set_size(atoll(info_map["st_size"].c_str()));
        stat->set_blocks(atoll(info_map["st_blocks"].c_str()));
        if (info_map["st_ifmt"] == "S_IFDIR") {
          stat->set_mode(S_IFDIR | 0777);
          stat->set_nlink(2);
        } else if (info_map["st_ifmt"] == "S_IFREG") {
          stat->set_mode(S_IFREG | 0666);
          stat->set_nlink(2);
        } else if (info_map["st_ifmt"] == "S_IFSOCK") {
          stat->set_mode(S_IFSOCK | 0400);
        } else if (info_map["st_ifchr"] == "S_IFCHR") {
          stat->set_mode(S_IFCHR | 0400);
        } else {
          rpc->SetFailed("AFCFileInfoOpen: Unknown s_ifmt value");
        }
      }
    }
    if (rpc->Failed()) {
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
    if (rpc->Failed()) {
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
    }
    done->Run();
  }

  void Open(RpcController* rpc,
            const proto::OpenRequest* request,
            proto::OpenResponse* response,
            Closure* done) {
    int mode = ((request->flags() & 1) ? 3 : 2);
    afc_file_ref fd;
    int ret = AFCFileRefOpen(conn_, request->path().c_str(), mode, &fd);
    if (ret != MDERR_OK) {
      rpc->SetFailed("AFCFileRefOpen failed");
      LOG_FAILURE(rpc);
    } else {
      response->set_filehandle(fd);
      LOG_SUCCESS(request, response);
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
      LOG_FAILURE(rpc);
    } else {
      response->set_filehandle(fd);
      LOG_SUCCESS(request, response);
    }
    done->Run();
  }

  void Release(RpcController* rpc,
               const proto::ReleaseRequest* request,
               proto::ReleaseResponse* response,
               Closure* done) {
    AFCFileRefClose(conn_, request->filehandle());
    LOG_SUCCESS(request, response);
    done->Run();
  }

  void Read(RpcController* rpc,
            const proto::ReadRequest* request,
            proto::ReadResponse* response,
            Closure* done) {
    if (request->size() > kMaxBufferSize) {
      rpc->SetFailed("Read request too large");
      LOG_FAILURE(rpc);
      done->Run();
      return;
    }
    int ret = AFCFileRefSeek(conn_, request->filehandle(), request->offset(), 0);
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
    if (rpc->Failed()) {
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
    }
    done->Run();
  }

  void Write(RpcController* rpc,
             const proto::WriteRequest* request,
             proto::WriteResponse* response,
             Closure* done) {
    if (request->buffer().size() > 0) {
      int ret = AFCFileRefSeek(conn_, request->filehandle(), request->offset(), 
                               0);
      if (ret != MDERR_OK) {
        rpc->SetFailed("AFCFileRefSeek failed");
      } else {
        ret = AFCFileRefWrite(conn_, request->filehandle(),
                              request->buffer().data(), request->buffer().size());
        if (ret != MDERR_OK) {
          rpc->SetFailed("AFCFileWrite failed");
        }
      }
      if (rpc->Failed()) {
        LOG_FAILURE(rpc);
      } else {
        LOG_SUCCESS(request, response);
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
    if (rpc->Failed()) {
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
        proto::StatFsResponse::Stat* stat = response->mutable_stat();
        int block_size = atoi(info_map["FSBlockSize"].c_str());
        unsigned long long total_bytes = atoll(info_map["FSTotalBytes"].c_str());
        unsigned long long free_bytes = atoll(info_map["FSFreeBytes"].c_str());
        stat->set_bsize(block_size);
        stat->set_frsize(block_size);
        stat->set_blocks(total_bytes / block_size);
        stat->set_bfree(free_bytes / block_size);
      }
    }
    if (rpc->Failed()) {
      LOG_FAILURE(rpc);
    } else {
      LOG_SUCCESS(request, response);
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
