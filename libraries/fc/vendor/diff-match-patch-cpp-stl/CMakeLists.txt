# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

message("***   Build the test harness for C++ STL variants of Diff Match and Patch ***")
message("***   http://code.google.com/p/google-diff-match-patch/ and               ***")
message("***   https://github.com/leutloff/diff-match-patch-cpp-stl                ***")

project (diff-match-patch-cpp-stl CXX)

add_executable(diff_match_patch_test_wstring diff_match_patch_test_wstring.cpp)

add_executable(diff_match_patch_test_string diff_match_patch_test_string.cpp)
