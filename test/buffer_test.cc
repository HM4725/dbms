#include "buffer.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <string>
#include "file.h"
#include "page.h"

#define DBFILENAME "test.db"

class BufferTest : public testing::Test {
 protected:
  // You can define per-test set-up logic as usual.
  void SetUp() override {
    dmgr = new DiskManager();
    bmgr = new BufferManager(dmgr);
    ASSERT_TRUE(dmgr != nullptr);
    ASSERT_TRUE(bmgr != nullptr);
    table_id = bmgr->openDatabase(path);
    ASSERT_TRUE(table_id > 0);
  }

  // You can define per-test tear-down logic as usual.
  void TearDown() override {
    delete bmgr;
    delete dmgr;
    remove(path);
  }

  // Some expensive resource shared by all tests.
  static PageManager *dmgr;
  static PageManager *bmgr;
  static const char *path;
  static int table_id;
};
PageManager   *BufferTest::dmgr     = nullptr;
PageManager   *BufferTest::bmgr     = nullptr;
const char    *BufferTest::path     = "test.db";
int            BufferTest::table_id = -1;

TEST_F(BufferTest, bufferOpenDatabase) {
  off_t file_size = lseek(table_id, 0, SEEK_END);
  ASSERT_EQ(file_size, INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(BufferTest, bufferReadPage) {
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();
  bmgr->readPage(table_id, PN_HEADER, &pg);
  ASSERT_EQ(phpg->magic_number, phpg->MAGIC_NUMBER);
}

TEST_F(BufferTest, bufferReadPage2) {
  {
    table_id = -1;
    delete bmgr;
    delete dmgr;
    dmgr = new DiskManager();
    bmgr = new BufferManager(dmgr);
    table_id = bmgr->openDatabase(path);
    ASSERT_TRUE(table_id > 0);
  }
  {
    Page pg;
    HeaderPage *phpg = pg.getHeaderPage();
    bmgr->readPage(table_id, PN_HEADER, &pg);
    ASSERT_EQ(phpg->magic_number, phpg->MAGIC_NUMBER);
  }
}

TEST_F(BufferTest, bufferAllocPage) {
  pagenum_t alloc_page_number;
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();

  for (int i = 1; i < INITIAL_PAGES_NUMBER; i++) {
    alloc_page_number = bmgr->allocPage(table_id);
    ASSERT_EQ(alloc_page_number, i);
  }
  bmgr->readPage(table_id, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(table_id, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);

  alloc_page_number = bmgr->allocPage(table_id);
  ASSERT_EQ(alloc_page_number, INITIAL_PAGES_NUMBER);
  bmgr->readPage(table_id, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, INITIAL_PAGES_NUMBER + 1);
  ASSERT_EQ(lseek(table_id, 0, SEEK_END), 2 * INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(BufferTest, bufferFreePage) {
  const int target_page_number = 2;

  pagenum_t alloc_page_number;
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();

  for (int i = 1; i < INITIAL_PAGES_NUMBER; i++) {
    alloc_page_number = bmgr->allocPage(table_id);
    ASSERT_EQ(alloc_page_number, i);
  }
  bmgr->readPage(table_id, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(table_id, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);

  bmgr->freePage(table_id, target_page_number);

  alloc_page_number = bmgr->allocPage(table_id);
  ASSERT_EQ(alloc_page_number, target_page_number);
  bmgr->readPage(table_id, PN_HEADER, &pg);
  ASSERT_EQ(phpg->free_page_number, PN_EOFREE);
  ASSERT_EQ(lseek(table_id, 0, SEEK_END), INITIAL_PAGES_NUMBER * PAGE_SIZE);
}

TEST_F(BufferTest, stressTest) {
  const int nepoch = 10000;

  /*
   * Write pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = bmgr->allocPage(table_id);
      std::string d = std::to_string(page_number);
      strncpy(pg.data, d.c_str(), d.size() + 1);
      bmgr->writePage(table_id, page_number, &pg);
    }
  }

  /*
   * Read pages
   */
  {
    Page pg;

    for(int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = static_cast<pagenum_t>(i);
      bmgr->readPage(table_id, page_number, &pg);
      bmgr->freePage(table_id, page_number);
      pagenum_t d = std::stoull(std::string(pg.data));
      ASSERT_EQ(page_number, d);
    }
  }
}

TEST_F(BufferTest, restartTest) {
  const int nepoch = 1000;

  /*
   * Write pages
   */
  {
    Page pg;

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = bmgr->allocPage(table_id);
      std::string d = std::to_string(page_number);
      strncpy(pg.data, d.c_str(), d.size() + 1);
      bmgr->writePage(table_id, page_number, &pg);
    }
  }

  /*
   * Restart DB
   */
  {
    delete bmgr;
    delete dmgr;
    dmgr = new DiskManager();
    bmgr = new BufferManager(dmgr);
    table_id = bmgr->openDatabase(path);
    ASSERT_TRUE(table_id > 0);
  }

  /*
   * Read pages
   */
  {
    Page pg;

    for(int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = static_cast<pagenum_t>(i);
      bmgr->readPage(table_id, page_number, &pg);
      bmgr->freePage(table_id, page_number);
      pagenum_t d = std::stoull(std::string(pg.data));
      ASSERT_EQ(page_number, d);
    }
  }
}