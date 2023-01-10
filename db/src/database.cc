#include "database.h"
#include "buffer.h"
#include "file.h"
#include "page.h"

DatabaseManager::DatabaseManager() {
  __dmgr = new DiskManager();
  bmgr = new BufferManager(__dmgr);
  /* TODO */
}

DatabaseManager::~DatabaseManager() {
  delete bmgr;
  delete __dmgr;
  /* TODO */
}

int DatabaseManager::openTable(const std::string &path) {
  /* TODO */
}

int DatabaseManager::insertDb(int table_id, const Data &data) {
  /* TODO */
}

DatabaseManager::Data DatabaseManager::findDb(int table_id, int64_t key) {
  /* TODO */
}

int DatabaseManager::deleteDb(int table_id, int64_t key) {
  /* TODO */
}

int DatabaseManager::scanDb(int table_id, int64_t begin_key, int64_t end_key,
                            std::vector<Data> &result) {
  /* TODO */
}