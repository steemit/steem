/*
 * Copyright 2008 Google Inc. All Rights Reserved.
 * Author: fraser@google.com (Neil Fraser)
 * Author: anteru@developer.shelter13.net (Matthaeus G. Chajdas)
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
 * Diff Match and Patch
 * http://code.google.com/p/google-diff-match-patch/
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Web;

namespace DiffMatchPatch
{
    internal static class CompatibilityExtensions
    {
        // JScript splice function
        public static List<T> Splice<T>(this List<T> input, int start, int count, params T[] objects)
        {
            List<T> deletedRange = input.GetRange(start, count);
            input.RemoveRange(start, count);
            input.InsertRange(start, objects);

            return deletedRange;
        }

        // Java substring function
        public static string JavaSubstring(this string s, int begin, int end)
        {
            return s.Substring(begin, end - begin);
        }
    }


    /**-
    * The data structure representing a diff is a Linked list of Diff objects:
    * {Diff(Operation.DELETE, "Hello"), Diff(Operation.INSERT, "Goodbye"),
    *  Diff(Operation.EQUAL, " world.")}
    * which means: delete "Hello", add "Goodbye" and keep " world."
    */
    public enum Operation
    {
        DELETE, INSERT, EQUAL
    }


    /**
    * Class representing one diff operation.
    */
    public class Diff
    {
        public Operation operation;
        // One of: INSERT, DELETE or EQUAL.
        public string text;
        // The text associated with this diff operation.

        /**
         * Constructor.  Initializes the diff with the provided values.
         * @param operation One of INSERT, DELETE or EQUAL.
         * @param text The text being applied.
         */
        public Diff(Operation operation, string text)
        {
            // Construct a diff with the specified operation and text.
            this.operation = operation;
            this.text = text;
        }

        /**
         * Display a human-readable version of this Diff.
         * @return text version.
         */
        public override string ToString()
        {
            string prettyText = this.text.Replace('\n', '\u00b6');
            return "Diff(" + this.operation + ",\"" + prettyText + "\")";
        }

        /**
         * Is this Diff equivalent to another Diff?
         * @param d Another Diff to compare against.
         * @return true or false.
         */
        public override bool Equals(Object obj)
        {
            // If parameter is null return false.
            if (obj == null)
            {
                return false;
            }

            // If parameter cannot be cast to Diff return false.
            Diff p = obj as Diff;
            if ((System.Object)p == null)
            {
                return false;
            }

            // Return true if the fields match.
            return p.operation == this.operation &&
                p.text == this.text;
        }

        public bool Equals(Diff obj)
        {
            // If parameter is null return false.
            if (obj == null)
            {
                return false;
            }

            // Return true if the fields match.
            return obj.operation == this.operation &&
                obj.text == this.text;
        }

        public override int GetHashCode()
        {
            return text.GetHashCode() ^ operation.GetHashCode();
        }
    }


    /**
     * Class representing one patch operation.
     */
    public class Patch
    {
        public List<Diff> diffs = new List<Diff>();
        public int start1;
        public int start2;
        public int length1;
        public int length2;

        /**
         * Emmulate GNU diff's format.
         * Header: @@ -382,8 +481,9 @@
         * Indicies are printed as 1-based, not 0-based.
         * @return The GNU diff string.
         */
        public override string ToString()
        {
            string coords1, coords2;
            if (this.length1 == 0)
            {
                coords1 = this.start1 + ",0";
            }
            else if (this.length1 == 1)
            {
                coords1 = Convert.ToString(this.start1 + 1);
            }
            else
            {
                coords1 = (this.start1 + 1) + "," + this.length1;
            }
            if (this.length2 == 0)
            {
                coords2 = this.start2 + ",0";
            }
            else if (this.length2 == 1)
            {
                coords2 = Convert.ToString(this.start2 + 1);
            }
            else
            {
                coords2 = (this.start2 + 1) + "," + this.length2;
            }
            StringBuilder text = new StringBuilder();
            text.Append("@@ -").Append(coords1).Append(" +").Append(coords2)
                .Append(" @@\n");
            // Escape the body of the patch with %xx notation.
            foreach (Diff aDiff in this.diffs)
            {
                switch (aDiff.operation)
                {
                    case Operation.INSERT:
                        text.Append('+');
                        break;
                    case Operation.DELETE:
                        text.Append('-');
                        break;
                    case Operation.EQUAL:
                        text.Append(' ');
                        break;
                }

                text.Append(HttpUtility.UrlEncode(aDiff.text, new UTF8Encoding()).Replace('+', ' ')).Append("\n");
            }

            return diff_match_patch.unescapeForEncodeUriCompatability(text.ToString());
        }
    }


    /**
     * Class containing the diff, match and patch methods.
     * Also Contains the behaviour settings.
     */
    public class diff_match_patch
    {
        // Defaults.
        // Set these on your diff_match_patch instance to override the defaults.

        // Number of seconds to map a diff before giving up (0 for infinity).
        public float Diff_Timeout = 1.0f;
        // Cost of an empty edit operation in terms of edit characters.
        public short Diff_EditCost = 4;
        // The size beyond which the double-ended diff activates.
        // Double-ending is twice as fast, but less accurate.
        public short Diff_DualThreshold = 32;
        // At what point is no match declared (0.0 = perfection, 1.0 = very loose).
        public float Match_Threshold = 0.5f;
        // How far to search for a match (0 = exact location, 1000+ = broad match).
        // A match this many characters away from the expected location will add
        // 1.0 to the score (0.0 is a perfect match).
        public int Match_Distance = 1000;
        // When deleting a large block of text (over ~64 characters), how close
        // does the contents have to match the expected contents. (0.0 =
        // perfection, 1.0 = very loose).  Note that Match_Threshold controls
        // how closely the end points of a delete need to match.
        public float Patch_DeleteThreshold = 0.5f;
        // Chunk size for context length.
        public short Patch_Margin = 4;

        // The number of bits in an int.
        private int Match_MaxBits = 32;


        //  DIFF FUNCTIONS


        /**
         * Find the differences between two texts.
         * Run a faster slightly less optimal diff
         * This method allows the 'checklines' of diff_main() to be optional.
         * Most of the time checklines is wanted, so default to true.
         * @param text1 Old string to be diffed.
         * @param text2 New string to be diffed.
         * @return Linked List of Diff objects.
         */
        public List<Diff> diff_main(string text1, string text2)
        {
            return diff_main(text1, text2, true);
        }

        /**
         * Find the differences between two texts.  Simplifies the problem by
         * stripping any common prefix or suffix off the texts before diffing.
         * @param text1 Old string to be diffed.
         * @param text2 New string to be diffed.
         * @param checklines Speedup flag.  If false, then don't run a
         *     line-level diff first to identify the changed areas.
         *     If true, then run a faster slightly less optimal diff
         * @return Linked List of Diff objects.
         */
        public List<Diff> diff_main(string text1, string text2,
                                    bool checklines)
        {
            // Check for equality (speedup)
            List<Diff> diffs;
            if (text1 == text2)
            {
                diffs = new List<Diff>();
                diffs.Add(new Diff(Operation.EQUAL, text1));
                return diffs;
            }

            // Trim off common prefix (speedup)
            int commonlength = diff_commonPrefix(text1, text2);
            string commonprefix = text1.Substring(0, commonlength);
            text1 = text1.Substring(commonlength);
            text2 = text2.Substring(commonlength);

            // Trim off common suffix (speedup)
            commonlength = diff_commonSuffix(text1, text2);
            string commonsuffix = text1.Substring(text1.Length - commonlength);
            text1 = text1.Substring(0, text1.Length - commonlength);
            text2 = text2.Substring(0, text2.Length - commonlength);

            // Compute the diff on the middle block
            diffs = diff_compute(text1, text2, checklines);

            // Restore the prefix and suffix
            if (commonprefix.Length != 0)
            {
                diffs.Insert(0, (new Diff(Operation.EQUAL, commonprefix)));
            }
            if (commonsuffix.Length != 0)
            {
                diffs.Add(new Diff(Operation.EQUAL, commonsuffix));
            }

            diff_cleanupMerge(diffs);
            return diffs;
        }

        /**
         * Find the differences between two texts.  Assumes that the texts do not
         * have any common prefix or suffix.
         * @param text1 Old string to be diffed.
         * @param text2 New string to be diffed.
         * @param checklines Speedup flag.  If false, then don't run a
         *     line-level diff first to identify the changed areas.
         *     If true, then run a faster slightly less optimal diff
         * @return Linked List of Diff objects.
         */
        protected List<Diff> diff_compute(string text1, string text2,
                                          bool checklines)
        {
            List<Diff> diffs = new List<Diff>();

            if (text1.Length == 0)
            {
                // Just add some text (speedup)
                diffs.Add(new Diff(Operation.INSERT, text2));
                return diffs;
            }

            if (text2.Length == 0)
            {
                // Just delete some text (speedup)
                diffs.Add(new Diff(Operation.DELETE, text1));
                return diffs;
            }

            string longtext = text1.Length > text2.Length ? text1 : text2;
            string shorttext = text1.Length > text2.Length ? text2 : text1;
            int i = longtext.IndexOf(shorttext);
            if (i != -1)
            {
                // Shorter text is inside the longer text (speedup)
                Operation op = (text1.Length > text2.Length) ?
                               Operation.DELETE : Operation.INSERT;
                diffs.Add(new Diff(op, longtext.Substring(0, i)));
                diffs.Add(new Diff(Operation.EQUAL, shorttext));
                diffs.Add(new Diff(op, longtext.Substring(i + shorttext.Length)));
                return diffs;
            }
            longtext = shorttext = null;  // Garbage collect.

            // Check to see if the problem can be split in two.
            string[] hm = diff_halfMatch(text1, text2);
            if (hm != null)
            {
                // A half-match was found, sort out the return data.
                string text1_a = hm[0];
                string text1_b = hm[1];
                string text2_a = hm[2];
                string text2_b = hm[3];
                string mid_common = hm[4];
                // Send both pairs off for separate processing.
                List<Diff> diffs_a = diff_main(text1_a, text2_a, checklines);
                List<Diff> diffs_b = diff_main(text1_b, text2_b, checklines);
                // Merge the results.
                diffs = diffs_a;
                diffs.Add(new Diff(Operation.EQUAL, mid_common));
                diffs.AddRange(diffs_b);
                return diffs;
            }

            // Perform a real diff.
            if (checklines && (text1.Length < 100 || text2.Length < 100))
            {
                checklines = false;  // Too trivial for the overhead.
            }

            List<string> linearray = null;
            if (checklines)
            {
                // Scan the text on a line-by-line basis first.
                Object[] b = diff_linesToChars(text1, text2);
                text1 = (string)b[0];
                text2 = (string)b[1];
                // The following Java warning is harmless.
                // Suggestions for how to clear it would be appreciated.
                linearray = (List<string>)b[2];
            }

            diffs = diff_map(text1, text2);
            if (diffs == null)
            {
                // No acceptable result.
                diffs = new List<Diff>();
                diffs.Add(new Diff(Operation.DELETE, text1));
                diffs.Add(new Diff(Operation.INSERT, text2));
            }

            if (checklines)
            {
                // Convert the diff back to original text.
                diff_charsToLines(diffs, linearray);
                // Eliminate freak matches (e.g. blank lines)
                diff_cleanupSemantic(diffs);

                // Rediff any Replacement blocks, this time character-by-character.
                // Add a dummy entry at the end.
                diffs.Add(new Diff(Operation.EQUAL, string.Empty));
                var pointer = 0;
                var count_delete = 0;
                var count_insert = 0;
                string text_delete = string.Empty;
                string text_insert = string.Empty;
                while (pointer < diffs.Count)
                {
                    switch (diffs[pointer].operation)
                    {
                        case Operation.INSERT:
                            count_insert++;
                            text_insert += diffs[pointer].text;
                            break;
                        case Operation.DELETE:
                            count_delete++;
                            text_delete += diffs[pointer].text;
                            break;
                        case Operation.EQUAL:
                            // Upon reaching an equality, check for prior redundancies.
                            if (count_delete >= 1 && count_insert >= 1)
                            {
                                // Delete the offending records and add the merged ones.
                                var a = this.diff_main(text_delete, text_insert, false);
                                diffs.RemoveRange(pointer - count_delete - count_insert,
                                    count_delete + count_insert);
                                pointer = pointer - count_delete - count_insert;
                                diffs.InsertRange(pointer, a);
                                pointer = pointer + a.Count;
                            }
                            count_insert = 0;
                            count_delete = 0;
                            text_delete = string.Empty;
                            text_insert = string.Empty;
                            break;
                    }
                    pointer++;
                }
                diffs.RemoveAt(diffs.Count - 1); ;  // Remove the dummy entry at the end.
            }
            return diffs;
        }

        /**
         * Split two texts into a list of strings.  Reduce the texts to a string of
         * hashes where each Unicode character represents one line.
         * @param text1 First string.
         * @param text2 Second string.
         * @return Three element Object array, containing the encoded text1, the
         *     encoded text2 and the List of unique strings.  The zeroth element
         *     of the List of unique strings is intentionally blank.
         */
        protected Object[] diff_linesToChars(string text1, string text2)
        {
            List<string> lineArray = new List<string>();
            Dictionary<string, int> lineHash = new Dictionary<string, int>();
            // e.g. linearray[4] == "Hello\n"
            // e.g. linehash.get("Hello\n") == 4

            // "\x00" is a valid character, but various debuggers don't like it.
            // So we'll insert a junk entry to avoid generating a null character.
            lineArray.Add(string.Empty);

            string chars1 = diff_linesToCharsMunge(text1, lineArray, lineHash);
            string chars2 = diff_linesToCharsMunge(text2, lineArray, lineHash);
            return new Object[] { chars1, chars2, lineArray };
        }

        /**
         * Split a text into a list of strings.  Reduce the texts to a string of
         * hashes where each Unicode character represents one line.
         * @param text String to encode.
         * @param lineArray List of unique strings.
         * @param lineHash Map of strings to indices.
         * @return Encoded string.
         */
        private string diff_linesToCharsMunge(string text, List<string> lineArray,
                                              Dictionary<string, int> lineHash)
        {
            int lineStart = 0;
            int lineEnd = -1;
            string line;
            StringBuilder chars = new StringBuilder();
            // Walk the text, pulling out a Substring for each line.
            // text.split('\n') would would temporarily double our memory footprint.
            // Modifying text would create many large strings to garbage collect.
            while (lineEnd < text.Length - 1)
            {
                lineEnd = text.IndexOf('\n', lineStart);
                if (lineEnd == -1)
                {
                    lineEnd = text.Length - 1;
                }
                line = text.JavaSubstring(lineStart, lineEnd + 1);
                lineStart = lineEnd + 1;

                if (lineHash.ContainsKey(line))
                {
                    chars.Append(((char)(int)lineHash[line]));
                }
                else
                {
                    lineArray.Add(line);
                    lineHash.Add(line, lineArray.Count - 1);
                    chars.Append(((char)(lineArray.Count - 1)));
                }
            }
            return chars.ToString();
        }

        /**
         * Rehydrate the text in a diff from a string of line hashes to real lines of
         * text.
         * @param diffs LinkedList of Diff objects.
         * @param lineArray List of unique strings.
         */
        protected void diff_charsToLines(ICollection<Diff> diffs,
                                        List<string> lineArray)
        {
            StringBuilder text;
            foreach (Diff diff in diffs)
            {
                text = new StringBuilder();
                for (int y = 0; y < diff.text.Length; y++)
                {
                    text.Append(lineArray[diff.text[y]]);
                }
                diff.text = text.ToString();
            }
        }

        /**
         * Explore the intersection points between the two texts.
         * @param text1 Old string to be diffed.
         * @param text2 New string to be diffed.
         * @return LinkedList of Diff objects or null if no diff available.
         */
        protected List<Diff> diff_map(string text1, string text2)
        {
            DateTime ms_end = DateTime.Now + new TimeSpan(0, 0, (int)Diff_Timeout);
            // Cache the text lengths to prevent multiple calls.
            int text1_length = text1.Length;
            int text2_length = text2.Length;
            int max_d = text1_length + text2_length - 1;
            bool doubleEnd = Diff_DualThreshold * 2 < max_d;
            List<HashSet<long>> v_map1 = new List<HashSet<long>>();
            List<HashSet<long>> v_map2 = new List<HashSet<long>>();
            Dictionary<int, int> v1 = new Dictionary<int, int>();
            Dictionary<int, int> v2 = new Dictionary<int, int>();
            v1.Add(1, 0);
            v2.Add(1, 0);
            int x, y;
            long footstep = 0L;  // Used to track overlapping paths.
            Dictionary<long, int> footsteps = new Dictionary<long, int>();
            bool done = false;
            // If the total number of characters is odd, then the front path will
            // collide with the reverse path.
            bool front = ((text1_length + text2_length) % 2 == 1);
            for (int d = 0; d < max_d; d++)
            {
                // Bail out if timeout reached.
                if (Diff_Timeout > 0 && DateTime.Now > ms_end)
                {
                    return null;
                }

                // Walk the front path one step.
                v_map1.Add(new HashSet<long>());  // Adds at index 'd'.
                for (int k = -d; k <= d; k += 2)
                {
                    if (k == -d || k != d && v1[k - 1] < v1[k + 1])
                    {
                        x = v1[k + 1];
                    }
                    else
                    {
                        x = v1[k - 1] + 1;
                    }
                    y = x - k;
                    if (doubleEnd)
                    {
                        footstep = diff_footprint(x, y);
                        if (front && (footsteps.ContainsKey(footstep)))
                        {
                            done = true;
                        }
                        if (!front)
                        {
                            footsteps.Add(footstep, d);
                        }
                    }
                    while (!done && x < text1_length && y < text2_length
                           && text1[x] == text2[y])
                    {
                        x++;
                        y++;
                        if (doubleEnd)
                        {
                            footstep = diff_footprint(x, y);
                            if (front && (footsteps.ContainsKey(footstep)))
                            {
                                done = true;
                            }
                            if (!front)
                            {
                                footsteps.Add(footstep, d);
                            }
                        }
                    }
                    if (v1.ContainsKey(k))
                    {
                        v1[k] = x;
                    }
                    else
                    {
                        v1.Add(k, x);
                    }
                    v_map1[d].Add(diff_footprint(x, y));
                    if (x == text1_length && y == text2_length)
                    {
                        // Reached the end in single-path mode.
                        return diff_path1(v_map1, text1, text2);
                    }
                    else if (done)
                    {
                        // Front path ran over reverse path.
                        v_map2 = v_map2.GetRange(0, footsteps[footstep] + 1);
                        List<Diff> a = diff_path1(v_map1, text1.Substring(0, x),
                                                        text2.Substring(0, y));
                        a.AddRange(diff_path2(v_map2, text1.Substring(x), text2.Substring(y)));
                        return a;
                    }
                }

                if (doubleEnd)
                {
                    // Walk the reverse path one step.
                    v_map2.Add(new HashSet<long>());  // Adds at index 'd'.
                    for (int k = -d; k <= d; k += 2)
                    {
                        if (k == -d || k != d && v2[k - 1] < v2[k + 1])
                        {
                            x = v2[k + 1];
                        }
                        else
                        {
                            x = v2[k - 1] + 1;
                        }
                        y = x - k;
                        footstep = diff_footprint(text1_length - x, text2_length - y);
                        if (!front && (footsteps.ContainsKey(footstep)))
                        {
                            done = true;
                        }
                        if (front)
                        {
                            footsteps.Add(footstep, d);
                        }
                        while (!done && x < text1_length && y < text2_length
                               && text1[text1_length - x - 1]
                               == text2[text2_length - y - 1])
                        {
                            x++;
                            y++;
                            footstep = diff_footprint(text1_length - x, text2_length - y);
                            if (!front && (footsteps.ContainsKey(footstep)))
                            {
                                done = true;
                            }
                            if (front)
                            {
                                footsteps.Add(footstep, d);
                            }
                        }
                        if (v2.ContainsKey(k))
                        {
                            v2[k] = x;
                        }
                        else
                        {
                            v2.Add(k, x);
                        }
                        v_map2[d].Add(diff_footprint(x, y));
                        if (done)
                        {
                            // Reverse path ran over front path.
                            v_map1 = v_map1.GetRange(0, footsteps[footstep] + 1);
                            List<Diff> a
                                = diff_path1(v_map1, text1.Substring(0, text1_length - x),
                                             text2.Substring(0, text2_length - y));
                            a.AddRange(diff_path2(v_map2, text1.Substring(text1_length - x),
                                                text2.Substring(text2_length - y)));
                            return a;
                        }
                    }
                }
            }
            // Number of diffs equals number of characters, no commonality at all.
            return null;
        }

        /**
         * Work from the middle back to the start to determine the path.
         * @param v_map List of path sets.
         * @param text1 Old string fragment to be diffed.
         * @param text2 New string fragment to be diffed.
         * @return LinkedList of Diff objects.
         */
        protected List<Diff> diff_path1(List<HashSet<long>> v_map,
                                        string text1, string text2)
        {
            LinkedList<Diff> path = new LinkedList<Diff>();
            int x = text1.Length;
            int y = text2.Length;
            Operation? last_op = null;
            for (int d = v_map.Count - 2; d >= 0; d--)
            {
                while (true)
                {
                    if (v_map[d].Contains(diff_footprint(x - 1, y)))
                    {
                        x--;
                        if (last_op == Operation.DELETE)
                        {
                            path.First().text = text1[x] + path.First().text;
                        }
                        else
                        {
                            path.AddFirst(new Diff(Operation.DELETE,
                                                   text1.Substring(x, 1)));
                        }
                        last_op = Operation.DELETE;
                        break;
                    }
                    else if (v_map[d].Contains(diff_footprint(x, y - 1)))
                    {
                        y--;
                        if (last_op == Operation.INSERT)
                        {
                            path.First().text = text2[y] + path.First().text;
                        }
                        else
                        {
                            path.AddFirst(new Diff(Operation.INSERT,
                                                   text2.Substring(y, 1)));
                        }
                        last_op = Operation.INSERT;
                        break;
                    }
                    else
                    {
                        x--;
                        y--;
                        //assert (text1.charAt(x) == text2.charAt(y))
                        //       : "No diagonal.  Can't happen. (diff_path1)";
                        if (last_op == Operation.EQUAL)
                        {
                            path.First().text = text1[x] + path.First().text;
                        }
                        else
                        {
                            path.AddFirst(new Diff(Operation.EQUAL, text1.Substring(x, 1)));
                        }
                        last_op = Operation.EQUAL;
                    }
                }
            }
            return path.ToList();
        }

        /**
         * Work from the middle back to the end to determine the path.
         * @param v_map List of path sets.
         * @param text1 Old string fragment to be diffed.
         * @param text2 New string fragment to be diffed.
         * @return LinkedList of Diff objects.
         */
        protected List<Diff> diff_path2(List<HashSet<long>> v_map,
                                        string text1, string text2)
        {
            LinkedList<Diff> path = new LinkedList<Diff>();
            int x = text1.Length;
            int y = text2.Length;
            Operation? last_op = null;
            for (int d = v_map.Count - 2; d >= 0; d--)
            {
                while (true)
                {
                    if (v_map[d].Contains(diff_footprint(x - 1, y)))
                    {
                        x--;
                        if (last_op == Operation.DELETE)
                        {
                            path.Last().text += text1[text1.Length - x - 1];
                        }
                        else
                        {
                            path.AddLast(new Diff(Operation.DELETE,
                                text1.Substring(text1.Length - x - 1, 1)));
                        }
                        last_op = Operation.DELETE;
                        break;
                    }
                    else if (v_map[d].Contains(diff_footprint(x, y - 1)))
                    {
                        y--;
                        if (last_op == Operation.INSERT)
                        {
                            path.Last().text += text2[text2.Length - y - 1];
                        }
                        else
                        {
                            path.AddLast(new Diff(Operation.INSERT,
                                text2.Substring(text2.Length - y - 1, 1)));
                        }
                        last_op = Operation.INSERT;
                        break;
                    }
                    else
                    {
                        x--;
                        y--;
                        //assert (text1.charAt(text1.Length - x - 1)
                        //        == text2.charAt(text2.Length - y - 1))
                        //      : "No diagonal.  Can't happen. (diff_path2)";
                        if (last_op == Operation.EQUAL)
                        {
                            path.Last().text += text1[text1.Length - x - 1];
                        }
                        else
                        {
                            path.AddLast(new Diff(Operation.EQUAL,
                                text1.Substring(text1.Length - x - 1, 1)));
                        }
                        last_op = Operation.EQUAL;
                    }
                }
            }
            return path.ToList();
        }

        /**
         * Compute a good hash of two integers.
         * @param x First int.
         * @param y Second int.
         * @return A long made up of both ints.
         */
        protected long diff_footprint(int x, int y)
        {
            // The maximum size for a long is 9,223,372,036,854,775,807
            // The maximum size for an int is 2,147,483,647
            // Two ints fit nicely in one long.
            long result = x;
            result = result << 32;
            result += y;
            return result;
        }

        /**
         * Determine the common prefix of two strings
         * @param text1 First string.
         * @param text2 Second string.
         * @return The number of characters common to the start of each string.
         */
        public int diff_commonPrefix(string text1, string text2)
        {
            // Performance analysis: http://neil.fraser.name/news/2007/10/09/
            int n = Math.Min(text1.Length, text2.Length);
            for (int i = 0; i < n; i++)
            {
                if (text1[i] != text2[i])
                {
                    return i;
                }
            }
            return n;
        }

        /**
         * Determine the common suffix of two strings
         * @param text1 First string.
         * @param text2 Second string.
         * @return The number of characters common to the end of each string.
         */
        public int diff_commonSuffix(string text1, string text2)
        {
            // Performance analysis: http://neil.fraser.name/news/2007/10/09/
            int text1_length = text1.Length;
            int text2_length = text2.Length;
            int n = Math.Min(text1.Length, text2.Length);
            for (int i = 1; i <= n; i++)
            {
                if (text1[text1_length - i] != text2[text2_length - i])
                {
                    return i - 1;
                }
            }
            return n;
        }

        /**
         * Do the two texts share a Substring which is at least half the length of
         * the longer text?
         * @param text1 First string.
         * @param text2 Second string.
         * @return Five element String array, containing the prefix of text1, the
         *     suffix of text1, the prefix of text2, the suffix of text2 and the
         *     common middle.  Or null if there was no match.
         */

        protected string[] diff_halfMatch(string text1, string text2)
        {
            string longtext = text1.Length > text2.Length ? text1 : text2;
            string shorttext = text1.Length > text2.Length ? text2 : text1;
            if (longtext.Length < 10 || shorttext.Length < 1)
            {
                return null;  // Pointless.
            }

            // First check if the second quarter is the seed for a half-match.
            string[] hm1 = diff_halfMatchI(longtext, shorttext,
                                           (longtext.Length + 3) / 4);
            // Check again based on the third quarter.
            string[] hm2 = diff_halfMatchI(longtext, shorttext,
                                           (longtext.Length + 1) / 2);
            string[] hm;
            if (hm1 == null && hm2 == null)
            {
                return null;
            }
            else if (hm2 == null)
            {
                hm = hm1;
            }
            else if (hm1 == null)
            {
                hm = hm2;
            }
            else
            {
                // Both matched.  Select the longest.
                hm = hm1[4].Length > hm2[4].Length ? hm1 : hm2;
            }

            // A half-match was found, sort out the return data.
            if (text1.Length > text2.Length)
            {
                return hm;
                //return new string[]{hm[0], hm[1], hm[2], hm[3], hm[4]};
            }
            else
            {
                return new string[] { hm[2], hm[3], hm[0], hm[1], hm[4] };
            }
        }

        /**
         * Does a Substring of shorttext exist within longtext such that the
         * Substring is at least half the length of longtext?
         * @param longtext Longer string.
         * @param shorttext Shorter string.
         * @param i Start index of quarter length Substring within longtext.
         * @return Five element string array, containing the prefix of longtext, the
         *     suffix of longtext, the prefix of shorttext, the suffix of shorttext
         *     and the common middle.  Or null if there was no match.
         */
        private string[] diff_halfMatchI(string longtext, string shorttext, int i)
        {
            // Start with a 1/4 length Substring at position i as a seed.
            string seed = longtext.Substring(i, longtext.Length / 4);
            int j = -1;
            string best_common = string.Empty;
            string best_longtext_a = string.Empty, best_longtext_b = string.Empty;
            string best_shorttext_a = string.Empty, best_shorttext_b = string.Empty;
            while (j < shorttext.Length && (j = shorttext.IndexOf(seed, j + 1)) != -1)
            {
                int prefixLength = diff_commonPrefix(longtext.Substring(i),
                                                     shorttext.Substring(j));
                int suffixLength = diff_commonSuffix(longtext.Substring(0, i),
                                                     shorttext.Substring(0, j));
                if (best_common.Length < suffixLength + prefixLength)
                {
                    best_common = shorttext.Substring(j - suffixLength, suffixLength)
                        + shorttext.Substring(j, prefixLength);
                    best_longtext_a = longtext.Substring(0, i - suffixLength);
                    best_longtext_b = longtext.Substring(i + prefixLength);
                    best_shorttext_a = shorttext.Substring(0, j - suffixLength);
                    best_shorttext_b = shorttext.Substring(j + prefixLength);
                }
            }
            if (best_common.Length >= longtext.Length / 2)
            {
                return new string[]{best_longtext_a, best_longtext_b,
                          best_shorttext_a, best_shorttext_b, best_common};
            }
            else
            {
                return null;
            }
        }

        /**
         * Reduce the number of edits by eliminating semantically trivial equalities.
         * @param diffs LinkedList of Diff objects.
         */
        public void diff_cleanupSemantic(List<Diff> diffs)
        {
            var changes = false;
            var equalities = new Stack<int>();  // Stack of indices where equalities are found.
            string lastequality = null;  // Always equal to equalities[equalitiesLength-1][1]
            var pointer = 0;  // Index of current position.
            // Number of characters that changed prior to the equality.
            var length_changes1 = 0;
            // Number of characters that changed after the equality.
            var length_changes2 = 0;
            while (pointer < diffs.Count)
            {
                if (diffs[pointer].operation == Operation.EQUAL)
                {  // equality found
                    equalities.Push(pointer);
                    length_changes1 = length_changes2;
                    length_changes2 = 0;
                    lastequality = diffs[pointer].text;
                }
                else
                {  // an insertion or deletion
                    length_changes2 += diffs[pointer].text.Length;
                    if (lastequality != null && (lastequality.Length <= length_changes1) &&
                        (lastequality.Length <= length_changes2))
                    {
                        // Duplicate record
                        diffs.Insert(equalities.Peek(),
                                     new Diff(Operation.DELETE, lastequality));
                        // Change second copy to insert.
                        diffs[equalities.Peek() + 1].operation = Operation.INSERT;
                        // Throw away the equality we just deleted.
                        equalities.Pop();
                        if (equalities.Count > 0)
                        {
                            equalities.Pop();
                        }
                        pointer = equalities.Count > 0 ? equalities.Peek() : -1;
                        length_changes1 = 0;  // Reset the counters.
                        length_changes2 = 0;
                        lastequality = null;
                        changes = true;
                    }
                }
                pointer++;
            }
            if (changes)
            {
                diff_cleanupMerge(diffs);
            }
            diff_cleanupSemanticLossless(diffs);
        }

        /**
         * Look for single edits surrounded on both sides by equalities
         * which can be shifted sideways to align the edit to a word boundary.
         * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
         * @param diffs LinkedList of Diff objects.
         */
        public void diff_cleanupSemanticLossless(List<Diff> diffs)
        {
            var pointer = 1;
            // Intentionally ignore the first and last element (don't need checking).
            while (pointer < diffs.Count - 1)
            {
                if (diffs[pointer - 1].operation == Operation.EQUAL &&
                    diffs[pointer + 1].operation == Operation.EQUAL)
                {
                    // This is a single edit surrounded by equalities.
                    var equality1 = diffs[pointer - 1].text;
                    var edit = diffs[pointer].text;
                    var equality2 = diffs[pointer + 1].text;

                    // First, shift the edit as far left as possible.
                    var commonOffset = this.diff_commonSuffix(equality1, edit);
                    if (commonOffset > 0)
                    {
                        var commonString = edit.Substring(edit.Length - commonOffset);
                        equality1 = equality1.Substring(0, equality1.Length - commonOffset);
                        edit = commonString + edit.Substring(0, edit.Length - commonOffset);
                        equality2 = commonString + equality2;
                    }

                    // Second, step character by character right, looking for the best fit.
                    var bestEquality1 = equality1;
                    var bestEdit = edit;
                    var bestEquality2 = equality2;
                    var bestScore = diff_cleanupSemanticScore(equality1, edit) +
                        diff_cleanupSemanticScore(edit, equality2);
                    while (edit.Length != 0 && equality2.Length != 0
                        && edit[0] == equality2[0])
                    {
                        equality1 += edit[0];
                        edit = edit.Substring(1) + equality2[0];
                        equality2 = equality2.Substring(1);
                        var score = diff_cleanupSemanticScore(equality1, edit) +
                            diff_cleanupSemanticScore(edit, equality2);
                        // The >= encourages trailing rather than leading whitespace on edits.
                        if (score >= bestScore)
                        {
                            bestScore = score;
                            bestEquality1 = equality1;
                            bestEdit = edit;
                            bestEquality2 = equality2;
                        }
                    }

                    if (diffs[pointer - 1].text != bestEquality1)
                    {
                        // We have an improvement, save it back to the diff.
                        if (bestEquality1.Length != 0)
                        {
                            diffs[pointer - 1].text = bestEquality1;
                        }
                        else
                        {
                            diffs.RemoveAt(pointer - 1);
                            pointer--;
                        }
                        diffs[pointer].text = bestEdit;
                        if (bestEquality2.Length != 0)
                        {
                            diffs[pointer + 1].text = bestEquality2;
                        }
                        else
                        {
                            diffs.RemoveAt(pointer + 1);
                            pointer--;
                        }
                    }
                }
                pointer++;
            }
        }

        /**
         * Given two strings, comAdde a score representing whether the internal
         * boundary falls on logical boundaries.
         * Scores range from 5 (best) to 0 (worst).
         * @param one First string.
         * @param two Second string.
         * @return The score.
         */
        private int diff_cleanupSemanticScore(string one, string two)
        {
            if (one.Length == 0 || two.Length == 0)
            {
                // Edges are the best.
                return 5;
            }

            // Each port of this function behaves slightly differently due to
            // subtle differences in each language's definition of things like
            // 'whitespace'.  Since this function's purpose is largely cosmetic,
            // the choice has been made to use each language's native features
            // rather than force total conformity.
            int score = 0;
            // One point for non-alphanumeric.
            if (!Char.IsLetterOrDigit(one[one.Length - 1])
                || !Char.IsLetterOrDigit(two[0]))
            {
                score++;
                // Two points for whitespace.
                if (Char.IsWhiteSpace(one[one.Length - 1])
                    || Char.IsWhiteSpace(two[0]))
                {
                    score++;
                    // Three points for line breaks.
                    if (Char.IsControl(one[one.Length - 1])
                        || Char.IsControl(two[0]))
                    {
                        score++;
                        // Four points for blank lines.
                        if (BLANKLINEEND.IsMatch(one)
                            || BLANKLINESTART.IsMatch(two))
                        {
                            score++;
                        }
                    }
                }
            }
            return score;
        }

        private Regex BLANKLINEEND
            = new Regex("\\n\\r?\\n\\Z");
        private Regex BLANKLINESTART
            = new Regex("\\A\\r?\\n\\r?\\n");

        /**
         * Reduce the number of edits by eliminating operationally trivial equalities.
         * @param diffs LinkedList of Diff objects.
         */
        public void diff_cleanupEfficiency(List<Diff> diffs)
        {
            var changes = false;
            var equalities = new Stack<int>();  // Stack of indices where equalities are found.
            string lastequality = string.Empty;  // Always equal to equalities[equalitiesLength-1][1]
            var pointer = 0;  // Index of current position.
            // Is there an insertion operation before the last equality.
            var pre_ins = false;
            // Is there a deletion operation before the last equality.
            var pre_del = false;
            // Is there an insertion operation after the last equality.
            var post_ins = false;
            // Is there a deletion operation after the last equality.
            var post_del = false;
            while (pointer < diffs.Count)
            {
                if (diffs[pointer].operation == Operation.EQUAL)
                {  // equality found
                    if (diffs[pointer].text.Length < this.Diff_EditCost &&
                        (post_ins || post_del))
                    {
                        // Candidate found.
                        equalities.Push(pointer);
                        pre_ins = post_ins;
                        pre_del = post_del;
                        lastequality = diffs[pointer].text;
                    }
                    else
                    {
                        // Not a candidate, and can never become one.
                        equalities.Clear();
                        lastequality = string.Empty;
                    }
                    post_ins = post_del = false;
                }
                else
                {  // an insertion or deletion
                    if (diffs[pointer].operation == Operation.DELETE)
                    {
                        post_del = true;
                    }
                    else
                    {
                        post_ins = true;
                    }
                    /*
                     * Five types to be split:
                     * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
                     * <ins>A</ins>X<ins>C</ins><del>D</del>
                     * <ins>A</ins><del>B</del>X<ins>C</ins>
                     * <ins>A</del>X<ins>C</ins><del>D</del>
                     * <ins>A</ins><del>B</del>X<del>C</del>
                     */
                    if ((lastequality.Length != 0) && ((pre_ins && pre_del && post_ins && post_del) ||
                        ((lastequality.Length < this.Diff_EditCost / 2) &&
                        ((pre_ins ? 1 : 0) + (pre_del ? 1 : 0) + (post_ins ? 1 : 0) + (post_del ? 1 : 0)) == 3)))
                    {
                        // Duplicate record
                        diffs.Insert(equalities.Peek(), new Diff(Operation.DELETE, lastequality));
                        // Change second copy to insert.
                        diffs[equalities.Peek() + 1].operation = Operation.INSERT;
                        equalities.Pop();  // Throw away the equality we just deleted;
                        lastequality = string.Empty;
                        if (pre_ins && pre_del)
                        {
                            // No changes made which could affect previous entry, keep going.
                            post_ins = post_del = true;
                            equalities.Clear();
                        }
                        else
                        {
                            if (equalities.Count > 0)
                            {
                                equalities.Pop();
                            }

                            pointer = equalities.Count > 0 ? equalities.Peek() : -1;
                            post_ins = post_del = false;
                        }
                        changes = true;
                    }
                }
                pointer++;
            }

            if (changes)
            {
                diff_cleanupMerge(diffs);
            }
        }

        /**
         * Reorder and merge like edit sections.  Merge equalities.
         * Any edit section can move as long as it doesn't cross an equality.
         * @param diffs LinkedList of Diff objects.
         */
        public void diff_cleanupMerge(List<Diff> diffs)
        {
            diffs.Add(new Diff(Operation.EQUAL, string.Empty));  // Add a dummy entry at the end.
            var pointer = 0;
            var count_delete = 0;
            var count_insert = 0;
            var text_delete = string.Empty;
            var text_insert = string.Empty;
            int commonlength;
            while (pointer < diffs.Count)
            {
                switch (diffs[pointer].operation)
                {
                    case Operation.INSERT:
                        count_insert++;
                        text_insert += diffs[pointer].text;
                        pointer++;
                        break;
                    case Operation.DELETE:
                        count_delete++;
                        text_delete += diffs[pointer].text;
                        pointer++;
                        break;
                    case Operation.EQUAL:
                        // Upon reaching an equality, check for prior redundancies.
                        if (count_delete != 0 || count_insert != 0)
                        {
                            if (count_delete != 0 && count_insert != 0)
                            {
                                // Factor out any common prefixies.
                                commonlength = this.diff_commonPrefix(text_insert, text_delete);
                                if (commonlength != 0)
                                {
                                    if ((pointer - count_delete - count_insert) > 0 &&
                                        diffs[pointer - count_delete - count_insert - 1].operation ==
                                        Operation.EQUAL)
                                    {
                                        diffs[pointer - count_delete - count_insert - 1].text +=
                                            text_insert.Substring(0, commonlength);
                                    }
                                    else
                                    {
                                        diffs.Insert(0, new Diff(Operation.EQUAL,
                                            text_insert.Substring(0, commonlength)));
                                        pointer++;
                                    }
                                    text_insert = text_insert.Substring(commonlength);
                                    text_delete = text_delete.Substring(commonlength);
                                }
                                // Factor out any common suffixies.
                                commonlength = this.diff_commonSuffix(text_insert, text_delete);
                                if (commonlength != 0)
                                {
                                    diffs[pointer].text = text_insert.Substring(text_insert.Length -
                                        commonlength) + diffs[pointer].text;
                                    text_insert = text_insert.Substring(0, text_insert.Length -
                                        commonlength);
                                    text_delete = text_delete.Substring(0, text_delete.Length -
                                        commonlength);
                                }
                            }
                            // Delete the offending records and add the merged ones.
                            if (count_delete == 0)
                            {
                                diffs.Splice(pointer - count_delete - count_insert,
                                    count_delete + count_insert, new Diff(Operation.INSERT, text_insert));
                            }
                            else if (count_insert == 0)
                            {
                                diffs.Splice(pointer - count_delete - count_insert,
                                    count_delete + count_insert, new Diff(Operation.DELETE, text_delete));
                            }
                            else
                            {
                                diffs.Splice(pointer - count_delete - count_insert,
                                    count_delete + count_insert, new Diff(Operation.DELETE, text_delete),
                                    new Diff(Operation.INSERT, text_insert));
                            }
                            pointer = pointer - count_delete - count_insert +
                                      (count_delete != 0 ? 1 : 0) + (count_insert != 0 ? 1 : 0) + 1;
                        }
                        else if (pointer != 0 && diffs[pointer - 1].operation == Operation.EQUAL)
                        {
                            // Merge this equality with the previous one.
                            diffs[pointer - 1].text += diffs[pointer].text;
                            diffs.RemoveAt(pointer);
                        }
                        else
                        {
                            pointer++;
                        }
                        count_insert = 0;
                        count_delete = 0;
                        text_delete = string.Empty;
                        text_insert = string.Empty;
                        break;
                }
            }
            if (diffs[diffs.Count - 1].text.Length == 0)
            {
                diffs.RemoveAt(diffs.Count - 1);  // Remove the dummy entry at the end.
            }

            // Second pass: look for single edits surrounded on both sides by equalities
            // which can be shifted sideways to eliminate an equality.
            // e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
            var changes = false;
            pointer = 1;
            // Intentionally ignore the first and last element (don't need checking).
            while (pointer < (diffs.Count - 1))
            {
                if (diffs[pointer - 1].operation == Operation.EQUAL &&
                    diffs[pointer + 1].operation == Operation.EQUAL)
                {
                    // This is a single edit surrounded by equalities.
                    if (diffs[pointer].text.EndsWith(diffs[pointer - 1].text))
                    {
                        // Shift the edit over the previous equality.
                        diffs[pointer].text = diffs[pointer - 1].text +
                            diffs[pointer].text.Substring(0, diffs[pointer].text.Length -
                                                        diffs[pointer - 1].text.Length);
                        diffs[pointer + 1].text = diffs[pointer - 1].text + diffs[pointer + 1].text;
                        diffs.Splice(pointer - 1, 1);
                        changes = true;
                    }
                    else if (diffs[pointer].text.StartsWith(diffs[pointer + 1].text))
                    {
                        // Shift the edit over the next equality.
                        diffs[pointer - 1].text += diffs[pointer + 1].text;
                        diffs[pointer].text =
                            diffs[pointer].text.Substring(diffs[pointer + 1].text.Length) +
                            diffs[pointer + 1].text;
                        diffs.Splice(pointer + 1, 1);
                        changes = true;
                    }
                }
                pointer++;
            }
            // If shifts were made, the diff needs reordering and another shift sweep.
            if (changes)
            {
                this.diff_cleanupMerge(diffs);
            }
        }

        /**
         * loc is a location in text1, comAdde and return the equivalent location in
         * text2.
         * e.g. "The cat" vs "The big cat", 1->1, 5->8
         * @param diffs LinkedList of Diff objects.
         * @param loc Location within text1.
         * @return Location within text2.
         */
        public int diff_xIndex(List<Diff> diffs, int loc)
        {
            int chars1 = 0;
            int chars2 = 0;
            int last_chars1 = 0;
            int last_chars2 = 0;
            Diff lastDiff = null;
            foreach (Diff aDiff in diffs)
            {
                if (aDiff.operation != Operation.INSERT)
                {
                    // Equality or deletion.
                    chars1 += aDiff.text.Length;
                }
                if (aDiff.operation != Operation.DELETE)
                {
                    // Equality or insertion.
                    chars2 += aDiff.text.Length;
                }
                if (chars1 > loc)
                {
                    // Overshot the location.
                    lastDiff = aDiff;
                    break;
                }
                last_chars1 = chars1;
                last_chars2 = chars2;
            }
            if (lastDiff != null && lastDiff.operation == Operation.DELETE)
            {
                // The location was deleted.
                return last_chars2;
            }
            // Add the remaining character length.
            return last_chars2 + (loc - last_chars1);
        }

        /**
         * Convert a Diff list into a pretty HTML report.
         * @param diffs LinkedList of Diff objects.
         * @return HTML representation.
         */
        public string diff_prettyHtml(List<Diff> diffs)
        {
            StringBuilder html = new StringBuilder();
            int i = 0;
            foreach (Diff aDiff in diffs)
            {
                string text = aDiff.text.Replace("&", "&amp;").Replace("<", "&lt;")
                    .Replace(">", "&gt;").Replace("\n", "&para;<BR>");
                switch (aDiff.operation)
                {
                    case Operation.INSERT:
                        html.Append("<INS STYLE=\"background:#E6FFE6;\" TITLE=\"i=").Append(i)
                            .Append("\">").Append(text).Append("</INS>");
                        break;
                    case Operation.DELETE:
                        html.Append("<DEL STYLE=\"background:#FFE6E6;\" TITLE=\"i=").Append(i)
                            .Append("\">").Append(text).Append("</DEL>");
                        break;
                    case Operation.EQUAL:
                        html.Append("<SPAN TITLE=\"i=").Append(i).Append("\">").Append(text)
                            .Append("</SPAN>");
                        break;
                }
                if (aDiff.operation != Operation.DELETE)
                {
                    i += aDiff.text.Length;
                }
            }
            return html.ToString();
        }

        /**
         * Compute and return the source text (all equalities and deletions).
         * @param diffs LinkedList of Diff objects.
         * @return Source text.
         */
        public string diff_text1(List<Diff> diffs)
        {
            StringBuilder text = new StringBuilder();
            foreach (Diff aDiff in diffs)
            {
                if (aDiff.operation != Operation.INSERT)
                {
                    text.Append(aDiff.text);
                }
            }
            return text.ToString();
        }

        /**
         * Compute and return the destination text (all equalities and insertions).
         * @param diffs LinkedList of Diff objects.
         * @return Destination text.
         */
        public string diff_text2(List<Diff> diffs)
        {
            StringBuilder text = new StringBuilder();
            foreach (Diff aDiff in diffs)
            {
                if (aDiff.operation != Operation.DELETE)
                {
                    text.Append(aDiff.text);
                }
            }
            return text.ToString();
        }

        /**
         * Compute the Levenshtein distance; the number of inserted, deleted or
         * substituted characters.
         * @param diffs LinkedList of Diff objects.
         * @return Number of changes.
         */
        public int diff_levenshtein(List<Diff> diffs)
        {
            int levenshtein = 0;
            int insertions = 0;
            int deletions = 0;
            foreach (Diff aDiff in diffs)
            {
                switch (aDiff.operation)
                {
                    case Operation.INSERT:
                        insertions += aDiff.text.Length;
                        break;
                    case Operation.DELETE:
                        deletions += aDiff.text.Length;
                        break;
                    case Operation.EQUAL:
                        // A deletion and an insertion is one substitution.
                        levenshtein += Math.Max(insertions, deletions);
                        insertions = 0;
                        deletions = 0;
                        break;
                }
            }
            levenshtein += Math.Max(insertions, deletions);
            return levenshtein;
        }

        /**
         * Crush the diff into an encoded string which describes the operations
         * required to transform text1 into text2.
         * E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
         * Operations are tab-separated.  Inserted text is escaped using %xx notation.
         * @param diffs Array of diff tuples.
         * @return Delta text.
         */
        public string diff_toDelta(List<Diff> diffs)
        {
            StringBuilder text = new StringBuilder();
            foreach (Diff aDiff in diffs)
            {
                switch (aDiff.operation)
                {
                    case Operation.INSERT:
                        text.Append("+").Append(HttpUtility.UrlEncode(aDiff.text, new UTF8Encoding())
                                                            .Replace('+', ' ')).Append("\t");

                        break;
                    case Operation.DELETE:
                        text.Append("-").Append(aDiff.text.Length).Append("\t");
                        break;
                    case Operation.EQUAL:
                        text.Append("=").Append(aDiff.text.Length).Append("\t");
                        break;
                }
            }
            string delta = text.ToString();
            if (delta.Length != 0)
            {
                // Strip off trailing tab character.
                delta = delta.Substring(0, delta.Length - 1);
                delta = unescapeForEncodeUriCompatability(delta);
            }
            return delta;
        }

        /**
         * Given the original text1, and an encoded string which describes the
         * operations required to transform text1 into text2, comAdde the full diff.
         * @param text1 Source string for the diff.
         * @param delta Delta text.
         * @return Array of diff tuples or null if invalid.
         * @throws ArgumentException If invalid input.
         */
        public List<Diff> diff_fromDelta(string text1, string delta)
        {
            List<Diff> diffs = new List<Diff>();
            int pointer = 0;  // Cursor in text1
            string[] tokens = delta.Split(new string[] { "\t" }, StringSplitOptions.None);
            foreach (string token in tokens)
            {
                if (token.Length == 0)
                {
                    // Blank tokens are ok (from a trailing \t).
                    continue;
                }
                // Each token begins with a one character parameter which specifies the
                // operation of this token (delete, insert, equality).
                string param = token.Substring(1);
                switch (token[0])
                {
                    case '+':
                        // decode would change all "+" to " "
                        param = param.Replace("+", "%2b");

                        param = HttpUtility.UrlDecode(param, new UTF8Encoding(false, true));
                        //} catch (UnsupportedEncodingException e) {
                        //  // Not likely on modern system.
                        //  throw new Error("This system does not support UTF-8.", e);
                        //} catch (IllegalArgumentException e) {
                        //  // Malformed URI sequence.
                        //  throw new IllegalArgumentException(
                        //      "Illegal escape in diff_fromDelta: " + param, e);
                        //}
                        diffs.Add(new Diff(Operation.INSERT, param));
                        break;
                    case '-':
                    // Fall through.
                    case '=':
                        int n;
                        try
                        {
                            n = Convert.ToInt32(param);
                        }
                        catch (FormatException e)
                        {
                            throw new ArgumentException(
                                "Invalid number in diff_fromDelta: " + param, e);
                        }
                        if (n < 0)
                        {
                            throw new ArgumentException(
                                "Negative number in diff_fromDelta: " + param);
                        }
                        string text;
                        try
                        {
                            text = text1.Substring(pointer, n);
                            pointer += n;
                        }
                        catch (ArgumentOutOfRangeException e)
                        {
                            throw new ArgumentException("Delta length (" + pointer
                                + ") larger than source text length (" + text1.Length
                                + ").", e);
                        }
                        if (token[0] == '=')
                        {
                            diffs.Add(new Diff(Operation.EQUAL, text));
                        }
                        else
                        {
                            diffs.Add(new Diff(Operation.DELETE, text));
                        }
                        break;
                    default:
                        // Anything else is an error.
                        throw new ArgumentException(
                            "Invalid diff operation in diff_fromDelta: " + token[0]);
                }
            }
            if (pointer != text1.Length)
            {
                throw new ArgumentException("Delta length (" + pointer
                    + ") smaller than source text length (" + text1.Length + ").");
            }
            return diffs;
        }


        //  MATCH FUNCTIONS


        /**
         * Locate the best instance of 'pattern' in 'text' near 'loc'.
         * Returns -1 if no match found.
         * @param text The text to search.
         * @param pattern The pattern to search for.
         * @param loc The location to search around.
         * @return Best match index or -1.
         */
        public int match_main(string text, string pattern, int loc)
        {
            loc = Math.Max(0, Math.Min(loc, text.Length));
            if (text == pattern)
            {
                // Shortcut (potentially not guaranteed by the algorithm)
                return 0;
            }
            else if (text.Length == 0)
            {
                // Nothing to match.
                return -1;
            }
            else if (loc + pattern.Length <= text.Length
                && text.Substring(loc, pattern.Length) == pattern)
            {
                // Perfect match at the perfect spot!  (Includes case of null pattern)
                return loc;
            }
            else
            {
                // Do a fuzzy compare.
                return match_bitap(text, pattern, loc);
            }
        }

        /**
         * Locate the best instance of 'pattern' in 'text' near 'loc' using the
         * Bitap algorithm.  Returns -1 if no match found.
         * @param text The text to search.
         * @param pattern The pattern to search for.
         * @param loc The location to search around.
         * @return Best match index or -1.
         */
        protected int match_bitap(string text, string pattern, int loc)
        {
            // assert (Match_MaxBits == 0 || pattern.Length <= Match_MaxBits)
            //    : "Pattern too long for this application.";

            // Initialise the alphabet.
            Dictionary<char, int> s = match_alphabet(pattern);

            // Highest score beyond which we give up.
            double score_threshold = Match_Threshold;
            // Is there a nearby exact match? (speedup)
            int best_loc = text.IndexOf(pattern, loc);
            if (best_loc != -1)
            {
                score_threshold = Math.Min(match_bitapScore(0, best_loc, loc,
                    pattern), score_threshold);
                // What about in the other direction? (speedup)
                best_loc = text.LastIndexOf(pattern,
                    Math.Min(loc + pattern.Length, text.Length));
                if (best_loc != -1)
                {
                    score_threshold = Math.Min(match_bitapScore(0, best_loc, loc,
                        pattern), score_threshold);
                }
            }

            // Initialise the bit arrays.
            int matchmask = 1 << (pattern.Length - 1);
            best_loc = -1;

            int bin_min, bin_mid;
            int bin_max = pattern.Length + text.Length;
            // Empty initialization added to appease C# compiler.
            int[] last_rd = new int[0];
            for (int d = 0; d < pattern.Length; d++)
            {
                // Scan for the best match; each iteration allows for one more error.
                // Run a binary search to determine how far from 'loc' we can stray at
                // this error level.
                bin_min = 0;
                bin_mid = bin_max;
                while (bin_min < bin_mid)
                {
                    if (match_bitapScore(d, loc + bin_mid, loc, pattern)
                        <= score_threshold)
                    {
                        bin_min = bin_mid;
                    }
                    else
                    {
                        bin_max = bin_mid;
                    }
                    bin_mid = (bin_max - bin_min) / 2 + bin_min;
                }
                // Use the result from this iteration as the maximum for the next.
                bin_max = bin_mid;
                int start = Math.Max(1, loc - bin_mid + 1);
                int finish = Math.Min(loc + bin_mid, text.Length) + pattern.Length;

                int[] rd = new int[finish + 2];
                rd[finish + 1] = (1 << d) - 1;
                for (int j = finish; j >= start; j--)
                {
                    int charMatch;
                    if (text.Length <= j - 1 || !s.ContainsKey(text[j - 1]))
                    {
                        // Out of range.
                        charMatch = 0;
                    }
                    else
                    {
                        charMatch = s[text[j - 1]];
                    }
                    if (d == 0)
                    {
                        // First pass: exact match.
                        rd[j] = ((rd[j + 1] << 1) | 1) & charMatch;
                    }
                    else
                    {
                        // Subsequent passes: fuzzy match.
                        rd[j] = ((rd[j + 1] << 1) | 1) & charMatch
                            | (((last_rd[j + 1] | last_rd[j]) << 1) | 1)
                            | last_rd[j + 1];
                    }
                    if ((rd[j] & matchmask) != 0)
                    {
                        double score = match_bitapScore(d, j - 1, loc, pattern);
                        // This match will almost certainly be better than any existing
                        // match.  But check anyway.
                        if (score <= score_threshold)
                        {
                            // Told you so.
                            score_threshold = score;
                            best_loc = j - 1;
                            if (best_loc > loc)
                            {
                                // When passing loc, don't exceed our current distance from loc.
                                start = Math.Max(1, 2 * loc - best_loc);
                            }
                            else
                            {
                                // Already passed loc, downhill from here on in.
                                break;
                            }
                        }
                    }
                }
                if (match_bitapScore(d + 1, loc, loc, pattern) > score_threshold)
                {
                    // No hope for a (better) match at greater error levels.
                    break;
                }
                last_rd = rd;
            }
            return best_loc;
        }

        /**
         * Compute and return the score for a match with e errors and x location.
         * @param e Number of errors in match.
         * @param x Location of match.
         * @param loc Expected location of match.
         * @param pattern Pattern being sought.
         * @return Overall score for match (0.0 = good, 1.0 = bad).
         */
        private double match_bitapScore(int e, int x, int loc, string pattern)
        {
            float accuracy = (float)e / pattern.Length;
            int proximity = Math.Abs(loc - x);
            if (Match_Distance == 0)
            {
                // Dodge divide by zero error.
                return proximity == 0 ? accuracy : 1.0;
            }
            return accuracy + (proximity / (float) Match_Distance);
        }

        /**
         * Initialise the alphabet for the Bitap algorithm.
         * @param pattern The text to encode.
         * @return Hash of character locations.
         */
        protected Dictionary<char, int> match_alphabet(string pattern)
        {
            Dictionary<char, int> s = new Dictionary<char, int>();
            char[] char_pattern = pattern.ToCharArray();
            foreach (char c in char_pattern)
            {
                if (!s.ContainsKey(c))
                {
                    s.Add(c, 0);
                }
            }
            int i = 0;
            foreach (char c in char_pattern)
            {
                var value = s[c] | (1 << (pattern.Length - i - 1));
                s[c] = value;
                i++;
            }
            return s;
        }


        //  PATCH FUNCTIONS


        /**
         * Increase the context until it is unique,
         * but don't let the pattern expand beyond Match_MaxBits.
         * @param patch The patch to grow.
         * @param text Source text.
         */
        protected void patch_addContext(Patch patch, string text)
        {
            if (text.Length == 0)
            {
                return;
            }
            string pattern = text.Substring(patch.start2, patch.length1);
            int padding = 0;

            // Look for the first and last matches of pattern in text.  If two
            // different matches are found, increase the pattern length.
            while (text.IndexOf(pattern) != text.LastIndexOf(pattern)
                && pattern.Length < Match_MaxBits - Patch_Margin - Patch_Margin)
            {
                padding += Patch_Margin;
                pattern = text.JavaSubstring(Math.Max(0, patch.start2 - padding),
                    Math.Min(text.Length, patch.start2 + patch.length1 + padding));
            }
            // Add one chunk for good luck.
            padding += Patch_Margin;

            // Add the prefix.
            string prefix = text.JavaSubstring(Math.Max(0, patch.start2 - padding),
                patch.start2);
            if (prefix.Length != 0)
            {
                patch.diffs.Insert(0, new Diff(Operation.EQUAL, prefix));
            }
            // Add the suffix.
            string suffix = text.JavaSubstring(patch.start2 + patch.length1,
                Math.Min(text.Length, patch.start2 + patch.length1 + padding));
            if (suffix.Length != 0)
            {
                patch.diffs.Add(new Diff(Operation.EQUAL, suffix));
            }

            // Roll back the start points.
            patch.start1 -= prefix.Length;
            patch.start2 -= prefix.Length;
            // Extend the lengths.
            patch.length1 += prefix.Length + suffix.Length;
            patch.length2 += prefix.Length + suffix.Length;
        }

        /**
         * Compute a list of patches to turn text1 into text2.
         * A set of diffs will be computed.
         * @param text1 Old text.
         * @param text2 New text.
         * @return LinkedList of Patch objects.
         */
        public List<Patch> patch_make(string text1, string text2)
        {
            // No diffs provided, comAdde our own.
            List<Diff> diffs = diff_main(text1, text2, true);
            if (diffs.Count > 2)
            {
                diff_cleanupSemantic(diffs);
                diff_cleanupEfficiency(diffs);
            }
            return patch_make(text1, diffs);
        }

        /**
         * Compute a list of patches to turn text1 into text2.
         * text1 will be derived from the provided diffs.
         * @param diffs Array of diff tuples for text1 to text2.
         * @return LinkedList of Patch objects.
         */
        public List<Patch> patch_make(List<Diff> diffs)
        {
            // No origin string provided, comAdde our own.
            string text1 = diff_text1(diffs);
            return patch_make(text1, diffs);
        }

        /**
         * Compute a list of patches to turn text1 into text2.
         * text2 is ignored, diffs are the delta between text1 and text2.
         * @param text1 Old text
         * @param text2 Ignored.
         * @param diffs Array of diff tuples for text1 to text2.
         * @return LinkedList of Patch objects.
         * @deprecated Prefer patch_make(string text1, LinkedList<Diff> diffs).
         */
        public List<Patch> patch_make(string text1, string text2,
            List<Diff> diffs)
        {
            return patch_make(text1, diffs);
        }

        /**
         * Compute a list of patches to turn text1 into text2.
         * text2 is not provided, diffs are the delta between text1 and text2.
         * @param text1 Old text.
         * @param diffs Array of diff tuples for text1 to text2.
         * @return LinkedList of Patch objects.
         */
        public List<Patch> patch_make(string text1, List<Diff> diffs)
        {
            List<Patch> patches = new List<Patch>();
            if (diffs.Count == 0)
            {
                return patches;  // Get rid of the null case.
            }
            Patch patch = new Patch();
            int char_count1 = 0;  // Number of characters into the text1 string.
            int char_count2 = 0;  // Number of characters into the text2 string.
            // Start with text1 (prepatch_text) and apply the diffs until we arrive at
            // text2 (postpatch_text). We recreate the patches one by one to determine
            // context info.
            string prepatch_text = text1;
            string postpatch_text = text1;
            foreach (Diff aDiff in diffs)
            {
                if (patch.diffs.Count == 0 && aDiff.operation != Operation.EQUAL)
                {
                    // A new patch starts here.
                    patch.start1 = char_count1;
                    patch.start2 = char_count2;
                }

                switch (aDiff.operation)
                {
                    case Operation.INSERT:
                        patch.diffs.Add(aDiff);
                        patch.length2 += aDiff.text.Length;
                        postpatch_text = postpatch_text.Substring(0, char_count2)
                            + aDiff.text + postpatch_text.Substring(char_count2);
                        break;
                    case Operation.DELETE:
                        patch.length1 += aDiff.text.Length;
                        patch.diffs.Add(aDiff);
                        postpatch_text = postpatch_text.Substring(0, char_count2)
                            + postpatch_text.Substring(char_count2 + aDiff.text.Length);
                        break;
                    case Operation.EQUAL:
                        if (aDiff.text.Length <= 2 * Patch_Margin
                            && patch.diffs.Count() != 0 && aDiff != diffs.Last())
                        {
                            // Small equality inside a patch.
                            patch.diffs.Add(aDiff);
                            patch.length1 += aDiff.text.Length;
                            patch.length2 += aDiff.text.Length;
                        }

                        if (aDiff.text.Length >= 2 * Patch_Margin)
                        {
                            // Time for a new patch.
                            if (patch.diffs.Count != 0)
                            {
                                patch_addContext(patch, prepatch_text);
                                patches.Add(patch);
                                patch = new Patch();
                                // Unlike Unidiff, our patch lists have a rolling context.
                                // http://code.google.com/p/google-diff-match-patch/wiki/Unidiff
                                // Update prepatch text & pos to reflect the application of the
                                // just completed patch.
                                prepatch_text = postpatch_text;
                                char_count1 = char_count2;
                            }
                        }
                        break;
                }

                // Update the current character count.
                if (aDiff.operation != Operation.INSERT)
                {
                    char_count1 += aDiff.text.Length;
                }
                if (aDiff.operation != Operation.DELETE)
                {
                    char_count2 += aDiff.text.Length;
                }
            }
            // Pick up the leftover patch if not empty.
            if (patch.diffs.Count != 0)
            {
                patch_addContext(patch, prepatch_text);
                patches.Add(patch);
            }

            return patches;
        }

        /**
         * Given an array of patches, return another array that is identical.
         * @param patches Array of patch objects.
         * @return Array of patch objects.
         */
        public List<Patch> patch_deepCopy(List<Patch> patches)
        {
            List<Patch> patchesCopy = new List<Patch>();
            foreach (Patch aPatch in patches)
            {
                Patch patchCopy = new Patch();
                foreach (Diff aDiff in aPatch.diffs)
                {
                    Diff diffCopy = new Diff(aDiff.operation, aDiff.text);
                    patchCopy.diffs.Add(diffCopy);
                }
                patchCopy.start1 = aPatch.start1;
                patchCopy.start2 = aPatch.start2;
                patchCopy.length1 = aPatch.length1;
                patchCopy.length2 = aPatch.length2;
                patchesCopy.Add(patchCopy);
            }
            return patchesCopy;
        }

        /**
         * Merge a set of patches onto the text.  Return a patched text, as well
         * as an array of true/false values indicating which patches were applied.
         * @param patches Array of patch objects
         * @param text Old text.
         * @return Two element Object array, containing the new text and an array of
         *      bool values.
         */
        public Object[] patch_apply(List<Patch> patches, string text)
        {
            if (patches.Count == 0)
            {
                return new Object[] { text, new bool[0] };
            }

            // Deep copy the patches so that no changes are made to originals.
            patches = patch_deepCopy(patches);

            string nullPadding = this.patch_addPadding(patches);
            text = nullPadding + text + nullPadding;
            patch_splitMax(patches);

            int x = 0;
            // delta keeps track of the offset between the expected and actual location
            // of the previous patch.  If there are patches expected at positions 10 and
            // 20, but the first patch was found at 12, delta is 2 and the second patch
            // has an effective expected position of 22.
            int delta = 0;
            bool[] results = new bool[patches.Count];
            foreach (Patch aPatch in patches)
            {
                int expected_loc = aPatch.start2 + delta;
                string text1 = diff_text1(aPatch.diffs);
                int start_loc;
                int end_loc = -1;
                if (text1.Length > this.Match_MaxBits)
                {
                    // patch_splitMax will only provide an oversized pattern
                    // in the case of a monster delete.
                    start_loc = match_main(text,
                        text1.Substring(0, this.Match_MaxBits), expected_loc);
                    if (start_loc != -1)
                    {
                        end_loc = match_main(text,
                            text1.Substring(text1.Length - this.Match_MaxBits),
                            expected_loc + text1.Length - this.Match_MaxBits);
                        if (end_loc == -1 || start_loc >= end_loc)
                        {
                            // Can't find valid trailing context.  Drop this patch.
                            start_loc = -1;
                        }
                    }
                }
                else
                {
                    start_loc = this.match_main(text, text1, expected_loc);
                }
                if (start_loc == -1)
                {
                    // No match found.  :(
                    results[x] = false;
                    // Subtract the delta for this failed patch from subsequent patches.
                    delta -= aPatch.length2 - aPatch.length1;
                }
                else
                {
                    // Found a match.  :)
                    results[x] = true;
                    delta = start_loc - expected_loc;
                    string text2;
                    if (end_loc == -1)
                    {
                        text2 = text.JavaSubstring(start_loc,
                            Math.Min(start_loc + text1.Length, text.Length));
                    }
                    else
                    {
                        text2 = text.JavaSubstring(start_loc,
                            Math.Min(end_loc + this.Match_MaxBits, text.Length));
                    }
                    if (text1 == text2)
                    {
                        // Perfect match, just shove the Replacement text in.
                        text = text.Substring(0, start_loc) + diff_text2(aPatch.diffs)
                            + text.Substring(start_loc + text1.Length);
                    }
                    else
                    {
                        // Imperfect match.  Run a diff to get a framework of equivalent
                        // indices.
                        List<Diff> diffs = diff_main(text1, text2, false);
                        if (text1.Length > this.Match_MaxBits
                            && this.diff_levenshtein(diffs) / (float) text1.Length
                            > this.Patch_DeleteThreshold)
                        {
                            // The end points match, but the content is unacceptably bad.
                            results[x] = false;
                        }
                        else
                        {
                            diff_cleanupSemanticLossless(diffs);
                            int index1 = 0;
                            foreach (Diff aDiff in aPatch.diffs)
                            {
                                if (aDiff.operation != Operation.EQUAL)
                                {
                                    int index2 = diff_xIndex(diffs, index1);
                                    if (aDiff.operation == Operation.INSERT)
                                    {
                                        // Insertion
                                        text = text.Insert(start_loc + index2, aDiff.text);
                                    }
                                    else if (aDiff.operation == Operation.DELETE)
                                    {
                                        // Deletion
                                        text = text.Substring(0, start_loc + index2)
                                            + text.Substring(start_loc + diff_xIndex(diffs,
                                            index1 + aDiff.text.Length));
                                    }
                                }
                                if (aDiff.operation != Operation.DELETE)
                                {
                                    index1 += aDiff.text.Length;
                                }
                            }
                        }
                    }
                }
                x++;
            }
            // Strip the padding off.
            text = text.JavaSubstring(nullPadding.Length, text.Length
                - nullPadding.Length);
            return new Object[] { text, results };
        }

        /**
         * Add some padding on text start and end so that edges can match something.
         * Intended to be called only from within patch_apply.
         * @param patches Array of patch objects.
         * @return The padding string added to each side.
         */
        public string patch_addPadding(List<Patch> patches)
        {
            int paddingLength = this.Patch_Margin;
            string nullPadding = string.Empty;
            for (int x = 1; x <= paddingLength; x++)
            {
                nullPadding += (char)x;
            }

            // Bump all the patches forward.
            foreach (Patch aPatch in patches)
            {
                aPatch.start1 += paddingLength;
                aPatch.start2 += paddingLength;
            }

            // Add some padding on start of first diff.
            Patch patch = patches.First();
            List<Diff> diffs = patch.diffs;
            if (diffs.Count == 0 || diffs.First().operation != Operation.EQUAL)
            {
                // Add nullPadding equality.
                diffs.Insert(0, new Diff(Operation.EQUAL, nullPadding));
                patch.start1 -= paddingLength;  // Should be 0.
                patch.start2 -= paddingLength;  // Should be 0.
                patch.length1 += paddingLength;
                patch.length2 += paddingLength;
            }
            else if (paddingLength > diffs.First().text.Length)
            {
                // Grow first equality.
                Diff firstDiff = diffs.First();
                int extraLength = paddingLength - firstDiff.text.Length;
                firstDiff.text = nullPadding.Substring(firstDiff.text.Length)
                    + firstDiff.text;
                patch.start1 -= extraLength;
                patch.start2 -= extraLength;
                patch.length1 += extraLength;
                patch.length2 += extraLength;
            }

            // Add some padding on end of last diff.
            patch = patches.Last();
            diffs = patch.diffs;
            if (diffs.Count == 0 || diffs.Last().operation != Operation.EQUAL)
            {
                // Add nullPadding equality.
                diffs.Add(new Diff(Operation.EQUAL, nullPadding));
                patch.length1 += paddingLength;
                patch.length2 += paddingLength;
            }
            else if (paddingLength > diffs.Last().text.Length)
            {
                // Grow last equality.
                Diff lastDiff = diffs.Last();
                int extraLength = paddingLength - lastDiff.text.Length;
                lastDiff.text += nullPadding.Substring(0, extraLength);
                patch.length1 += extraLength;
                patch.length2 += extraLength;
            }

            return nullPadding;
        }

        /**
         * Look through the patches and break up any which are longer than the
         * maximum limit of the match algorithm.
         * @param patches LinkedList of Patch objects.
         */
        public void patch_splitMax(List<Patch> patches)
        {
            for (var x = 0; x < patches.Count; x++)
            {
                if (patches[x].length1 > this.Match_MaxBits)
                {
                    var bigpatch = patches[x];
                    // Remove the big old patch.
                    patches.Splice(x--, 1);
                    var patch_size = this.Match_MaxBits;
                    var start1 = bigpatch.start1;
                    var start2 = bigpatch.start2;
                    var precontext = string.Empty;
                    while (bigpatch.diffs.Count != 0)
                    {
                        // Create one of several smaller patches.
                        var patch = new Patch();
                        var empty = true;
                        patch.start1 = start1 - precontext.Length;
                        patch.start2 = start2 - precontext.Length;
                        if (precontext.Length != 0)
                        {
                            patch.length1 = patch.length2 = precontext.Length;
                            patch.diffs.Add(new Diff(Operation.EQUAL, precontext));
                        }
                        while (bigpatch.diffs.Count != 0 &&
                               patch.length1 < patch_size - this.Patch_Margin)
                        {
                            var diff_type = bigpatch.diffs[0].operation;
                            var diff_text = bigpatch.diffs[0].text;
                            if (diff_type == Operation.INSERT)
                            {
                                // Insertions are harmless.
                                patch.length2 += diff_text.Length;
                                start2 += diff_text.Length;
                                patch.diffs.Add(bigpatch.diffs.First());
                                bigpatch.diffs.RemoveAt(0);
                                empty = false;
                            }
                            else if (diff_type == Operation.DELETE && patch.diffs.Count == 1
                                && patch.diffs.First().operation == Operation.EQUAL
                                && diff_text.Length > 2 * patch_size)
                            {
                                // This is a large deletion.  Let it pass in one chunk.
                                patch.length1 += diff_text.Length;
                                start1 += diff_text.Length;
                                empty = false;
                                patch.diffs.Add(new Diff(diff_type, diff_text));
                                bigpatch.diffs.RemoveAt(0);
                            }
                            else
                            {
                                // Deletion or equality.  Only take as much as we can stomach.
                                diff_text = diff_text.Substring(0, Math.Min(diff_text.Length,
                                    patch_size - patch.length1 - Patch_Margin));
                                patch.length1 += diff_text.Length;
                                start1 += diff_text.Length;
                                if (diff_type == Operation.EQUAL)
                                {
                                    patch.length2 += diff_text.Length;
                                    start2 += diff_text.Length;
                                }
                                else
                                {
                                    empty = false;
                                }
                                patch.diffs.Add(new Diff(diff_type, diff_text));
                                if (diff_text == bigpatch.diffs[0].text)
                                {
                                    bigpatch.diffs.RemoveAt(0);
                                }
                                else
                                {
                                    bigpatch.diffs[0].text =
                                        bigpatch.diffs[0].text.Substring(diff_text.Length);
                                }
                            }
                        }
                        // Compute the head context for the next patch.
                        precontext = this.diff_text2(patch.diffs);
                        precontext =
                            precontext.Substring(Math.Max(0, precontext.Length - this.Patch_Margin));

                        string postcontext = null;
                        // Append the end context for this patch.
                        if (diff_text1(bigpatch.diffs).Length > Patch_Margin)
                        {
                            postcontext = diff_text1(bigpatch.diffs).Substring(0, Patch_Margin);
                        }
                        else
                        {
                            postcontext = diff_text1(bigpatch.diffs);
                        }

                        if (postcontext.Length != 0)
                        {
                            patch.length1 += postcontext.Length;
                            patch.length2 += postcontext.Length;
                            if (patch.diffs.Count != 0 &&
                                patch.diffs[patch.diffs.Count - 1].operation == Operation.EQUAL)
                            {
                                patch.diffs[patch.diffs.Count - 1].text += postcontext;
                            }
                            else
                            {
                                patch.diffs.Add(new Diff(Operation.EQUAL, postcontext));
                            }
                        }
                        if (!empty)
                        {
                            patches.Splice(++x, 0, patch);
                        }
                    }
                }
            }
        }

        /**
         * Take a list of patches and return a textual representation.
         * @param patches List of Patch objects.
         * @return Text representation of patches.
         */
        public string patch_toText(List<Patch> patches)
        {
            StringBuilder text = new StringBuilder();
            foreach (Patch aPatch in patches)
            {
                text.Append(aPatch);
            }
            return text.ToString();
        }

        /**
         * Parse a textual representation of patches and return a List of Patch
         * objects.
         * @param textline Text representation of patches.
         * @return List of Patch objects.
         * @throws ArgumentException If invalid input.
         */
        public List<Patch> patch_fromText(string textline)
        {
            List<Patch> patches = new List<Patch>();
            if (textline.Length == 0)
            {
                return patches;
            }
            List<string> textList = new List<string>(textline.Split(new string[] { "\n" }, StringSplitOptions.None));
            LinkedList<string> text = new LinkedList<string>(textList);
            Patch patch;
            Regex patchHeader
                = new Regex("^@@ -(\\d+),?(\\d*) \\+(\\d+),?(\\d*) @@$");
            Match m;
            char sign;
            string line;
            while (text.Count != 0)
            {
                m = patchHeader.Match(text.First());
                if (!m.Success)
                {
                    throw new ArgumentException(
                        "Invalid patch string: " + text.First());
                }
                patch = new Patch();
                patches.Add(patch);
                patch.start1 = Convert.ToInt32(m.Groups[1].Value);
                if (m.Groups[2].Length == 0)
                {
                    patch.start1--;
                    patch.length1 = 1;
                }
                else if (m.Groups[2].Value == "0")
                {
                    patch.length1 = 0;
                }
                else
                {
                    patch.start1--;
                    patch.length1 = Convert.ToInt32(m.Groups[2].Value);
                }

                patch.start2 = Convert.ToInt32(m.Groups[3].Value);
                if (m.Groups[4].Length == 0)
                {
                    patch.start2--;
                    patch.length2 = 1;
                }
                else if (m.Groups[4].Value == "0")
                {
                    patch.length2 = 0;
                }
                else
                {
                    patch.start2--;
                    patch.length2 = Convert.ToInt32(m.Groups[4].Value);
                }
                text.RemoveFirst();

                while (text.Count != 0)
                {
                    try
                    {
                        sign = text.First()[0];
                    }
                    catch (IndexOutOfRangeException)
                    {
                        // Blank line?  Whatever.
                        text.RemoveFirst();
                        continue;
                    }
                    line = text.First().Substring(1);
                    line = line.Replace("+", "%2b");
                    line = HttpUtility.UrlDecode(line, new UTF8Encoding(false, true));
                    if (sign == '-')
                    {
                        // Deletion.
                        patch.diffs.Add(new Diff(Operation.DELETE, line));
                    }
                    else if (sign == '+')
                    {
                        // Insertion.
                        patch.diffs.Add(new Diff(Operation.INSERT, line));
                    }
                    else if (sign == ' ')
                    {
                        // Minor equality.
                        patch.diffs.Add(new Diff(Operation.EQUAL, line));
                    }
                    else if (sign == '@')
                    {
                        // Start of next patch.
                        break;
                    }
                    else
                    {
                        // WTF?
                        throw new ArgumentException(
                            "Invalid patch mode '" + sign + "' in: " + line);
                    }
                    text.RemoveFirst();
                }
            }
            return patches;
        }

        /**
         * Unescape selected chars for compatability with JavaScript's encodeURI.
         * In speed critical applications this could be dropped since the
         * receiving application will certainly decode these fine.
         * Note that this function is case-sensitive.  Thus "%3F" would not be
         * unescaped.  But this is ok because it is only called with the output of
         * HttpUtility.UrlEncode which returns lowercase hex.
         *
         * Example: "%3f" -> "?", "%24" -> "$", etc.
         *
         * @param str The string to escape.
         * @return The escaped string.
         */
        public static string unescapeForEncodeUriCompatability(string str)
        {
            return str.Replace("%21", "!").Replace("%7e", "~")
                .Replace("%27", "'").Replace("%28", "(").Replace("%29", ")")
                .Replace("%3b", ";").Replace("%2f", "/").Replace("%3f", "?")
                .Replace("%3a", ":").Replace("%40", "@").Replace("%26", "&")
                .Replace("%3d", "=").Replace("%2b", "+").Replace("%24", "$")
                .Replace("%2c", ",").Replace("%23", "#");
        }
    }
}
