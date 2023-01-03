#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "page.h"
#include "file.h"
#include "params.h"
#include <vector>
#include <unordered_map>

#define TID_INVALID (-1)

/**
 * PageManager Decorator
 * 
 * @example PageManager *dmgr = new DiskManager();
 *          PageManager *bmgr = new BufferManager(dmgr);
 */
class BufferManager : public PageManager {
 private:
  class BufferedPage {
   public:
    Page          frame;
    int           table_id;
    pagenum_t     page_number;
    bool          is_dirty;
    int           pins;
    BufferedPage *lru_prev;
    BufferedPage *lru_next;

   public:
    BufferedPage();
  };
  using HashTable = std::unordered_map<pagenum_t, BufferedPage *>;

 private:
  std::vector<HashTable *>  buffer_mapping;
  BufferedPage  *buffer_pool;
  PageManager   *dmgr;
  BufferedPage  *lru_head;
  BufferedPage  *lru_tail;
  uint64_t       size;
  uint64_t       capacity;

 private:
  HashTable    *__getBufferMapper(int table_id);
  BufferedPage *__findBufferedPage(int table_id, pagenum_t page_number);
  BufferedPage *__acquireBufferedPage(int table_id, pagenum_t page_number);
  void          __releaseBufferedPage(BufferedPage *pbpg);
  void          __lruLink(BufferedPage *pbpg);
  void          __lruUnlink(BufferedPage *pbpg);
  BufferedPage *__lruVictim();

 public:
  BufferManager() = delete;
  BufferManager(PageManager *dmgr);
  ~BufferManager() override;
  int       openDatabase(const std::string &path) override;
  pagenum_t allocPage(int table_id) override;
  void      freePage(int table_id, pagenum_t page_number) override;
  void      readPage(int table_id, pagenum_t page_number, Page *dest) override;
  void      writePage(int table_id, pagenum_t page_number, const Page *src) override;
};




#endif /* __BUFFER_H__ */