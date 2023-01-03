#include "db.h"
#include <string>
#include <vector>
#include "file.h"

int64_t openTable(const std::string &pathname) {
  return static_cast<int64_t>(fileOpenDatabaseFile(pathname));
}

int dbInsert(int64_t table_id, int64_t key, const std::string &value,
             uint16_t val_size) {

}

int dbFind(int64_t table_id, int64_t key, std::string &ret_val,
           uint16_t &val_size) {

}

int dbDelete(int64_t table_id, int64_t key) {
  return 0;
}
int dbScan(int64_t table_id, int64_t begin_key, int64_t end_key,
           std::vector<int64_t> &keys, std::vector<std::string> &values,
           std::vector<uint16_t> &val_sizes) {
  return 0;
}
int initDb() {
  return 0;
}
int shutdownDb() {
  fileCloseDatabaseFile();
  return 0;
}