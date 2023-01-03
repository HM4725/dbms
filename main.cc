#include <cassert>
#include <cinttypes>
#include <iostream>
#include <unistd.h>
#include "file.h"
#include "buffer.h"
#include "page.h"

int main() {
  DiskManager dmgr;
  BufferManager bmgr(&dmgr);
  int table_id = bmgr.openDatabase("test.db");

  Page pg;
  bmgr.readPage(table_id, PN_HEADER, &pg);


  const int nepoch = 4;
  const char *scratch = "Hello World\n\0";

  /*
   * Write pages
   */
  {
    Page pg;
    strncpy(pg.data, scratch, 13);

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = bmgr.allocPage(table_id);
      bmgr.writePage(table_id, page_number, &pg);
    }
  }

  /*
   * Read pages
   */
  {
    Page pg;
    char buffer[20] = {0};

    for (int i = 1; i <= nepoch; i++) {
      pagenum_t page_number = static_cast<pagenum_t>(i);
      bmgr.readPage(table_id, page_number, &pg);
      bmgr.freePage(table_id, page_number);
      strncpy(buffer, pg.data, 13);
      for(int i = 0; i < 12; i++) {
        assert(buffer[i] == scratch[i]);
      }
    }

    remove("test.db");
    return 0;
  }


  return 0;
}
