#include "file.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "optimize.h"
#include "page.h"

bool DiskManager::__fileExists(const std::string &path) {
  struct stat buf;
  return (stat(path.c_str(), &buf) == 0);
};

int DiskManager::__openExistingDatabaseFile(const std::string &path) {
  int fd = open(path.c_str(), O_RDWR);
  if (fd < 0) return fd;

  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();

  readPage(fd, PN_HEADER, &pg);
  if (phpg->magic_number != phpg->MAGIC_NUMBER) {
    close(fd);
    return F_VALIDATEFAIL;
  }

  return fd;
}

int DiskManager::__createDatabaseFile(const std::string &path) {
  int fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) return fd;

  if (ftruncate(fd, INITIAL_PAGES_NUMBER * PAGE_SIZE) < 0) {
    close(fd);
    return F_TRUNCATEFAIL;
  }
  fsync(fd);

  Page pg;

  /*
   * Initialize header page
   */
  HeaderPage *phpg = pg.getHeaderPage();
  phpg->magic_number = phpg->MAGIC_NUMBER;
  phpg->number_of_pages = INITIAL_PAGES_NUMBER;
  phpg->free_page_number = 1;
  writePage(fd, PN_HEADER, &pg);

  /*
   * Initialize free pages
   */
  phpg->magic_number = 0;
  phpg->number_of_pages = 0;
  phpg->free_page_number = 0;
  FreePage *pfpg = pg.getFreePage();
  for (pagenum_t i = 1; i < INITIAL_PAGES_NUMBER - 1; i++) {
    pfpg->next_free_page_number = i + 1;
    writePage(fd, i, &pg);
  }
  pfpg->next_free_page_number = PN_EOFREE;
  writePage(fd, INITIAL_PAGES_NUMBER - 1, &pg);

  return fd;
}

DiskManager::DiskManager() { fds.reserve(FDS_DEFAULT_CAPACITY); }

DiskManager::~DiskManager() {
  for (int fd : fds) {
    close(fd);
  }
}

/**
 * Open the database file
 *
 * @param path path of file
 * @return file descriptor or status
 */
int DiskManager::openDatabase(const std::string &path) {
  int fd = __fileExists(path) ? __openExistingDatabaseFile(path)
                              : __createDatabaseFile(path);
  fds.push_back(fd);
  return fd;
}

/**
 * Allocate a page
 *
 * @param fd file descriptor
 * @return page number of the allocated page
 *
 * @note If there wasn't any free page, it expands
 *       the database size doubly. It guarantees that
 *       it always returns an allocated page.
 */
pagenum_t DiskManager::allocPage(int fd) {
  Page hpg, fpg;
  HeaderPage *phpg = hpg.getHeaderPage();
  FreePage *pfpg = fpg.getFreePage();

  /*
   * Get a free page number
   */
  readPage(fd, PN_HEADER, &hpg);
  pagenum_t free_page_number = phpg->free_page_number;

  if (unlikely(free_page_number == PN_EOFREE)) {
    /*
     * Expand file and set the free page number
     */
    pagenum_t old_number_of_pages = phpg->number_of_pages;
    pagenum_t new_number_of_pages = 2 * old_number_of_pages;
    if (ftruncate(fd, new_number_of_pages * PAGE_SIZE) < 0) {
      return PN_INVALID;
    }
    fsync(fd);

    for (pagenum_t i = old_number_of_pages; i < new_number_of_pages - 1; i++) {
      pfpg->next_free_page_number = i + 1;
      writePage(fd, i, &fpg);
    }
    pfpg->next_free_page_number = PN_EOFREE;
    writePage(fd, new_number_of_pages - 1, &fpg);

    phpg->number_of_pages = new_number_of_pages;
    free_page_number = phpg->free_page_number = old_number_of_pages;
  }

  /*
   * Set header page
   */
  pagenum_t alloc_page_number = free_page_number;
  readPage(fd, free_page_number, &fpg);
  phpg->free_page_number = pfpg->next_free_page_number;
  writePage(fd, PN_HEADER, &hpg);

  return alloc_page_number;
}

/**
 * Deallocate a page
 *
 * @param fd          file discriptor
 * @param page_number page number to deallocate
 */
void DiskManager::freePage(int fd, pagenum_t page_number) {
  Page pg;

  /*
   * Set header page
   */
  HeaderPage *phpg = pg.getHeaderPage();
  readPage(fd, PN_HEADER, &pg);
  pagenum_t free_page_number = phpg->free_page_number;
  phpg->free_page_number = page_number;
  writePage(fd, PN_HEADER, &pg);

  /*
   * Free page
   */
  FreePage *pfpg = pg.getFreePage();
  pfpg->next_free_page_number = free_page_number;
  writePage(fd, page_number, &pg);
}

/**
 * Read a page from file
 *
 * @param fd          [in]  file discriptor
 * @param page_number [in]  page number to deallocate
 * @param dest        [out] destination address to read a page
 */
void DiskManager::readPage(int fd, pagenum_t page_number, Page *dest) {
  pread(fd, dest->data, PAGE_SIZE, page_number * PAGE_SIZE);
}

/**
 * Write a page from file
 *
 * @param fd          [in] file descriptor
 * @param page_number [in] page number to write
 * @param src         [in] source address to write a page
 */
void DiskManager::writePage(int fd, pagenum_t page_number, const Page *src) {
  pwrite(fd, src->data, PAGE_SIZE, page_number * PAGE_SIZE);
  fsync(fd);
}
