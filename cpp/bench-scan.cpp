#include <iostream>

#include <stxxl/vector>

using vector_type = stxxl::vector<uint64_t>;

int main(int argc, char *argv[]) {
  size_t no_iterations = 1;
  size_t max_exp = 30;

  if (argc >= 2)
    max_exp = atoi(argv[1]); // in MiB
  if (argc >= 3)
    no_iterations = atoi(argv[2]);

  std::cout << "Iter,POT,TimeInit,TimeWrite,TimeRead,TimeRead2,BWWrite,BWRead,BWRead2\n";

  uint64_t sum = 0;
  for (size_t iter = 0; iter < no_iterations; ++iter) {
    for (size_t exp = 20; exp < max_exp; exp += 2) {
      const auto size_bytes = (1llu << exp);
      const auto size_elemns =
          size_bytes / sizeof(typename vector_type::value_type);
      foxxll::timer timer;

      // init
      timer.start();
      vector_type vec(size_elemns);
      const auto time_init = timer.mseconds();

      // write
      {
        timer.start();
        vector_type::bufwriter_type writer(vec);
        for (uint64_t i = 0; i < size_elemns; ++i)
          writer << (i % 1024);
        writer.finish();
      }
      const auto time_write = timer.mseconds();

      // read
      {
        timer.start();
        for (vector_type::bufreader_type reader(vec); !reader.empty(); ++reader)
          sum += *reader;
      }
      const auto time_read = timer.mseconds();

      // read again
      {
        timer.start();
        for (vector_type::bufreader_type reader(vec); !reader.empty(); ++reader)
          sum += *reader;
      }
      const auto time_read2 = timer.mseconds();

      const auto bw_write = (size_bytes / time_write) / 1000.0; // MB/s
      const auto bw_read = (size_bytes / time_read) / 1000.0;   // MB/s
      const auto bw_read2 = (size_bytes / time_read2) / 1000.0; // MB/s

      std::cout << iter << ","       //
                << exp << ","        //
                << time_init << ","  //
                << time_write << "," //
                << time_read << ","  //
                << time_read2 << "," //
                << bw_write << ","   //
                << bw_read << ","    //
                << bw_read2 << std::endl;
    }
  }

  return sum == 0x1231324334232llu; // make sure sum is not optimized away
}
