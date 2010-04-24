// Author: Allen Porter <allen@thebends.org>

#include "test/loopback_fs_service.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/attr.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "proto/fs_service.pb.h"
#include "rpc/mach_service.h"

using ::google::protobuf::Closure;
using ::google::protobuf::RpcController;
using ::rpc::ExportService;

namespace test {

static const int kMaxBufferSize = 1024 * 1024;

class LoopbackService : public proto::FsService {
 public:
  void GetAttr(RpcController* rpc,
               const proto::GetAttrRequest* request,
               proto::GetAttrResponse* response,
               Closure* done) {
    struct stat stbuf;
    int res = stat(request->path().c_str(), &stbuf);
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->mutable_stat()->set_size(stbuf.st_size);
      response->mutable_stat()->set_blocks(stbuf.st_blocks);
      response->mutable_stat()->set_mode(stbuf.st_mode);
      response->mutable_stat()->set_nlink(stbuf.st_nlink);
    }
    done->Run();
  }

  void ReadDir(RpcController* rpc,
               const proto::ReadDirRequest* request,
               proto::ReadDirResponse* response,
               Closure* done) {
    DIR* d = opendir(request->path().c_str());
    if (d == NULL) {
      rpc->SetFailed(strerror(errno));
    } else {
      struct dirent* dp;
      while ((dp = readdir(d)) != NULL) {
        response->add_entry()->set_filename(dp->d_name);
      }
      closedir(d);
    }
    done->Run();
  }

  void Unlink(RpcController* rpc,
              const proto::UnlinkRequest* request,
              proto::UnlinkResponse* response,
              Closure* done) {
    int res = unlink(request->path().c_str());
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    }
    done->Run();
  }

  void MkDir(RpcController* rpc,
             const proto::MkDirRequest* request,
             proto::MkDirResponse* response,
             Closure* done) {
    int res = mkdir(request->path().c_str(), request->mode());
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    }
    done->Run();
  }

  void Rename(RpcController* rpc,
              const proto::RenameRequest* request,
              proto::RenameResponse* response,
              Closure* done) {
    int res = rename(request->source_path().c_str(),
                     request->destination_path().c_str());
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    }
    done->Run();
  }

  void Open(RpcController* rpc,
            const proto::OpenRequest* request,
            proto::OpenResponse* response,
            Closure* done) {
    int fd = open(request->path().c_str(), request->flags());
    if (fd == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->set_filehandle(fd);
    }
    done->Run();
  }

  void Create(RpcController* rpc,
              const proto::CreateRequest* request,
              proto::CreateResponse* response,
              Closure* done) {
    int fd = open(request->path().c_str(), request->flags(), request->mode());
    if (fd == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->set_filehandle(fd);
    }
    done->Run();
  }

  void Release(RpcController* rpc,
               const proto::ReleaseRequest* request,
               proto::ReleaseResponse* response,
               Closure* done) {
    close(request->filehandle());
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
    void* buf = malloc(request->size());
    ssize_t n = pread(request->filehandle(), buf, request->size(),
                        request->offset());
    if (n == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->mutable_buffer()->assign((char*)buf, n);
    }
    free(buf);
    done->Run();
  }

  void Write(RpcController* rpc,
             const proto::WriteRequest* request,
             proto::WriteResponse* response,
             Closure* done) {
    ssize_t n = pwrite(request->filehandle(), request->buffer().data(),
                       request->buffer().size(), request->offset());
    if (n == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->set_size(n);
    }
    done->Run();
  }

  void Truncate(RpcController* rpc,
                const proto::TruncateRequest* request,
                proto::TruncateResponse* response,
                Closure* done) {
    int res = truncate(request->path().c_str(), request->offset());
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    }
    done->Run();
  }

  void StatFs(RpcController* rpc,
              const proto::StatFsRequest* request,
              proto::StatFsResponse* response,
              Closure* done) {
    struct statvfs stbuf;
    int res = statvfs("/", &stbuf);
    if (res == -1) {
      rpc->SetFailed(strerror(errno));
    } else {
      response->mutable_stat()->set_bsize(stbuf.f_bsize);
      response->mutable_stat()->set_frsize(stbuf.f_frsize);
      response->mutable_stat()->set_blocks(stbuf.f_blocks);
      response->mutable_stat()->set_bfree(stbuf.f_bfree);
    }
    done->Run();
  }
};


proto::FsService* NewLoopbackService() {
  return new LoopbackService();
}

}  // namespace test
