#include "buffer.h"
#include <unistd.h>
#include <cassert>
#include "file.h"
#include "optimize.h"
#include "page.h"

BufferManager::BufferedPage::BufferedPage()
    : table_id(TID_INVALID),
      page_number(PN_INVALID),
      is_dirty(false),
      pins(0),
      lru_prev(nullptr),
      lru_next(nullptr) {}

BufferManager::BufferManager(PageManager *dmgr) {
  assert(dmgr != nullptr);
  buffer_pool = new BufferedPage[BUFFER_SIZE];
  this->dmgr = dmgr;

  BufferedPage *alpha = &buffer_pool[0];
  alpha->table_id = 0;
  alpha->page_number = PN_INVALID;
  alpha->is_dirty = false;
  alpha->pins = 0;
  alpha->lru_prev = nullptr;
  alpha->lru_next = nullptr;
  HashTable *ht = __getBufferMapper(0);
  ht->insert(std::make_pair(PN_INVALID, alpha));

  lru_head = alpha;
  lru_tail = alpha;
  size = 1;
  capacity = BUFFER_SIZE;
}

BufferManager::~BufferManager() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    BufferedPage *pbpg = &buffer_pool[i];
    if (pbpg->is_dirty) {
      dmgr->writePage(pbpg->table_id, pbpg->page_number, &pbpg->frame);
    }
  }
  for (HashTable *ht : buffer_mapping) {
    if (ht) delete ht;
  }
  delete buffer_pool;
}

BufferManager::HashTable *BufferManager::__getBufferMapper(int table_id) {
  if (unlikely(buffer_mapping.size() <= table_id)) {
    buffer_mapping.resize(table_id + 1, nullptr);
    buffer_mapping[table_id] = new HashTable();
  }
  return buffer_mapping[table_id];
}

BufferManager::BufferedPage *BufferManager::__findBufferedPage(
    int table_id, pagenum_t page_number) {
  HashTable *ht = __getBufferMapper(table_id);
  const auto &value = ht->find(page_number);
  return value != ht->end() ? value->second : nullptr;
}

BufferManager::BufferedPage *BufferManager::__acquireBufferedPage(
    int table_id, pagenum_t page_number) {
  BufferedPage *pbpg = __findBufferedPage(table_id, page_number);
  if (pbpg == nullptr) {
    if (size < capacity) {
      pbpg = &buffer_pool[size++];
    } else {
      pbpg = __lruVictim();
      __lruUnlink(pbpg);
      HashTable *ht = __getBufferMapper(pbpg->table_id);
      ht->erase(pbpg->page_number);
      if (pbpg->is_dirty) {
        dmgr->writePage(pbpg->table_id, pbpg->page_number, &pbpg->frame);
      }
    }
    dmgr->readPage(table_id, page_number, &pbpg->frame);
    pbpg->table_id = table_id;
    pbpg->page_number = page_number;
    pbpg->is_dirty = false;
    pbpg->pins = 0;
    HashTable *ht = __getBufferMapper(table_id);
    ht->insert(std::make_pair(page_number, pbpg));
  } else {
    __lruUnlink(pbpg);
  }

  pbpg->pins += 1;
  __lruLink(pbpg);
  return pbpg;
}

void BufferManager::__releaseBufferedPage(BufferedPage *pbpg) {
  pbpg->pins -= 1;
}

/**
 * Link a buffered page to LRU cache
 *
 * @param pbpg Buffered page to link
 * @note  It appends the page to its head, because
 *        the cache replacement policy is LRU
 *        (Least Recently Used). Head means that
 *        the page is most-recently used. However,
 *        Tail means that the page is least-recently
 *        used.
 */
void BufferManager::__lruLink(BufferedPage *pbpg) {
  pbpg->lru_prev = nullptr;
  pbpg->lru_next = lru_head;
  lru_head->lru_prev = pbpg;
  lru_head = pbpg;
}

/**
 * Unlink a buffered page from LRU cache
 *
 * @warning Buffer size must be at least 2,
 *          when it is called.
 * @param   pbpg Buffered page to unlink
 *
 * @example     head                     tail
 *          X-"[page0]"-[page1]-[page2]-[page3]-X
 *                       head            tail
 *      =>  X-----------[page1]-[page2]-[page3]-X
 *
 * @example    head                      tail
 *          X-[page0]-"[page1]"-[page2]-[page3]-X
 *             head                      tail
 *      =>  X-[page0]-----------[page2]-[page3]-X
 *
 * @example    head                     tail
 *          X-[page0]-[page1]-[page2]-"[page3]"-X
 *             head            tail
 *      =>  X-[page0]-[page1]-[page2]-----------X
 */
