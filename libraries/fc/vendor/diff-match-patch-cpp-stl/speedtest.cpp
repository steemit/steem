// Copyright 2011 Google Inc.
// All Rights Reserved.
// Author: fraser@google.com

#include <fstream>
#include <iostream>
#include <string>
#include "diff_match_patch.h"

using namespace std;

wstring readFile(const char *filename) {
  wifstream file(filename);
  wstring text;
  getline(file, text, wstring::traits_type::to_char_type(wstring::traits_type::eof()));
  return text;
}

int main(int /* argc */, char ** /* argv */) {
  wstring text1 = readFile("speedtest1.txt");
  wstring text2 = readFile("speedtest2.txt");

  diff_match_patch<wstring> dmp;
  dmp.Diff_Timeout = 0.0;

  // Execute one reverse diff as a warmup.
  dmp.diff_main(text2, text1, false);

  clock_t t = clock();
  dmp.diff_main(text1, text2, false);
  cout << "Elapsed time: " << int((clock() - t) * 1000 / CLOCKS_PER_SEC) << " ms" << endl;
  return 0;
}
