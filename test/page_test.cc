#include <gtest/gtest.h>
#include "page.h"

TEST(PageTest, page) {
  bool debug = true;
  Page pg;
  HeaderPage hpg(debug);
  FreePage fpg(debug);
  AllocPage apg(debug);

  ASSERT_EQ(sizeof(pg), PAGE_SIZE);
  ASSERT_EQ(sizeof(hpg), PAGE_SIZE);
  ASSERT_EQ(sizeof(fpg), PAGE_SIZE);
  ASSERT_EQ(sizeof(apg), PAGE_SIZE);
  ASSERT_EQ(sizeof(*pg.getHeaderPage()), PAGE_SIZE);
  ASSERT_EQ(sizeof(*pg.getFreePage()), PAGE_SIZE);
  ASSERT_EQ(sizeof(*pg.getAllocPage()), PAGE_SIZE);
}

TEST(PageTest, castingPage) {
  Page pg;
  HeaderPage *phpg = pg.getHeaderPage();
  char *check = pg.data;
  phpg->magic_number = static_cast<uint64_t>(-1);
  phpg->free_page_number = 0;
  phpg->number_of_pages = static_cast<pagenum_t>(-1);

  int i = 0;
  int end_magic_number = sizeof(uint64_t) / sizeof(char);
  int end_free_page_number =
      end_magic_number + sizeof(pagenum_t) / sizeof(char);
  int end_number_of_pages =
      end_free_page_number + sizeof(pagenum_t) / sizeof(char);
  while (i < end_magic_number) {
    ASSERT_EQ(check[i], -1);
    i += 1;
  }
  while (i < end_free_page_number) {
    ASSERT_EQ(check[i], 0);
    i += 1;
  }
  while (i < end_number_of_pages) {
    ASSERT_EQ(check[i], -1);
    i += 1;
  }
}

