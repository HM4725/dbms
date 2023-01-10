#ifndef __DB_H__
#define __DB_H__

#include <cinttypes>
#include <string>
#include <tuple>
#include <vector>
#include "buffer.h"
#include "file.h"

class DatabaseManager {
 private:
  /**
   * Data type
   *
   * @param key        int64_t
   * @param value      std::string
   * @param value_size uint16_t
   */
  using Data = std::tuple<int64_t, std::string, uint16_t>;

 private:
  /**
   * Page managers
   * 
   * @warning Do not use __dmgr directly!
   * @note    Use bmgr. 
   */
  PageManager *__dmgr;
  PageManager *bmgr;

 public:
  DatabaseManager();  /* Init database */
  ~DatabaseManager(); /* Shutdown database */
  int  openTable(const std::string &path);
  int  insertDb(int table_id, const Data &data);
  Data findDb(int table_id, int64_t key);
  int  deleteDb(int table_id, int64_t key);
  int  scanDb(int table_id, int64_t begin_key, int64_t end_key,
              std::vector<Data> &result);
};

#endif /* __DB_H__ */