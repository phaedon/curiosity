#include <benchmark/benchmark.h>

#include "absl/container/flat_hash_map.h"
#include "absl/random/random.h"
#include <string>
#include <uuid/uuid.h>

std::string genUUID() {
  uuid_t uuid;
  char uuid_str[37]; // 36 characters + null terminator

  // Generate a UUID
  uuid_generate(uuid);

  // Convert UUID to a string
  uuid_unparse(uuid, uuid_str);
  return std::string(uuid_str);
}

absl::flat_hash_map<int, std::string> createMap(int num_elems, int max_range) {
  absl::BitGen gen;
  absl::flat_hash_map<int, std::string> uuids;
  for (size_t i = 0; i < num_elems; ++i) {
    int key = absl::Uniform(gen, 0, max_range);
    uuids[key] = genUUID();
  }
  return uuids;
}

static void BM_Contains(benchmark::State &state) {
  const int num_elems = 500000;
  const int max_range = num_elems * 20;
  const auto uuid_map = createMap(num_elems, max_range);
  int char_count = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < max_range; ++i) {
      if (uuid_map.contains(i)) {
        char_count += uuid_map.at(i).size();
      } else {
        --char_count;
      }
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_Contains);

static void BM_FindEnd(benchmark::State &state) {
  const int num_elems = 500000;
  const int max_range = num_elems * 20;
  const auto uuid_map = createMap(num_elems, max_range);
  int char_count = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < max_range; ++i) {
      const auto it = uuid_map.find(i);
      if (it == uuid_map.end()) {
        --char_count;
      } else {
        char_count += it->second.size();
      }
    }
  }
}
BENCHMARK(BM_FindEnd);

BENCHMARK_MAIN();