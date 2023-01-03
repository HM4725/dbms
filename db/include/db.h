#ifndef __DB_H__
#define __DB_H__

#include <cinttypes>
#include <string>
#include <vector>

extern int64_t openTable(const std::string &pathname);
extern int     dbInsert(int64_t table_id, int64_t key, const std::string &value,
                        uint16_t val_size);
extern int     dbFind(int64_t table_id, int64_t key, std::string &ret_val,
                      uint16_t &val_size);
extern int     dbDelete(int64_t table_id, int64_t key);
extern int     dbScan(int64_t table_id, int64_t begin_key, int64_t end_key,
                      std::vector<int64_t> &keys, std::vector<std::string> &values,
                      std::vector<uint16_t> &val_sizes);
extern int     initDb();
extern int     shutdownDb();

#endif /* __DB_H__ */