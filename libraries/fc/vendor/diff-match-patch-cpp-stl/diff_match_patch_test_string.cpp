/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: fraser@google.com (Neil Fraser)
 * Author: mikeslemmer@gmail.com (Mike Slemmer)
 * Author: snhere@gmail.com (Sergey Nozhenko)
 * Author: leutloff@sundancer.oche.de (Christian Leutloff)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Diff Match and Patch -- Test Harness using std::string
 * http://code.google.com/p/google-diff-match-patch/
 */

#include "diff_match_patch.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

struct wastring : wstring  // The same as wstring, but can be constructed from char* (to use ASCII strings in the test functions)
{
  wastring() {}
  wastring(const wstring& s) : wstring(s) {}
  wastring(const value_type* s) : wstring(s) {}
  wastring(const value_type* s, size_t n) : wstring(s, n) {}
  wastring(const char* s) : wstring(s, s + strlen(s)) {}
  wastring(const char* s, size_t n) : wstring(s, s + n) {}
  wastring operator+=(value_type c) { append(1, c); return *this; }
  wastring operator+=(const wastring& s) { append(s); return *this; }
  wastring operator+=(const char* s) { append(s, s + strlen(s)); return *this; }

  static const wchar_t eol = L'\n';
  static const wchar_t tab = L'\t';
};

inline wastring operator+(const wastring& s, const char* p) { return s + wastring(p); }
inline wastring operator+(const char* p, const wastring& s) { return wastring(p) + s; }

ostream& operator<<(ostream& o, const wastring& s)
{
  o << setfill('0');
  for (wastring::const_pointer p = s.c_str(), end = p + s.length(); p != end; ++p)
  {
    if (*p >= 0x80)
      o << "\\u" << hex << setw(4) << unsigned(*p);
    else if (*p >= ' ')
      o << char(*p);
    else
      switch (char(*p)) {
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default: o << "\\x" << hex << setw(2) << unsigned(*p);
      }
  }
  return o;
}


#define dmp (*this)

class diff_match_patch_test : diff_match_patch<string> {
 typedef vector<string_t> Strings;
 typedef diff_match_patch_traits<char> traits;
 public:
  diff_match_patch_test() {}

  void run_all_tests() {
    clock_t t = clock();
    try {
      testDiffCommonPrefix();
      testDiffCommonSuffix();
      testDiffCommonOverlap();
      testDiffHalfmatch();
      testDiffLinesToChars();
      //TODO testDiffCharsToLines();
      testDiffCleanupMerge();
      testDiffCleanupSemanticLossless();
      testDiffCleanupSemantic();
      testDiffCleanupEfficiency();
      testDiffPrettyHtml();
      testDiffText();
      testDiffDelta();
      testDiffXIndex();
      testDiffLevenshtein();
      testDiffBisect();
      testDiffMain();

      testMatchAlphabet();
      testMatchBitap();
      testMatchMain();

      testPatchObj();
      testPatchFromText();
      testPatchToText();
      testPatchAddContext();
      testPatchMake();
      testPatchSplitMax();
      testPatchAddPadding();
      testPatchApply();

      cout << "All tests passed." << endl;
    } catch (const char* strCase) {
      cerr << "Test failed: " << strCase << endl;
    }
    cout << "Total time: " << int((clock() - t) * 1000 / CLOCKS_PER_SEC) << " ms" << endl;
  }


  //  DIFF TEST FUNCTIONS

