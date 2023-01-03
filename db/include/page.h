#ifndef __PAGE_H__
#define __PAGE_H__

#include <cinttypes>
#include "params.h"

#define PN_HEADER (0)
#define PN_INVALID (0)
#define PN_EOFREE (0)

typedef uint64_t pagenum_t;

/**
 * Supported types of pages
 */
class Page;
class HeaderPage;
class FreePage;
class AllocPage;

/**
 * Page
 */
class alignas(PAGE_SIZE) Page {
 public:
  char data[PAGE_SIZE];

 public:
  inline HeaderPage *getHeaderPage() {
    return reinterpret_cast<HeaderPage *>(this);
  }
  inline FreePage *getFreePage() {
    return reinterpret_cast<FreePage *>(this);
  }
  inline AllocPage *getAllocPage() {
    return reinterpret_cast<AllocPage *>(this);
  }
};

/**
 * Header page
 */
class alignas(PAGE_SIZE) HeaderPage {
 public:
  static constexpr uint64_t MAGIC_NUMBER = 0x12341234;

 public:
  uint64_t magic_number;
  pagenum_t free_page_number;
  pagenum_t number_of_pages;

 public:
  HeaderPage() = delete;
  HeaderPage(bool debug) {}
};

/**
 * Free page
 */
class alignas(PAGE_SIZE) FreePage {
 public:
  pagenum_t next_free_page_number;

 public:
  FreePage() = delete;
  FreePage(bool debug) {}
};

/**
 * Alloc page
 */
class alignas(PAGE_SIZE) AllocPage {
 public:
  char reserved[PAGE_SIZE];

 public:
  AllocPage() = delete;
  AllocPage(bool debug) {}
};

#endif /* __PAGE_H__ */