#include "file.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <string>
#include "page.h"

#define DBFILENAME "test.db"

class FileTest : public testing::Test {
 protected:
  // You can define per-test set-up logic as usual.
  void SetUp() override {
    dmgr = new DiskManager();
    fd = dmgr->openDatabase(path);
    ASSERT_TRUE(fd > 0);
  }

  // You can define per-test tear-down logic as usual.
  void TearDown() override {
    fd = -1;
    delete dmgr;
    remove(path);
  }

  // Some expensive resource shared by all tests.
  static PageManager *dmgr;
  static const char  *path;
  static int          fd;
};

PageManager *FileTest::dmgr = nullptr;
const char  *FileTest::path = "test.db";
int          FileTest::fd   = -1;

TEST_F(FileTest, fileOpenDatabase) {
  off_t file_size = lseek(fd, 0, SEEK_END);
  ASSERT_EQ(file_size, INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(FileTest, fileReadPage) {
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();
  dmgr->readPage(fd, PN_HEADER, &pg);
  ASSERT_EQ(phpg->magic_number, phpg->MAGIC_NUMBER);
}

TEST_F(FileTest, fileReadPage2) {
  {
    fd = -1;
    delete dmgr;
    dmgr = new DiskManager();
    fd = dmgr->openDatabase(path);
    ASSERT_TRUE(fd > 0);
  }
  {
    Page pg;
    HeaderPage *phpg = pg.getHeaderPage();
    dmgr->readPage(fd, PN_HEADER, &pg);
    ASSERT_EQ(phpg->magic_number, phpg->MAGIC_NUMBER);
  }
}

TEST_F(FileTest, fileAllocPage) {
  pagenum_t alloc_page_number;
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();

  for (int i = 1; i < INITIAL_PAGES_NUMBER; i++) {
    alloc_page_number = dmgr->allocPage(fd);
    ASSERT_EQ(alloc_page_number, i);
  }
  dmgr->readPage(fd, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(fd, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);

  alloc_page_number = dmgr->allocPage(fd);
  ASSERT_EQ(alloc_page_number, INITIAL_PAGES_NUMBER);
  dmgr->readPage(fd, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, INITIAL_PAGES_NUMBER + 1);
  ASSERT_EQ(lseek(fd, 0, SEEK_END), 2 * INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(FileTest, fileFreePage) {
  const int target_page_number = 2;

  pagenum_t alloc_page_number;
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();

  for (int i = 1; i < INITIAL_PAGES_NUMBER; i++) {
    alloc_page_number = dmgr->allocPage(fd);
    ASSERT_EQ(alloc_page_number, i);
  }
  dmgr->readPage(fd, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(fd, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);

  dmgr->freePage(fd, target_page_number);

  alloc_page_number = dmgr->allocPage(fd);
  ASSERT_EQ(alloc_page_number, target_page_number);
  dmgr->readPage(fd, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(fd, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(FileTest, stressTest) {
  const int nepoch = 10000;

  /*
   * Write pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = dmgr->allocPage(fd);
      std::string d = std::to_string(page_number);
      strncpy(pg.data, d.c_str(), d.size() + 1);
      dmgr->writePage(fd, page_number, &pg);
    }
  }

  /*
   * Read pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = static_cast<pagenum_t>(i);
      dmgr->readPage(fd, page_number, &pg);
      dmgr->freePage(fd, page_number);
      pagenum_t d = std::stoull(std::string(pg.data));
      ASSERT_EQ(page_number, d);
    }
  }
}

TEST_F(FileTest, restartTest) {
  const int nepoch = 1000;

  /*
   * Write pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = dmgr->allocPage(fd);
      std::string d = std::to_string(page_number);
      strncpy(pg.data, d.c_str(), d.size() + 1);
      dmgr->writePage(fd, page_number, &pg);
    }
  }

  /*
   * Restart DB
   */
  {
    delete dmgr;
    dmgr = new DiskManager();
    fd = dmgr->openDatabase(path);
    ASSERT_TRUE(fd > 0);
  }

  /*
   * Read pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = static_cast<pagenum_t>(i);
      dmgr->readPage(fd, page_number, &pg);
      dmgr->freePage(fd, page_number);
      pagenum_t d = std::stoull(std::string(pg.data));
      ASSERT_EQ(page_number, d);
    }
  }
}