  void testDiffCommonPrefix() {
    // Detect any common prefix.
    assertEquals("diff_commonPrefix: Null case.", 0, dmp.diff_commonPrefix("abc", "xyz"));

    assertEquals("diff_commonPrefix: Non-null case.", 4, dmp.diff_commonPrefix("1234abcdef", "1234xyz"));

    assertEquals("diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix("1234", "1234xyz"));
  }

  void testDiffCommonSuffix() {
    // Detect any common suffix.
    assertEquals("diff_commonSuffix: Null case.", 0, dmp.diff_commonSuffix("abc", "xyz"));

    assertEquals("diff_commonSuffix: Non-null case.", 4, dmp.diff_commonSuffix("abcdef1234", "xyz1234"));

    assertEquals("diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix("1234", "xyz1234"));
  }

  void testDiffCommonOverlap() {
    // Detect any suffix/prefix overlap.
    assertEquals("diff_commonOverlap: Null case.", 0, dmp.diff_commonOverlap("", "abcd"));

    assertEquals("diff_commonOverlap: Whole case.", 3, dmp.diff_commonOverlap("abc", "abcd"));

    assertEquals("diff_commonOverlap: No overlap.", 0, dmp.diff_commonOverlap("123456", "abcd"));

    assertEquals("diff_commonOverlap: Overlap.", 3, dmp.diff_commonOverlap("123456xxx", "xxxabcd"));

    // Some overly clever languages (C#) may treat ligatures as equal to their
    // component letters.  E.g. U+FB01 == 'fi'
    assertEquals("diff_commonOverlap: Unicode.", 0, dmp.diff_commonOverlap("fi", "\xEF\xAC\x81i"));
}

  void testDiffHalfmatch() {
    // Detect a halfmatch.
    dmp.Diff_Timeout = 1;
    assertEmpty("diff_halfMatch: No match #1.", diff_halfMatch("1234567890", "abcdef"));

    assertEmpty("diff_halfMatch: No match #2.", diff_halfMatch("12345", "23"));

    assertEquals("diff_halfMatch: Single Match #1.", split("12,90,a,z,345678", ','), diff_halfMatch("1234567890", "a345678z"));

    assertEquals("diff_halfMatch: Single Match #2.", split("a,z,12,90,345678", ','), diff_halfMatch("a345678z", "1234567890"));

    assertEquals("diff_halfMatch: Single Match #3.", split("abc,z,1234,0,56789", ','), diff_halfMatch("abc56789z", "1234567890"));

    assertEquals("diff_halfMatch: Single Match #4.", split("a,xyz,1,7890,23456", ','), diff_halfMatch("a23456xyz", "1234567890"));

    assertEquals("diff_halfMatch: Multiple Matches #1.", split("12123,123121,a,z,1234123451234", ','), diff_halfMatch("121231234123451234123121", "a1234123451234z"));

    assertEquals("diff_halfMatch: Multiple Matches #2.", split(",-=-=-=-=-=,x,,x-=-=-=-=-=-=-=", ','), diff_halfMatch("x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-="));

    assertEquals("diff_halfMatch: Multiple Matches #3.", split("-=-=-=-=-=,,,y,-=-=-=-=-=-=-=y", ','), diff_halfMatch("-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy"));

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertEquals("diff_halfMatch: Non-optimal halfmatch.", split("qHillo,w,x,Hulloy,HelloHe", ','), diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));

  //  dmp.Diff_Timeout = 0;
  //  assertEmpty("diff_halfMatch: Optimal no halfmatch.", diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));
  }

  void testDiffLinesToChars() {
    // Convert lines down to characters.
    Lines tmpVector, resVector;
    tmpVector.text1 = string_t("alpha\n");
    tmpVector.text2 = string_t("beta\n");
    tmpVector.resize(3);
    tmpVector[1] = LinePtr(tmpVector.text1.c_str(), tmpVector.text1.length());
    tmpVector[2] = LinePtr(tmpVector.text2.c_str(), tmpVector.text2.length());
    string_t text1("alpha\nbeta\nalpha\n"), text2("beta\nalpha\nbeta\n");
    dmp.diff_linesToChars(text1, text2, resVector);
    assertEquals("diff_linesToChars:", "\1\2\1", "\2\1\2", tmpVector, text1, text2, resVector);

    tmpVector.text1 = string_t("alpha\r\n");
    tmpVector.text2 = string_t("beta\r\n\r\n");
    tmpVector.resize(4);
    tmpVector[1] = LinePtr(tmpVector.text1.c_str(), tmpVector.text1.length());
    tmpVector[2] = LinePtr(tmpVector.text2.c_str(), tmpVector.text2.length() - 2);
    tmpVector[3] = LinePtr(tmpVector.text2.c_str() + 6, 2);
    text1.clear(), text2 = "alpha\r\nbeta\r\n\r\n\r\n";
    dmp.diff_linesToChars(text1, text2, resVector);
    assertEquals("diff_linesToChars:", string_t(), "\1\2\3\3", tmpVector, text1, text2, resVector);

    tmpVector.text1 = string_t("a");
    tmpVector.text2 = string_t("b");
    tmpVector.resize(3);
    tmpVector[1] = LinePtr(tmpVector.text1.c_str(), tmpVector.text1.length());
    tmpVector[2] = LinePtr(tmpVector.text2.c_str(), tmpVector.text2.length());
    text1 = "a", text2 = "b";
    resVector.clear();
    dmp.diff_linesToChars(text1, text2, resVector);
    assertEquals("diff_linesToChars:", "\1", "\2", tmpVector, text1, text2, resVector);

    // More than 256 to reveal any 8-bit limitations.
    size_t n = 300;
    tmpVector.resize(n + 1);
    tmpVector[0].second = 0;
    basic_stringstream<char_t> lines;
    string_t chars;
    for (size_t x = 1; x < n + 1; x++) {
      lines << x << traits::eol;
      tmpVector[x].second = lines.str().size();
      chars += (wchar_t)x;
    }
    tmpVector.text1 = lines.str();
    for (size_t x = 1, prev = 0; x < n + 1; x++) {
      tmpVector[x].first = tmpVector.text1.c_str() + prev;
      tmpVector[x].second -= prev;
      prev += tmpVector[x].second;
    }
    assertEquals("diff_linesToChars: More than 256 (setup).", n + 1, tmpVector.size());
    assertEquals("diff_linesToChars: More than 256 (setup).", n, chars.length());
    text1 = tmpVector.text1, text2.clear();
    resVector.clear();
    dmp.diff_linesToChars(text1, text2, resVector);
    assertEquals("diff_linesToChars:", chars, "", tmpVector, text1, text2, resVector);
  }

  void testDiffCharsToLines() {
    // First check that Diff equality works.
    assertTrue("diff_charsToLines:", Diff(EQUAL, "a") == Diff(EQUAL, "a"));

    assertEquals("diff_charsToLines:", Diff(EQUAL, "a"), Diff(EQUAL, "a"));

    // Convert chars up to lines.
    Diffs diffs;
    diffs.push_back(Diff(EQUAL, "\1\2\1"));  // ("\u0001\u0002\u0001");
    diffs.push_back(Diff(INSERT, "\2\1\2"));  // ("\u0002\u0001\u0002");
    Lines tmpVector;
    tmpVector.text1 = string_t("alpha\n");
    tmpVector.text2 = string_t("beta\n");
    tmpVector.resize(3);
    tmpVector[1] = LinePtr(tmpVector.text1.c_str(), tmpVector.text1.length());
    tmpVector[2] = LinePtr(tmpVector.text2.c_str(), tmpVector.text2.length());
    dmp.diff_charsToLines(diffs, tmpVector);
    assertEquals("diff_charsToLines:", diffList(Diff(EQUAL, "alpha\nbeta\nalpha\n"), Diff(INSERT, "beta\nalpha\nbeta\n")), diffs);

    // More than 256 to reveal any 8-bit limitations.
    size_t n = 300;
    tmpVector.resize(n + 1);
    tmpVector[0].second = 0;
    basic_stringstream<char_t> lines;
    string_t chars;
    for (size_t x = 1; x < n + 1; x++) {
      lines << x << traits::eol;
      tmpVector[x].second = lines.str().size();
      chars += (char_t)x;
    }
    tmpVector.text1 = lines.str();
    for (size_t x = 1, prev = 0; x < n + 1; x++) {
      tmpVector[x].first = tmpVector.text1.c_str() + prev;
      tmpVector[x].second -= prev;
      prev += tmpVector[x].second;
    }
    assertEquals("diff_linesToChars: More than 256 (setup).", n + 1, tmpVector.size());
    assertEquals("diff_linesToChars: More than 256 (setup).", n, chars.length());
    diffs = diffList(Diff(DELETE, chars));
    dmp.diff_charsToLines(diffs, tmpVector);
    assertEquals("diff_charsToLines: More than 256.", diffList(Diff(DELETE, tmpVector.text1)), diffs);
  }

  void testDiffCleanupMerge() {
    // Cleanup a messy diff.
    Diffs diffs;
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Null case.", diffList(), diffs);

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "b"), Diff(INSERT, "c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: No change case.", diffList(Diff(EQUAL, "a"), Diff(DELETE, "b"), Diff(INSERT, "c")), diffs);

    diffs = diffList(Diff(EQUAL, "a"), Diff(EQUAL, "b"), Diff(EQUAL, "c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge equalities.", diffList(Diff(EQUAL, "abc")), diffs);

    diffs = diffList(Diff(DELETE, "a"), Diff(DELETE, "b"), Diff(DELETE, "c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge deletions.", diffList(Diff(DELETE, "abc")), diffs);

    diffs = diffList(Diff(INSERT, "a"), Diff(INSERT, "b"), Diff(INSERT, "c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge insertions.", diffList(Diff(INSERT, "abc")), diffs);

    diffs = diffList(Diff(DELETE, "a"), Diff(INSERT, "b"), Diff(DELETE, "c"), Diff(INSERT, "d"), Diff(EQUAL, "e"), Diff(EQUAL, "f"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge interweave.", diffList(Diff(DELETE, "ac"), Diff(INSERT, "bd"), Diff(EQUAL, "ef")), diffs);

    diffs = diffList(Diff(DELETE, "a"), Diff(INSERT, "abc"), Diff(DELETE, "dc"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Prefix and suffix detection.", diffList(Diff(EQUAL, "a"), Diff(DELETE, "d"), Diff(INSERT, "b"), Diff(EQUAL, "c")), diffs);

    diffs = diffList(Diff(EQUAL, "x"), Diff(DELETE, "a"), Diff(INSERT, "abc"), Diff(DELETE, "dc"), Diff(EQUAL, "y"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Prefix and suffix detection with equalities.", diffList(Diff(EQUAL, "xa"), Diff(DELETE, "d"), Diff(INSERT, "b"), Diff(EQUAL, "cy")), diffs);

    diffs = diffList(Diff(EQUAL, "a"), Diff(INSERT, "ba"), Diff(EQUAL, "c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit left.", diffList(Diff(INSERT, "ab"), Diff(EQUAL, "ac")), diffs);

    diffs = diffList(Diff(EQUAL, "c"), Diff(INSERT, "ab"), Diff(EQUAL, "a"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit right.", diffList(Diff(EQUAL, "ca"), Diff(INSERT, "ba")), diffs);

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "b"), Diff(EQUAL, "c"), Diff(DELETE, "ac"), Diff(EQUAL, "x"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit left recursive.", diffList(Diff(DELETE, "abc"), Diff(EQUAL, "acx")), diffs);

    diffs = diffList(Diff(EQUAL, "x"), Diff(DELETE, "ca"), Diff(EQUAL, "c"), Diff(DELETE, "b"), Diff(EQUAL, "a"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit right recursive.", diffList(Diff(EQUAL, "xca"), Diff(DELETE, "cba")), diffs);
  }

  void testDiffCleanupSemanticLossless() {
    // Slide diffs to match logical boundaries.
    Diffs diffs = diffList();
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Null case.", diffList(), diffs);

    diffs = diffList(Diff(EQUAL, "AAA\r\n\r\nBBB"), Diff(INSERT, "\r\nDDD\r\n\r\nBBB"), Diff(EQUAL, "\r\nEEE"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Blank lines.", diffList(Diff(EQUAL, "AAA\r\n\r\n"), Diff(INSERT, "BBB\r\nDDD\r\n\r\n"), Diff(EQUAL, "BBB\r\nEEE")), diffs);

    diffs = diffList(Diff(EQUAL, "AAA\r\nBBB"), Diff(INSERT, " DDD\r\nBBB"), Diff(EQUAL, " EEE"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Line boundaries.", diffList(Diff(EQUAL, "AAA\r\n"), Diff(INSERT, "BBB DDD\r\n"), Diff(EQUAL, "BBB EEE")), diffs);

    diffs = diffList(Diff(EQUAL, "The c"), Diff(INSERT, "ow and the c"), Diff(EQUAL, "at."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Word boundaries.", diffList(Diff(EQUAL, "The "), Diff(INSERT, "cow and the "), Diff(EQUAL, "cat.")), diffs);

    diffs = diffList(Diff(EQUAL, "The-c"), Diff(INSERT, "ow-and-the-c"), Diff(EQUAL, "at."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Alphanumeric boundaries.", diffList(Diff(EQUAL, "The-"), Diff(INSERT, "cow-and-the-"), Diff(EQUAL, "cat.")), diffs);

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "a"), Diff(EQUAL, "ax"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Hitting the start.", diffList(Diff(DELETE, "a"), Diff(EQUAL, "aax")), diffs);

    diffs = diffList(Diff(EQUAL, "xa"), Diff(DELETE, "a"), Diff(EQUAL, "a"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Hitting the end.", diffList(Diff(EQUAL, "xaa"), Diff(DELETE, "a")), diffs);

    diffs = diffList(Diff(EQUAL, "The xxx. The "), Diff(INSERT, "zzz. The "), Diff(EQUAL, "yyy."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemantic: Sentence boundaries.", diffList(Diff(EQUAL, "The xxx."), Diff(INSERT, " The zzz."), Diff(EQUAL, " The yyy.")), diffs);
}

  void testDiffCleanupSemantic() {
    // Cleanup semantically trivial equalities.
    Diffs diffs = diffList();
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Null case.", diffList(), diffs);

    diffs = diffList(Diff(DELETE, "ab"), Diff(INSERT, "cd"), Diff(EQUAL, "12"), Diff(DELETE, "e"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No elimination #1.", diffList(Diff(DELETE, "ab"), Diff(INSERT, "cd"), Diff(EQUAL, "12"), Diff(DELETE, "e")), diffs);

    diffs = diffList(Diff(DELETE, "abc"), Diff(INSERT, "ABC"), Diff(EQUAL, "1234"), Diff(DELETE, "wxyz"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No elimination #2.", diffList(Diff(DELETE, "abc"), Diff(INSERT, "ABC"), Diff(EQUAL, "1234"), Diff(DELETE, "wxyz")), diffs);

    diffs = diffList(Diff(DELETE, "a"), Diff(EQUAL, "b"), Diff(DELETE, "c"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Simple elimination.", diffList(Diff(DELETE, "abc"), Diff(INSERT, "b")), diffs);

    diffs = diffList(Diff(DELETE, "ab"), Diff(EQUAL, "cd"), Diff(DELETE, "e"), Diff(EQUAL, "f"), Diff(INSERT, "g"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Backpass elimination.", diffList(Diff(DELETE, "abcdef"), Diff(INSERT, "cdfg")), diffs);

    diffs = diffList(Diff(INSERT, "1"), Diff(EQUAL, "A"), Diff(DELETE, "B"), Diff(INSERT, "2"), Diff(EQUAL, "_"), Diff(INSERT, "1"), Diff(EQUAL, "A"), Diff(DELETE, "B"), Diff(INSERT, "2"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Multiple elimination.", diffList(Diff(DELETE, "AB_AB"), Diff(INSERT, "1A2_1A2")), diffs);

    diffs = diffList(Diff(EQUAL, "The c"), Diff(DELETE, "ow and the c"), Diff(EQUAL, "at."));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Word boundaries.", diffList(Diff(EQUAL, "The "), Diff(DELETE, "cow and the "), Diff(EQUAL, "cat.")), diffs);

    diffs = diffList(Diff(DELETE, "abcxx"), Diff(INSERT, "xxdef"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No overlap elimination.", diffList(Diff(DELETE, "abcxx"), Diff(INSERT, "xxdef")), diffs);

    diffs = diffList(Diff(DELETE, "abcxxx"), Diff(INSERT, "xxxdef"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Overlap elimination.", diffList(Diff(DELETE, "abc"), Diff(EQUAL, "xxx"), Diff(INSERT, "def")), diffs);

    diffs = diffList(Diff(DELETE, "xxxabc"), Diff(INSERT, "defxxx"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Reverse overlap elimination.", diffList(Diff(INSERT, "def"), Diff(EQUAL, "xxx"), Diff(DELETE, "abc")), diffs);

    diffs = diffList(Diff(DELETE, "abcd1212"), Diff(INSERT, "1212efghi"), Diff(EQUAL, "----"), Diff(DELETE, "A3"), Diff(INSERT, "3BC"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Two overlap eliminations.", diffList(Diff(DELETE, "abcd"), Diff(EQUAL, "1212"), Diff(INSERT, "efghi"), Diff(EQUAL, "----"), Diff(DELETE, "A"), Diff(EQUAL, "3"), Diff(INSERT, "BC")), diffs);
  }

  void testDiffCleanupEfficiency() {
    // Cleanup operationally trivial equalities.
    dmp.Diff_EditCost = 4;
    Diffs diffs = diffList();
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Null case.", diffList(), diffs);

    diffs = diffList(Diff(DELETE, "ab"), Diff(INSERT, "12"), Diff(EQUAL, "wxyz"), Diff(DELETE, "cd"), Diff(INSERT, "34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: No elimination.", diffList(Diff(DELETE, "ab"), Diff(INSERT, "12"), Diff(EQUAL, "wxyz"), Diff(DELETE, "cd"), Diff(INSERT, "34")), diffs);

    diffs = diffList(Diff(DELETE, "ab"), Diff(INSERT, "12"), Diff(EQUAL, "xyz"), Diff(DELETE, "cd"), Diff(INSERT, "34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Four-edit elimination.", diffList(Diff(DELETE, "abxyzcd"), Diff(INSERT, "12xyz34")), diffs);

    diffs = diffList(Diff(INSERT, "12"), Diff(EQUAL, "x"), Diff(DELETE, "cd"), Diff(INSERT, "34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Three-edit elimination.", diffList(Diff(DELETE, "xcd"), Diff(INSERT, "12x34")), diffs);

    diffs = diffList(Diff(DELETE, "ab"), Diff(INSERT, "12"), Diff(EQUAL, "xy"), Diff(INSERT, "34"), Diff(EQUAL, "z"), Diff(DELETE, "cd"), Diff(INSERT, "56"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Backpass elimination.", diffList(Diff(DELETE, "abxyzcd"), Diff(INSERT, "12xy34z56")), diffs);

    dmp.Diff_EditCost = 5;
    diffs = diffList(Diff(DELETE, "ab"), Diff(INSERT, "12"), Diff(EQUAL, "wxyz"), Diff(DELETE, "cd"), Diff(INSERT, "34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: High cost elimination.", diffList(Diff(DELETE, "abwxyzcd"), Diff(INSERT, "12wxyz34")), diffs);
    dmp.Diff_EditCost = 4;
  }

  void testDiffPrettyHtml() {
    // Pretty print.
    Diffs diffs = diffList(Diff(EQUAL, "a\n"), Diff(DELETE, "<B>b</B>"), Diff(INSERT, "c&d"));
    assertEquals("diff_prettyHtml:", "<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>", dmp.diff_prettyHtml(diffs));
  }

  void testDiffText() {
    // Compute the source and destination texts.
    Diffs diffs = diffList(Diff(EQUAL, "jump"), Diff(DELETE, "s"), Diff(INSERT, "ed"), Diff(EQUAL, " over "), Diff(DELETE, "the"), Diff(INSERT, "a"), Diff(EQUAL, " lazy"));
    assertEquals("diff_text1:", "jumps over the lazy", dmp.diff_text1(diffs));
    assertEquals("diff_text2:", "jumped over a lazy", dmp.diff_text2(diffs));
  }

  void testDiffDelta() {
    // Convert a diff into delta string.
    Diffs diffs = diffList(Diff(EQUAL, "jump"), Diff(DELETE, "s"), Diff(INSERT, "ed"), Diff(EQUAL, " over "), Diff(DELETE, "the"), Diff(INSERT, "a"), Diff(EQUAL, " lazy"), Diff(INSERT, "old dog"));
    string_t text1 = dmp.diff_text1(diffs);
    assertEquals("diff_text1: Base text.", "jumps over the lazy", text1);

    string_t delta = dmp.diff_toDelta(diffs);
    assertEquals("diff_toDelta:", "=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta);

    // Convert delta string into a diff.
    assertEquals("diff_fromDelta: Normal.", diffs, dmp.diff_fromDelta(text1, delta));

    // Generates error (19 < 20).
    try {
      dmp.diff_fromDelta(text1 + "x", delta);
      assertFalse("diff_fromDelta: Too long.", true);
    } catch (string_t ex) {
      // Exception expected.
    }

    // Generates error (19 > 18).
    try {
      dmp.diff_fromDelta(text1.substr(1), delta);
      assertFalse("diff_fromDelta: Too short.", true);
    } catch (string_t ex) {
      // Exception expected.
    }

    // Generates error (%c3%xy invalid Unicode).
    try {
      dmp.diff_fromDelta("", "+%c3%xy");
      assertFalse("diff_fromDelta: Invalid character.", true);
    } catch (string_t ex) {
      // Exception expected.
    }

// TODO add again
//    // Test deltas with special characters.
//    diffs = diffList(Diff(EQUAL, string_t("\u0680 \000 \t %", 7)), Diff(DELETE, string_t("\u0681 \001 \n ^", 7)), Diff(INSERT, string_t("\u0682 \002 \\ |", 7)));
//    text1 = dmp.diff_text1(diffs);
//    assertEquals("diff_text1: Unicode text.", string_t(L"\u0680 \000 \t %\u0681 \001 \n ^", 14), text1);

//    delta = dmp.diff_toDelta(diffs);
//    assertEquals("diff_toDelta: Unicode.", "=7\t-7\t+%DA%82 %02 %5C %7C", delta);

//    assertEquals("diff_fromDelta: Unicode.", diffs, dmp.diff_fromDelta(text1, delta));

    // Verify pool of unchanged characters.
    diffs = diffList(Diff(INSERT, "A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # "));
    string_t text2 = dmp.diff_text2(diffs);
    assertEquals("diff_text2: Unchanged characters.", "A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2);

    delta = dmp.diff_toDelta(diffs);
    assertEquals("diff_toDelta: Unchanged characters.", "+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta);

    // Convert delta string into a diff.
    assertEquals("diff_fromDelta: Unchanged characters.", diffs, dmp.diff_fromDelta("", delta));
  }

  void testDiffXIndex() {
    // Translate a location in text1 to text2.
    Diffs diffs = diffList(Diff(DELETE, "a"), Diff(INSERT, "1234"), Diff(EQUAL, "xyz"));
    assertEquals("diff_xIndex: Translation on equality.", 5, dmp.diff_xIndex(diffs, 2));

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "1234"), Diff(EQUAL, "xyz"));
    assertEquals("diff_xIndex: Translation on deletion.", 1, dmp.diff_xIndex(diffs, 3));
  }

  void testDiffLevenshtein() {
    Diffs diffs = diffList(Diff(DELETE, "abc"), Diff(INSERT, "1234"), Diff(EQUAL, "xyz"));
    assertEquals("diff_levenshtein: Trailing equality.", 4, dmp.diff_levenshtein(diffs));

    diffs = diffList(Diff(EQUAL, "xyz"), Diff(DELETE, "abc"), Diff(INSERT, "1234"));
    assertEquals("diff_levenshtein: Leading equality.", 4, dmp.diff_levenshtein(diffs));

    diffs = diffList(Diff(DELETE, "abc"), Diff(EQUAL, "xyz"), Diff(INSERT, "1234"));
    assertEquals("diff_levenshtein: Middle equality.", 7, dmp.diff_levenshtein(diffs));
  }

  void testDiffBisect() {
    // Normal.
    string_t a = "cat";
    string_t b = "map";
    // Since the resulting diff hasn't been normalized, it would be ok if
    // the insertion and deletion pairs are swapped.
    // If the order changes, tweak this test as required.
    Diffs diffs = diffList(Diff(DELETE, "c"), Diff(INSERT, "m"), Diff(EQUAL, "a"), Diff(DELETE, "t"), Diff(INSERT, "p"));
    assertEquals("diff_bisect: Normal.", diffs, dmp.diff_bisect(a, b, numeric_limits<clock_t>::max()));

    // Timeout.
    while (clock() == 0); // Wait for the first tick
    diffs = diffList(Diff(DELETE, "cat"), Diff(INSERT, "map"));
    assertEquals("diff_bisect: Timeout.", diffs, dmp.diff_bisect(a, b, 0));
  }

  void testDiffMain() {
    // Perform a trivial diff.
    Diffs diffs = diffList();
    assertEquals("diff_main: Null case.", diffs, dmp.diff_main("", "", false));

    diffs = diffList(Diff(EQUAL, "abc"));
    assertEquals("diff_main: Equality.", diffs, dmp.diff_main("abc", "abc", false));

    diffs = diffList(Diff(EQUAL, "ab"), Diff(INSERT, "123"), Diff(EQUAL, "c"));
    assertEquals("diff_main: Simple insertion.", diffs, dmp.diff_main("abc", "ab123c", false));

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "123"), Diff(EQUAL, "bc"));
    assertEquals("diff_main: Simple deletion.", diffs, dmp.diff_main("a123bc", "abc", false));

    diffs = diffList(Diff(EQUAL, "a"), Diff(INSERT, "123"), Diff(EQUAL, "b"), Diff(INSERT, "456"), Diff(EQUAL, "c"));
    assertEquals("diff_main: Two insertions.", diffs, dmp.diff_main("abc", "a123b456c", false));

    diffs = diffList(Diff(EQUAL, "a"), Diff(DELETE, "123"), Diff(EQUAL, "b"), Diff(DELETE, "456"), Diff(EQUAL, "c"));
    assertEquals("diff_main: Two deletions.", diffs, dmp.diff_main("a123b456c", "abc", false));

    // Perform a real diff.
    // Switch off the timeout.
    dmp.Diff_Timeout = 0;
    diffs = diffList(Diff(DELETE, "a"), Diff(INSERT, "b"));
    assertEquals("diff_main: Simple case #1.", diffs, dmp.diff_main("a", "b", false));

    diffs = diffList(Diff(DELETE, "Apple"), Diff(INSERT, "Banana"), Diff(EQUAL, "s are a"), Diff(INSERT, "lso"), Diff(EQUAL, " fruit."));
    assertEquals("diff_main: Simple case #2.", diffs, dmp.diff_main("Apples are a fruit.", "Bananas are also fruit.", false));

//    diffs = diffList(Diff(DELETE, "a"), Diff(INSERT, L"\u0680"), Diff(EQUAL, "x"), Diff(DELETE, "\t"), Diff(INSERT, string_t("\000", 1)));
//    assertEquals("diff_main: Simple case #3.", diffs, dmp.diff_main("ax\t", string_t(L"\u0680x\000", 3), false));

    diffs = diffList(Diff(DELETE, "1"), Diff(EQUAL, "a"), Diff(DELETE, "y"), Diff(EQUAL, "b"), Diff(DELETE, "2"), Diff(INSERT, "xab"));
    assertEquals("diff_main: Overlap #1.", diffs, dmp.diff_main("1ayb2", "abxab", false));

    diffs = diffList(Diff(INSERT, "xaxcx"), Diff(EQUAL, "abc"), Diff(DELETE, "y"));
    assertEquals("diff_main: Overlap #2.", diffs, dmp.diff_main("abcy", "xaxcxabc", false));

    diffs = diffList(Diff(DELETE, "ABCD"), Diff(EQUAL, "a"), Diff(DELETE, "="), Diff(INSERT, "-"), Diff(EQUAL, "bcd"), Diff(DELETE, "="), Diff(INSERT, "-"), Diff(EQUAL, "efghijklmnopqrs"), Diff(DELETE, "EFGHIJKLMNOefg"));
    assertEquals("diff_main: Overlap #3.", diffs, dmp.diff_main("ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", "a-bcd-efghijklmnopqrs", false));

    diffs = diffList(Diff(INSERT, " "), Diff(EQUAL, "a"), Diff(INSERT, "nd"), Diff(EQUAL, " [[Pennsylvania]]"), Diff(DELETE, " and [[New"));
    assertEquals("diff_main: Large equality.", diffs, dmp.diff_main("a [[Pennsylvania]] and [[New", " and [[Pennsylvania]]", false));

    dmp.Diff_Timeout = 0.1f;  // 100ms
    // This test may 'fail' on extremely fast computers.  If so, just increase the text lengths.
    string_t a = "`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
    string_t b = "I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
    // Increase the text lengths by 1024 times to ensure a timeout.
    for (int x = 0; x < 10; x++) {
      a = a + a;
      b = b + b;
    }
    clock_t startTime = clock();
    dmp.diff_main(a, b);
    clock_t endTime = clock();
    // Test that we took at least the timeout period.
    assertTrue("diff_main: Timeout min.", dmp.Diff_Timeout * CLOCKS_PER_SEC <= endTime - startTime);
    // Test that we didn't take forever (be forgiving).
    // Theoretically this test could fail very occasionally if the
    // OS task swaps or locks up for a second at the wrong moment.
    // Java seems to overrun by ~80% (compared with 10% for other languages).
    // Therefore use an upper limit of 0.5s instead of 0.2s.
    assertTrue("diff_main: Timeout max.", dmp.Diff_Timeout * CLOCKS_PER_SEC * 2 > endTime - startTime);
    dmp.Diff_Timeout = 0;

    // Test the linemode speedup.
    // Must be long to pass the 100 char cutoff.
    a = "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = "abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
    assertEquals("diff_main: Simple line-mode.", dmp.diff_main(a, b, true), dmp.diff_main(a, b, false));

    a = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    b = "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
    assertEquals("diff_main: Single line-mode.", dmp.diff_main(a, b, true), dmp.diff_main(a, b, false));

    a = "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = "abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
    Strings texts_linemode = diff_rebuildtexts(dmp.diff_main(a, b, true));
    Strings texts_textmode = diff_rebuildtexts(dmp.diff_main(a, b, false));
    assertEquals("diff_main: Overlap line-mode.", texts_textmode, texts_linemode);

    // Test null inputs -- not needed because nulls can't be passed in string_t&.
  }


  //  MATCH TEST FUNCTIONS

  void testMatchAlphabet() {
    // Initialise the bitmasks for Bitap.
    map<char_t, int> bitmask, bitmask2;
    bitmask['a'] = 4;
    bitmask['b'] = 2;
    bitmask['c'] = 1;
    dmp.match_alphabet("abc", bitmask2);
    assertEquals("match_alphabet: Unique.", bitmask, bitmask2);

    bitmask.clear();
    bitmask['a'] = 37;
    bitmask['b'] = 18;
    bitmask['c'] = 8;
    bitmask2.clear();
    dmp.match_alphabet("abcaba", bitmask2);
    assertEquals("match_alphabet: Duplicates.", bitmask, bitmask2);
  }

  void testMatchBitap() {
    // Bitap algorithm.
    dmp.Match_Distance = 100;
    dmp.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Exact match #1.", 5, dmp.match_bitap("abcdefghijk", "fgh", 5));

    assertEquals("match_bitap: Exact match #2.", 5, dmp.match_bitap("abcdefghijk", "fgh", 0));

    assertEquals("match_bitap: Fuzzy match #1.", 4, dmp.match_bitap("abcdefghijk", "efxhi", 0));

    assertEquals("match_bitap: Fuzzy match #2.", 2, dmp.match_bitap("abcdefghijk", "cdefxyhijk", 5));

    assertEquals("match_bitap: Fuzzy match #3.", -1, dmp.match_bitap("abcdefghijk", "bxy", 1));

    assertEquals("match_bitap: Overflow.", 2, dmp.match_bitap("123456789xx0", "3456789x0", 2));

    assertEquals("match_bitap: Before start match.", 0, dmp.match_bitap("abcdef", "xxabc", 4));

    assertEquals("match_bitap: Beyond end match.", 3, dmp.match_bitap("abcdef", "defyy", 4));

    assertEquals("match_bitap: Oversized pattern.", 0, dmp.match_bitap("abcdef", "xabcdefy", 0));

    dmp.Match_Threshold = 0.4f;
    assertEquals("match_bitap: Threshold #1.", 4, dmp.match_bitap("abcdefghijk", "efxyhi", 1));

    dmp.Match_Threshold = 0.3f;
    assertEquals("match_bitap: Threshold #2.", -1, dmp.match_bitap("abcdefghijk", "efxyhi", 1));

    dmp.Match_Threshold = 0.0f;
    assertEquals("match_bitap: Threshold #3.", 1, dmp.match_bitap("abcdefghijk", "bcdef", 1));

    dmp.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Multiple select #1.", 0, dmp.match_bitap("abcdexyzabcde", "abccde", 3));

    assertEquals("match_bitap: Multiple select #2.", 8, dmp.match_bitap("abcdexyzabcde", "abccde", 5));

    dmp.Match_Distance = 10;  // Strict location.
    assertEquals("match_bitap: Distance test #1.", -1, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));

    assertEquals("match_bitap: Distance test #2.", 0, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdxxefg", 1));

    dmp.Match_Distance = 1000;  // Loose location.
    assertEquals("match_bitap: Distance test #3.", 0, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));
  }

  void testMatchMain() {
    // Full match.
    assertEquals("match_main: Equality.", 0, dmp.match_main("abcdef", "abcdef", 1000));

    assertEquals("match_main: Null text.", -1, dmp.match_main("", "abcdef", 1));

    assertEquals("match_main: Null pattern.", 3, dmp.match_main("abcdef", "", 3));

    assertEquals("match_main: Exact match.", 3, dmp.match_main("abcdef", "de", 3));

    dmp.Match_Threshold = 0.7f;
    assertEquals("match_main: Complex match.", 4, dmp.match_main("I am the very model of a modern major general.", " that berry ", 5));
    dmp.Match_Threshold = 0.5f;

    // Test null inputs -- not needed because nulls can't be passed in string_t&.
  }


  //  PATCH TEST FUNCTIONS

  void testPatchObj() {
    // Patch Object.
    Patch p;
    p.start1 = 20;
    p.start2 = 21;
    p.length1 = 18;
    p.length2 = 17;
    p.diffs = diffList(Diff(EQUAL, "jump"), Diff(DELETE, "s"), Diff(INSERT, "ed"), Diff(EQUAL, " over "), Diff(DELETE, "the"), Diff(INSERT, "a"), Diff(EQUAL, "\nlaz"));
    string_t strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals("Patch: toString.", strp, p.toString());
  }

  void testPatchFromText() {
    assertTrue("patch_fromText: #0.", dmp.patch_fromText("").empty());

    string_t strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals("patch_fromText: #1.", strp, dmp.patch_fromText(strp).front().toString());

    assertEquals("patch_fromText: #2.", "@@ -1 +1 @@\n-a\n+b\n", dmp.patch_fromText("@@ -1 +1 @@\n-a\n+b\n").front().toString());

    assertEquals("patch_fromText: #3.", "@@ -1,3 +0,0 @@\n-abc\n", dmp.patch_fromText("@@ -1,3 +0,0 @@\n-abc\n").front().toString());

    assertEquals("patch_fromText: #4.", "@@ -0,0 +1,3 @@\n+abc\n", dmp.patch_fromText("@@ -0,0 +1,3 @@\n+abc\n").front().toString());

    // Generates error.
    try {
      dmp.patch_fromText("Bad\nPatch\n");
      assertFalse("patch_fromText: #5.", true);
    } catch (string_t ex) {
      // Exception expected.
    }
  }

  void testPatchToText() {
    string_t strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    Patches patches;
    patches = dmp.patch_fromText(strp);
    assertEquals("patch_toText: Single", strp, dmp.patch_toText(patches));

    strp = "@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
    patches = dmp.patch_fromText(strp);
    assertEquals("patch_toText: Dua", strp, dmp.patch_toText(patches));
  }

  void testPatchAddContext() {
    dmp.Patch_Margin = 4;
    Patch p;
    p = dmp.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n").front();
    dmp.patch_addContext(p, "The quick brown fox jumps over the lazy dog.");
    assertEquals("patch_addContext: Simple case.", "@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString());

    p = dmp.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n").front();
    dmp.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough trailing context.", "@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString());

    p = dmp.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n").front();
    dmp.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough leading context.", "@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString());

    p = dmp.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n").front();
    dmp.patch_addContext(p, "The quick brown fox jumps.  The quick brown fox crashes.");
    assertEquals("patch_addContext: Ambiguity.", "@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString());
  }

  void testPatchMake() {
    Patches patches;
    patches = dmp.patch_make("", "");
    assertEquals("patch_make: Null case", "", dmp.patch_toText(patches));

    string_t text1 = "The quick brown fox jumps over the lazy dog.";
    string_t text2 = "That quick brown fox jumped over a lazy dog.";
    string_t expectedPatch = "@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
    // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
    patches = dmp.patch_make(text2, text1);
    assertEquals("patch_make: Text2+Text1 inputs", expectedPatch, dmp.patch_toText(patches));

    expectedPatch = "@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    patches = dmp.patch_make(text1, text2);
    assertEquals("patch_make: Text1+Text2 inputs", expectedPatch, dmp.patch_toText(patches));

    Diffs diffs = dmp.diff_main(text1, text2, false);
    patches = dmp.patch_make(diffs);
    assertEquals("patch_make: Diff input", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make(text1, diffs);
    assertEquals("patch_make: Text1+Diff inputs", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make(text1, text2, diffs);
    assertEquals("patch_make: Text1+Text2+Diff inputs (deprecated)", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make("`1234567890-=[]\\;',./", "~!@#$%^&*()_+{}|:\"<>?");
    assertEquals("patch_toText: Character encoding.", "@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n", dmp.patch_toText(patches));

    diffs = diffList(Diff(DELETE, "`1234567890-=[]\\;',./"), Diff(INSERT, "~!@#$%^&*()_+{}|:\"<>?"));
    assertEquals("patch_fromText: Character decoding.", diffs, dmp.patch_fromText("@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n").front().diffs);

    text1.clear();
    for (int x = 0; x < 100; x++) {
      text1 += "abcdef";
    }
    text2 = text1 + "123";
    expectedPatch = "@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
    patches = dmp.patch_make(text1, text2);
    assertEquals("patch_make: Long string with repeats.", expectedPatch, dmp.patch_toText(patches));

    // Test null inputs -- not needed because nulls can't be passed in string_t&.
  }

  void testPatchSplitMax() {
    // Assumes that Match_MaxBits is 32.
    Patches patches;
    patches = dmp.patch_make("abcdefghijklmnopqrstuvwxyz01234567890", "XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0");
    dmp.patch_splitMax(patches);
    assertEquals("patch_splitMax: #1.", "@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", dmp.patch_toText(patches));

    patches = dmp.patch_make("abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz", "abcdefuvwxyz");
    string_t oldToText = dmp.patch_toText(patches);
    dmp.patch_splitMax(patches);
    assertEquals("patch_splitMax: #2.", oldToText, dmp.patch_toText(patches));

    patches = dmp.patch_make("1234567890123456789012345678901234567890123456789012345678901234567890", "abc");
    dmp.patch_splitMax(patches);
    assertEquals("patch_splitMax: #3.", "@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", dmp.patch_toText(patches));

    patches = dmp.patch_make("abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1", "abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1");
    dmp.patch_splitMax(patches);
    assertEquals("patch_splitMax: #4.", "@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", dmp.patch_toText(patches));
  }

  void testPatchAddPadding() {
    Patches patches;
    patches = dmp.patch_make("", "test");
    assertEquals("patch_addPadding: Both edges full.", "@@ -0,0 +1,4 @@\n+test\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges full.", "@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n", dmp.patch_toText(patches));

    patches = dmp.patch_make("XY", "XtestY");
    assertEquals("patch_addPadding: Both edges partial.", "@@ -1,2 +1,6 @@\n X\n+test\n Y\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges partial.", "@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n", dmp.patch_toText(patches));

    patches = dmp.patch_make("XXXXYYYY", "XXXXtestYYYY");
    assertEquals("patch_addPadding: Both edges none.", "@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges none.", "@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText(patches));
  }

  void testPatchApply() {
    dmp.Match_Distance = 1000;
    dmp.Match_Threshold = 0.5f;
    dmp.Patch_DeleteThreshold = 0.5f;
    Patches patches;
    patches = dmp.patch_make("", "");
    pair<string_t, vector<bool> > results = dmp.patch_apply(patches, "Hello world.");
    vector<bool> boolArray = results.second;

    basic_stringstream<char_t> result;
    result << results.first << traits::tab << boolArray.size();
    assertEquals("patch_apply: Null case.", "Hello world.\t0", result.str());

    string_t resultStr;
    patches = dmp.patch_make("The quick brown fox jumps over the lazy dog.", "That quick brown fox jumped over a lazy dog.");
    results = dmp.patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Exact match.", "That quick brown fox jumped over a lazy dog.\ttrue\ttrue", resultStr);

    results = dmp.patch_apply(patches, "The quick red rabbit jumps over the tired tiger.");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Partial match.", "That quick red rabbit jumped over a tired tiger.\ttrue\ttrue", resultStr);

    results = dmp.patch_apply(patches, "I am the very model of a modern major general.");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Failed match.", "I am the very model of a modern major general.\tfalse\tfalse", resultStr);

    patches = dmp.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = dmp.patch_apply(patches, "x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Big delete, small change.", "xabcy\ttrue\ttrue", resultStr);

    patches = dmp.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = dmp.patch_apply(patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Big delete, large change 1.", "xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue", resultStr);

    dmp.Patch_DeleteThreshold = 0.6f;
    patches = dmp.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = dmp.patch_apply(patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Big delete, large change 2.", "xabcy\ttrue\ttrue", resultStr);
    dmp.Patch_DeleteThreshold = 0.5f;

    dmp.Match_Threshold = 0.0f;
    dmp.Match_Distance = 0;
    patches = dmp.patch_make("abcdefghijklmnopqrstuvwxyz--------------------1234567890", "abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890");
    results = dmp.patch_apply(patches, "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false") + "\t" + (boolArray[1] ? "true" : "false");
    assertEquals("patch_apply: Compensate for failed patch.", "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue", resultStr);
    dmp.Match_Threshold = 0.5f;
    dmp.Match_Distance = 1000;

    patches = dmp.patch_make("", "test");
    string_t patchStr = dmp.patch_toText(patches);
    dmp.patch_apply(patches, "");
    assertEquals("patch_apply: No side effects.", patchStr, dmp.patch_toText(patches));

    patches = dmp.patch_make("The quick brown fox jumps over the lazy dog.", "Woof");
    patchStr = dmp.patch_toText(patches);
    dmp.patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
    assertEquals("patch_apply: No side effects with major delete.", patchStr, dmp.patch_toText(patches));

    patches = dmp.patch_make("", "test");
    results = dmp.patch_apply(patches, "");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false");
    assertEquals("patch_apply: Edge exact match.", "test\ttrue", resultStr);

    patches = dmp.patch_make("XY", "XtestY");
    results = dmp.patch_apply(patches, "XY");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false");
    assertEquals("patch_apply: Near edge exact match.", "XtestY\ttrue", resultStr);

    patches = dmp.patch_make("y", "y123");
    results = dmp.patch_apply(patches, "x");
    boolArray = results.second;
    resultStr = results.first + "\t" + (boolArray[0] ? "true" : "false");
    assertEquals("patch_apply: Edge partial match.", "x123\ttrue", resultStr);
  }

 private:
  // Define equality.
  void assertEquals(const char* strCase, int n1, int n2) {
    if (n1 != n2) {
      cerr << strCase << " FAIL\nExpected: " << n1 << "\nActual: " << n2 << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const string_t &s1, const string_t &s2) {
    if (s1 != s2) {
      cerr << strCase << " FAIL\nExpected: " << s1 << "\nActual: " << s2 << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const Diff &d1, const Diff &d2) {
    if (d1 != d2) {
      cerr << strCase << " FAIL\nExpected: " << d1.toString() << "\nActual: " << d2.toString() << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const Diffs &list1, const Diffs &list2) {
    bool fail = false;
    if (list1.size() == list2.size()) {
      for (Diffs::const_iterator d1 = list1.begin(), d2 = list2.begin(); d1 != list1.end(); ++d1, ++d2) {
        if (*d1 != *d2) {
          fail = true;
          break;
        }
      }
    } else {
      fail = true;
    }

    if (fail) {
      // Build human readable description of both lists.
      string_t listString1 = "(";
      bool first = true;
      for (Diffs::const_iterator d1 = list1.begin(); d1 != list1.end(); ++d1) {
        if (!first) {
          listString1 += ", ";
        }
        listString1 += (*d1).toString();
        first = false;
      }
      listString1 += ")";
      string_t listString2 = "(";
      first = true;
      for (Diffs::const_iterator d2 = list2.begin(); d2 != list2.end(); ++d2) {
        if (!first) {
          listString2 += ", ";
        }
        listString2 += (*d2).toString();
        first = false;
      }
      listString2 += ")";
      cerr << strCase << " FAIL\nExpected: " << listString1 << "\nActual: " << listString2 << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const string_t& text1_1, const string_t& text2_1, const Lines &lines1,
                                                                const string_t& text1_2, const string_t& text2_2, const Lines &lines2)
  {
    bool fail = false;
    if (text1_1 != text1_2 || text2_1 != text2_2 || lines1.size() != lines2.size())
      fail = true;
    else
      for (Lines::const_iterator i = lines1.begin(), j = lines2.begin(); i != lines1.end(); ++i, ++j)
        if (string_t((*i).first, (*i).second) != string_t((*j).first, (*j).second)) { fail = true; break; }

    if (fail) {
      // Build human readable description of both lists.
      string_t listString1 = "\"" + text1_1 + "\", \"" + text2_1 + "\", (\"";
      bool first = true;
      for (Lines::const_iterator i = lines1.begin() + 1; i != lines1.end(); ++i) {
        if (!first) listString1 += "\", \"";
        listString1.append((*i).first, (*i).second);
        first = false;
      }
      listString1 += "\")";
      string_t listString2 = "\"" + text1_2 + "\", \"" + text2_2 + "\", (\"";
      first = true;
      for (Lines::const_iterator j = lines2.begin() + 1; j != lines2.end(); ++j) {
        if (!first) listString2 += "\", \"";
        listString2.append((*j).first, (*j).second);
        first = false;
      }
      listString2 += "\")";
      cerr << strCase << " FAIL\nExpected: " << listString1 << "\nActual: " << listString2 << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const map<char_t, int> &m1, const map<char_t, int> &m2) {
    map<char_t, int>::const_iterator i1 = m1.begin(), i2 = m2.begin();

    while (i1 != m1.end() && i2 != m2.end()) {
      if ((*i1).first != (*i2).first || (*i1).second != (*i2).second) {
        cerr << strCase << " FAIL\nExpected: (" << char((*i1).first) << ", " << (*i1).second <<
                                 ")\nActual: (" << char((*i2).first) << ", " << (*i2).second << ')' << endl;
        throw strCase;
      }
      ++i1, ++i2;
    }

    if (i1 != m1.end()) {
      cerr << strCase << " FAIL\nExpected: (" << char((*i1).first) << ", " << (*i1).second << ")\nActual: none" << endl;
      throw strCase;
    }
    if (i2 != m2.end()) {
      cerr << strCase << " FAIL\nExpected: none\nActual: (" << char((*i2).first) << ", " << (*i2).second << ')' << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEquals(const char* strCase, const Strings &list1, const Strings &list2) {
    if (list1 != list2) {
      cerr << strCase << " FAIL\nExpected: " << join(list1, ',') << "\nActual: " << join(list2, ',') << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertTrue(const char* strCase, bool value) {
    if (!value) {
      cerr << strCase << " FAIL\nExpected: true\nActual: false" << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertFalse(const char* strCase, bool value) {
    if (value) {
      cerr << strCase << " FAIL\nExpected: false\nActual: true" << endl;
      throw strCase;
    }
    cout << strCase << " OK" << endl;
  }

  void assertEmpty(const char* strCase, const Strings &list) {
    if (!list.empty()) {
      throw strCase;
    }
  }

  // Construct the two texts which made up the diff originally.
  static Strings diff_rebuildtexts(Diffs diffs) {
    Strings text;
    text.push_back(string_t());
    text.push_back(string_t());
    Strings::iterator t1 = text.begin(), t0 = t1++;
    for (Diffs::const_iterator myDiff = diffs.begin(); myDiff != diffs.end(); ++myDiff) {
      if ((*myDiff).operation != INSERT) {
        *t0 += (*myDiff).text;
      }
      if ((*myDiff).operation != DELETE) {
        *t1 += (*myDiff).text;
      }
    }
    return text;
  }

  // Private function for quickly building lists of diffs.
  static Diffs diffList(
      // Diff(INSERT, NULL) is invalid and thus is used as the default argument.
      Diff d1 = Diff(INSERT, string_t()), Diff d2 = Diff(INSERT, string_t()),
      Diff d3 = Diff(INSERT, string_t()), Diff d4 = Diff(INSERT, string_t()),
      Diff d5 = Diff(INSERT, string_t()), Diff d6 = Diff(INSERT, string_t()),
      Diff d7 = Diff(INSERT, string_t()), Diff d8 = Diff(INSERT, string_t()),
      Diff d9 = Diff(INSERT, string_t()), Diff d10 = Diff(INSERT, string_t())) {
    // Diff(INSERT, NULL) is invalid and thus is used as the default argument.
    Diffs listRet;
    if (d1.operation == INSERT && d1.text.empty()) {
      return listRet;
    }
    listRet.push_back(d1);

    if (d2.operation == INSERT && d2.text.empty()) {
      return listRet;
    }
    listRet.push_back(d2);

    if (d3.operation == INSERT && d3.text.empty()) {
      return listRet;
    }
    listRet.push_back(d3);

    if (d4.operation == INSERT && d4.text.empty()) {
      return listRet;
    }
    listRet.push_back(d4);

    if (d5.operation == INSERT && d5.text.empty()) {
      return listRet;
    }
    listRet.push_back(d5);

    if (d6.operation == INSERT && d6.text.empty()) {
      return listRet;
    }
    listRet.push_back(d6);

    if (d7.operation == INSERT && d7.text.empty()) {
      return listRet;
    }
    listRet.push_back(d7);

    if (d8.operation == INSERT && d8.text.empty()) {
      return listRet;
    }
    listRet.push_back(d8);

    if (d9.operation == INSERT && d9.text.empty()) {
      return listRet;
    }
    listRet.push_back(d9);

    if (d10.operation == INSERT && d10.text.empty()) {
      return listRet;
    }
    listRet.push_back(d10);

    return listRet;
  }

  Strings diff_halfMatch(const string_t &text1, const string_t &text2) {
    Strings list;
    HalfMatchResult hm;
    if (diff_match_patch<string>::diff_halfMatch(text1, text2, hm)) {
      list.push_back(hm.text1_a);
      list.push_back(hm.text1_b);
      list.push_back(hm.text2_a);
      list.push_back(hm.text2_b);
      list.push_back(hm.mid_common);
    }
    return list;
  }

  static string_t join(const Strings &list, char delim) {
    string_t s;
    for (Strings::const_iterator i = list.begin(); i != list.end(); ++i) {
      if (i != list.begin()) s += delim;
      s += *i;
    }
    return s;
  }

  static Strings split(const string_t& str, char delim) {
    Strings list;
    string_t::size_type token_len;
    for (string_t::const_pointer token = str.c_str(), end = token + str.length(), p; token < end; token += token_len + 1) {
      for (p = token; p != end; ++p) if (*p == delim) break;
      list.push_back(string_t(token, token_len = p - token));
    }
    return list;
  }
};


int main() {
  diff_match_patch_test dmp_test;
  cout << "Starting diff_match_patch unit tests." << endl;
  dmp_test.run_all_tests();
  cout << "Done." << endl;
  return 0;
}


/*
Compile instructions for MinGW on Windows:
g++ -O2 -o diff_match_patch_test diff_match_patch_test.cpp
diff_match_patch_test.exe
*/
