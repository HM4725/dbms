#ifndef __FILE_H__
#define __FILE_H__

#include <cinttypes>
#include <string>
#include <vector>
#include "page.h"

#define F_SUCCESS      (1)
#define F_OPENFAIL     (-1)
#define F_CREATEFAIL   (-2)
#define F_TRUNCATEFAIL (-3)
#define F_VALIDATEFAIL (-4)

class PageManager {
 public:
  virtual ~PageManager() = default;
  virtual int       openDatabase(const std::string &path) = 0;
  virtual pagenum_t allocPage(int fd) = 0;
  virtual void      freePage(int fd, pagenum_t page_number) = 0;
  virtual void      readPage(int fd, pagenum_t page_number, Page *dest) = 0;
  virtual void      writePage(int fd, pagenum_t page_number, const Page *src) = 0;
};

class DiskManager : public PageManager {
 private:
  static const int FDS_DEFAULT_CAPACITY = 10;

 private:
  std::vector<int> fds;

 private:
  bool __fileExists(const std::string &path);
  int  __openExistingDatabaseFile(const std::string &path);
  int  __createDatabaseFile(const std::string &path);

 public:
  DiskManager();
  ~DiskManager() override;
  int       openDatabase(const std::string &path) override;
  pagenum_t allocPage(int fd) override;
  void      freePage(int fd, pagenum_t page_number) override;
  void      readPage(int fd, pagenum_t page_number, Page *dest) override;
  void      writePage(int fd, pagenum_t page_number, const Page *src) override;
};

#endif /* __FILE_H__ */