void BufferManager::__lruUnlink(BufferedPage *pbpg) {
  assert(size > 1);

  BufferedPage *prev = pbpg->lru_prev;
  BufferedPage *next = pbpg->lru_next;

  if (unlikely(prev == nullptr)) {
    /* lru_head == pbpg */
    lru_head = next;
  } else {
    prev->lru_next = next;
  }

  if (likely(next == nullptr)) {
    /* lru_tail == pbpg */
    lru_tail = prev;
  } else {
    next->lru_prev = prev;
  }

  pbpg->lru_prev = nullptr;
  pbpg->lru_next = nullptr;
}

/**
 * Get the victim to drop from LRU cache
 *
 * @return Buffered page to drop | nullptr
 * @note   Cache replacement policy: LRU
 */
BufferManager::BufferedPage *BufferManager::__lruVictim() {
  BufferedPage *pbpg = lru_tail;
  while (unlikely(pbpg->pins > 0)) {
    pbpg = pbpg->lru_prev;
    if (unlikely(pbpg == nullptr)) {
      break;
    }
  }
  return pbpg;
}

int BufferManager::openDatabase(const std::string &path) {
  int table_id = dmgr->openDatabase(path);
  {
    Page pg;
    readPage(table_id, PN_HEADER, &pg);
  }
  return table_id;
}

pagenum_t BufferManager::allocPage(int table_id) {
  Page hpg, fpg;
  HeaderPage *phpg = hpg.getHeaderPage();
  FreePage *pfpg = fpg.getFreePage();

  /*
   * Get a free page number
   */
  readPage(table_id, PN_HEADER, &hpg);
  pagenum_t free_page_number = phpg->free_page_number;

  if (unlikely(free_page_number == PN_EOFREE)) {
    /*
     * Expand file and set the free page number.
     * Eventhough it is buffered API,
     * it expands the disk space.
     */
    pagenum_t old_number_of_pages = phpg->number_of_pages;
    pagenum_t new_number_of_pages = 2 * old_number_of_pages;
    if (ftruncate(table_id, new_number_of_pages * PAGE_SIZE) < 0) {
      return PN_INVALID;
    }
    fsync(table_id);

    for (pagenum_t i = old_number_of_pages; i < new_number_of_pages - 1; i++) {
      pfpg->next_free_page_number = i + 1;
      writePage(table_id, i, &fpg);
    }
    pfpg->next_free_page_number = PN_EOFREE;
    writePage(table_id, new_number_of_pages - 1, &fpg);

    phpg->number_of_pages = new_number_of_pages;
    free_page_number = phpg->free_page_number = old_number_of_pages;
  }

  /*
   * Set header page
   */
  pagenum_t alloc_page_number = free_page_number;
  readPage(table_id, free_page_number, &fpg);
  phpg->free_page_number = pfpg->next_free_page_number;
  writePage(table_id, PN_HEADER, &hpg);

  return alloc_page_number;
}

void BufferManager::freePage(int table_id, pagenum_t page_number) {
  Page pg;

  /*
   * Set header page
   */
  HeaderPage *phpg = pg.getHeaderPage();
  readPage(table_id, PN_HEADER, &pg);
  pagenum_t free_page_number = phpg->free_page_number;
  phpg->free_page_number = page_number;
  writePage(table_id, PN_HEADER, &pg);

  /*
   * Free page
   */
  FreePage *pfpg = pg.getFreePage();
  pfpg->next_free_page_number = free_page_number;
  writePage(table_id, page_number, &pg);
}

void BufferManager::readPage(int table_id, pagenum_t page_number, Page *dest) {
  BufferedPage *pbpg = __acquireBufferedPage(table_id, page_number);
  memcpy(dest, &pbpg->frame.data, PAGE_SIZE);
  __releaseBufferedPage(pbpg);
}

void BufferManager::writePage(int table_id, pagenum_t page_number,
                              const Page *src) {
  BufferedPage *pbpg = __acquireBufferedPage(table_id, page_number);
  pbpg->is_dirty = true;
  memcpy(&pbpg->frame.data, src, PAGE_SIZE);
  __releaseBufferedPage(pbpg);
}
