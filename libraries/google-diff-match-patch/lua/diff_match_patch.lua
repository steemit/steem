--[[
* Diff Match and Patch
*
* Copyright 2006 Google Inc.
* http://code.google.com/p/google-diff-match-patch/
*
* Based on the JavaScript implementation by Neil Fraser
* Ported to Lua by Duncan Cross
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
--]]

require 'bit'   -- <http://bitop.luajit.org/>

local band, bor, lshift
    = bit.band, bit.bor, bit.lshift
local type, setmetatable, ipairs, select
    = type, setmetatable, ipairs, select
local unpack, tonumber, error
    = unpack, tonumber, error
local strsub, strbyte, strchar, gmatch, gsub
    = string.sub, string.byte, string.char, string.gmatch, string.gsub
local strmatch, strfind, strformat
    = string.match, string.find, string.format
local tinsert, tremove, tconcat
    = table.insert, table.remove, table.concat
local max, min, floor, ceil, abs
    = math.max, math.min, math.floor, math.ceil, math.abs
local clock = os.clock
local print = print

module 'diff_match_patch'


-- Utility functions

local percentEncode_pattern = '[^A-Za-z0-9%-=;\',./~!@#$%&*%(%)_%+ %?]'
local function percentEncode_replace(v)
  return strformat('%%%02X', strbyte(v))
end

local function tsplice(t, idx, deletions, ...)
  local insertions = select('#', ...)
  for i = 1, deletions do
    tremove(t, idx)
  end
  for i = insertions, 1, -1 do
    -- do not remove parentheses around select
    tinsert(t, idx, (select(i, ...)))
  end
end

local function tsub(t, i, j)
  if (i < 0) then
    local sz = #t
    i = sz + 1 + i
    if j and (j < 0) then
      j = sz + 1 + j
    end
  elseif j and (j < 0) then
    j = #t + 1 + j
  end
  return {unpack(t, i, j)}
end

local function strelement(str, i)
  return strsub(str, i, i)
end

local function telement(t, i)
  if (i < 0) then
    i = #t + 1 + i
  end
  return t[i]
end

local function tprepend(t, v)
  tinsert(t, 1, v)
  return t
end

local function tappend(t, v)
  t[#t + 1] = v
  return t
end

local function strappend(str, v)
  return str .. v
end

local function strprepend(str, v)
  return v .. str
end

local function indexOf(a, b, start)
  if (#b == 0) then
    return nil
  end
  return strfind(a, b, start, true)
end

local function getMaxBits()
  local maxbits = 0
  local oldi, newi = 1, 2
  while (oldi ~= newi) do
    maxbits = maxbits + 1
    oldi = newi
    newi = lshift(newi, 1)
  end
  return maxbits
end

local htmlEncode_pattern = '[&<>\n]'
local htmlEncode_replace = {
  ['&'] = '&amp;', ['<'] = '&lt;', ['>'] = '&gt;', ['\n'] = '&para;<BR>'
}

-- Public API Functions
-- (Exported at the end of the script)

local diff_main,
      diff_cleanupSemantic,
      diff_cleanupEfficiency,
      diff_levenshtein,
      diff_prettyHtml

local match_main

local patch_make,
      patch_toText,
      patch_fromText,
      patch_apply

--[[
* The data structure representing a diff is an array of tuples:
* {{DIFF_DELETE, 'Hello'}, {DIFF_INSERT, 'Goodbye'}, {DIFF_EQUAL, ' world.'}}
* which means: delete 'Hello', add 'Goodbye' and keep ' world.'
--]]
local DIFF_DELETE = -1
local DIFF_INSERT = 1
local DIFF_EQUAL = 0

-- Number of seconds to map a diff before giving up (0 for infinity).
local Diff_Timeout = 1.0
-- Cost of an empty edit operation in terms of edit characters.
local Diff_EditCost = 4
-- The size beyond which the double-ended diff activates.
-- Double-ending is twice as fast, but less accurate.
local Diff_DualThreshold = 32
-- At what point is no match declared (0.0 = perfection, 1.0 = very loose).
local Match_Threshold = 0.5
-- How far to search for a match (0 = exact location, 1000+ = broad match).
-- A match this many characters away from the expected location will add
-- 1.0 to the score (0.0 is a perfect match).
local Match_Distance = 1000
-- When deleting a large block of text (over ~64 characters), how close does
-- the contents have to match the expected contents. (0.0 = perfection,
-- 1.0 = very loose).  Note that Match_Threshold controls how closely the
-- end points of a delete need to match.
local Patch_DeleteThreshold = 0.5
-- Chunk size for context length.
local Patch_Margin = 4
-- How many bits in a number?
local Match_MaxBits = getMaxBits()

function settings(new)
  if new then
    Diff_Timeout = new.Diff_Timeout or Diff_Timeout
    Diff_EditCost = new.Diff_EditCost or Diff_EditCost
    Diff_DualThreshold = new.Diff_DualThreshold or Diff_DualThreshold
    Match_Threshold = new.Match_Threshold or Match_Threshold
    Match_Distance = new.Match_Distance or Match_Distance
    Patch_DeleteThreshold = new.Patch_DeleteThreshold or Patch_DeleteThreshold
    Patch_Margin = new.Patch_Margin or Patch_Margin
    Match_MaxBits = new.Match_MaxBits or Match_MaxBits
  else
    return {
      Diff_Timeout = Diff_Timeout;
      Diff_EditCost = Diff_EditCost;
      Diff_DualThreshold = Diff_DualThreshold;
      Match_Threshold = Match_Threshold;
      Match_Distance = Match_Distance;
      Patch_DeleteThreshold = Patch_DeleteThreshold;
      Patch_Margin = Patch_Margin;
      Match_MaxBits = Match_MaxBits;
    }
  end
end

-- ---------------------------------------------------------------------------
--  DIFF API
-- ---------------------------------------------------------------------------

-- The private diff functions
local _diff_compute,
      _diff_map,
      _diff_path1,
      _diff_path2,
      _diff_halfMatchI,
      _diff_halfMatch,
      _diff_cleanupSemanticScore,
      _diff_cleanupSemanticLossless,
      _diff_cleanupMerge,
      _diff_commonPrefix,
      _diff_commonSuffix,
      _diff_xIndex,
      _diff_text1,
      _diff_text2,
      _diff_toDelta,
      _diff_fromDelta,
      _diff_toLines,
      _diff_fromLines

--[[
* Find the differences between two texts.  Simplifies the problem by stripping
* any common prefix or suffix off the texts before diffing.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @param {boolean} opt_checklines Optional speedup flag.  If present and false,
*    then don't run a line-level diff first to identify the changed areas.
*    Defaults to true, which does a faster, slightly less optimal diff
* @return {Array.<Array.<number|string>>} Array of diff tuples.
--]]
function diff_main(text1, text2, opt_checklines)
  -- Check for equality (speedup)
  if (text1 == text2) then
    return {{DIFF_EQUAL, text1}}
  end

  if (opt_checklines == nil) then
    opt_checklines = true
  end
  local checklines = opt_checklines

  -- Trim off common prefix (speedup)
  local commonlength = _diff_commonPrefix(text1, text2)
  local commonprefix
  if (commonlength > 0) then
    commonprefix = strsub(text1, 1, commonlength)
    text1 = strsub(text1, commonlength + 1)
    text2 = strsub(text2, commonlength + 1)
  end

  -- Trim off common suffix (speedup)
  commonlength = _diff_commonSuffix(text1, text2)
  local commonsuffix
  if (commonlength > 0) then
    commonsuffix = strsub(text1, -commonlength)
    text1 = strsub(text1, 1, -commonlength - 1)
    text2 = strsub(text2, 1, -commonlength - 1)
  end

  -- Compute the diff on the middle block
  local diffs = _diff_compute(text1, text2, checklines)

  -- Restore the prefix and suffix
  if commonprefix then
    tinsert(diffs, 1, {DIFF_EQUAL, commonprefix})
  end
  if commonsuffix then
    diffs[#diffs + 1] = {DIFF_EQUAL, commonsuffix}
  end

  _diff_cleanupMerge(diffs)
  return diffs
end

--[[
* Reduce the number of edits by eliminating semantically trivial equalities.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function diff_cleanupSemantic(diffs)
  local changes = false
  local equalities = {}  -- Stack of indices where equalities are found.
  local equalitiesLength = 0  -- Keeping our own length var is faster.
  local lastequality = nil  -- Always equal to equalities[equalitiesLength][2]
  local pointer = 1  -- Index of current position.
  -- Number of characters that changed prior to the equality.
  local length_changes1 = 0
  -- Number of characters that changed after the equality.
  local length_changes2 = 0

  while diffs[pointer] do
    if (diffs[pointer][1] == DIFF_EQUAL) then  -- equality found
      equalitiesLength = equalitiesLength + 1
      equalities[equalitiesLength] = pointer
      length_changes1 = length_changes2
      length_changes2 = 0
      lastequality = diffs[pointer][2]
    else  -- an insertion or deletion
      length_changes2 = length_changes2 + #(diffs[pointer][2])
      if lastequality and (#lastequality > 0)
         and (#lastequality <= length_changes1)
         and (#lastequality <= length_changes2) then
        -- Duplicate record
        tinsert(diffs, equalities[equalitiesLength],
         {DIFF_DELETE, lastequality})
        -- Change second copy to insert.
        diffs[equalities[equalitiesLength] + 1][1] = DIFF_INSERT
        -- Throw away the equality we just deleted.
        equalitiesLength = equalitiesLength - 1
        -- Throw away the previous equality (it needs to be reevaluated).
        equalitiesLength = equalitiesLength - 1
        pointer = (equalitiesLength > 0) and equalities[equalitiesLength] or 0
        length_changes1, length_changes2 = 0, 0  -- Reset the counters.
        lastequality = nil
        changes = true
      end
    end
    pointer = pointer + 1
  end
  if changes then
    _diff_cleanupMerge(diffs)
  end
  _diff_cleanupSemanticLossless(diffs)
end

--[[
* Reduce the number of edits by eliminating operationally trivial equalities.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function diff_cleanupEfficiency(diffs)
  local changes = false
  -- Stack of indices where equalities are found.
  local equalities = {}
  -- Keeping our own length var is faster.
  local equalitiesLength = 0
  -- Always equal to diffs[equalities[equalitiesLength]][2]
  local lastequality = ''
  -- Index of current position.
  local pointer = 1

  -- The following four are really booleans but are stored as numbers because
  -- they are used at one point like this:
  --
  -- (pre_ins + pre_del + post_ins + post_del) == 3
  --
  -- ...i.e. checking that 3 of them are true and 1 of them is false.

  -- Is there an insertion operation before the last equality.
  local pre_ins = 0
  -- Is there a deletion operation before the last equality.
  local pre_del = 0
  -- Is there an insertion operation after the last equality.
  local post_ins = 0
  -- Is there a deletion operation after the last equality.
  local post_del = 0

  while diffs[pointer] do
    if (diffs[pointer][1] == DIFF_EQUAL) then   -- equality found
      local diffText = diffs[pointer][2]
      if (#diffText < Diff_EditCost) and (post_ins == 1 or post_del == 1) then
        -- Candidate found.
        equalitiesLength = equalitiesLength + 1
        equalities[equalitiesLength] = pointer
        pre_ins, pre_del = post_ins, post_del
        lastequality = diffText
      else
        -- Not a candidate, and can never become one.
        equalitiesLength = 0
        lastequality = ''
      end
      post_ins, post_del = 0, 0
    else  -- an insertion or deletion
      if (diffs[pointer][1] == DIFF_DELETE) then
        post_del = 1
      else
        post_ins = 1
      end
      --[[
      * Five types to be split:
      * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
      * <ins>A</ins>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<ins>C</ins>
      * <ins>A</del>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<del>C</del>
      --]]
      if (#lastequality > 0) and (
          (pre_ins+pre_del+post_ins+post_del == 4)
          or
          (
            (#lastequality < Diff_EditCost / 2)
            and
            (pre_ins+pre_del+post_ins+post_del == 3)
          )) then
        -- Duplicate record
        tinsert(diffs, equalities[equalitiesLength],
            {DIFF_DELETE, lastequality})
        -- Change second copy to insert.
        diffs[equalities[equalitiesLength] + 1][1] = DIFF_INSERT
        -- Throw away the equality we just deleted;
        equalitiesLength = equalitiesLength - 1
        lastequality = ''
        if (pre_ins == 1) and (pre_del == 1) then
          -- No changes made which could affect previous entry, keep going.
          post_ins, post_del = 1, 1
          equalitiesLength = 0
        else
          -- Throw away the previous equality;
          equalitiesLength = equalitiesLength - 1
          pointer = (equalitiesLength > 0) and equalities[equalitiesLength] or 0
          post_ins, post_del = 0, 0
        end
        changes = true
      end
    end
    pointer = pointer + 1
  end

  if changes then
    _diff_cleanupMerge(diffs)
  end
end

--[[
* Compute the Levenshtein distance; the number of inserted, deleted or
* substituted characters.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {number} Number of changes.
--]]
function diff_levenshtein(diffs)
  local levenshtein = 0
  local insertions, deletions = 0, 0
  for x, diff in ipairs(diffs) do
    local op, data = diff[1], diff[2]
    if (op == DIFF_INSERT) then
      insertions = insertions + #data
    elseif (op == DIFF_DELETE) then
      deletions = deletions + #data
    else--if (op == DIFF_EQUAL) then
      -- A deletion and an insertion is one substitution.
      levenshtein = levenshtein + max(insertions, deletions)
      insertions = 0
      deletions = 0
    end
  end
  levenshtein = levenshtein + max(insertions, deletions)
  return levenshtein
end

--[[
* Convert a diff array into a pretty HTML report.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} HTML representation.
--]]
function diff_prettyHtml(diffs)
  local html = {}
  local i = 0
  for x, diff in ipairs(diffs) do
    local op = diff[1]   -- Operation (insert, delete, equal)
    local data = diff[2]  -- Text of change.
    local text = gsub(data, htmlEncode_pattern, htmlEncode_replace)
    if (op == DIFF_INSERT) then
      html[x] = '<INS STYLE="background:#E6FFE6;" TITLE="i=' .. i .. '">'
          .. text .. '</INS>'
    elseif (op == DIFF_DELETE) then
      html[x] = '<DEL STYLE="background:#FFE6E6;" TITLE="i=' .. i .. '">'
          .. text .. '</DEL>'
    else--if (op == DIFF_EQUAL) then
      html[x] = '<SPAN TITLE="i=' .. i .. '">' .. text .. '</SPAN>'
    end
    if (op ~= DIFF_DELETE) then
      i = i + #data
    end
  end
  return tconcat(html)
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE DIFF FUNCTIONS
-- ---------------------------------------------------------------------------

--[[
* Find the differences between two texts.  Assumes that the texts do not
* have any common prefix or suffix.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @param {boolean} checklines Speedup flag.  If false, then don't run a
*    line-level diff first to identify the changed areas.
*    If true, then run a faster, slightly less optimal diff
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @private
--]]
function _diff_compute(text1, text2, checklines)
  if (#text1 == 0) then
    -- Just add some text (speedup)
    return {{DIFF_INSERT, text2}}
  end

  if (#text2 == 0) then
    -- Just delete some text (speedup)
    return {{DIFF_DELETE, text1}}
  end

  local diffs

  local longtext = (#text1 > #text2) and text1 or text2
  local shorttext = (#text1 > #text2) and text2 or text1
  local i = indexOf(longtext, shorttext)

  if (i ~= nil) then
    -- Shorter text is inside the longer text (speedup)
    diffs = {
      {DIFF_INSERT, strsub(longtext, 1, i - 1)},
      {DIFF_EQUAL, shorttext},
      {DIFF_INSERT, strsub(longtext, i + #shorttext)}
    }
    -- Swap insertions for deletions if diff is reversed.
    if (#text1 > #text2) then
      diffs[1][1], diffs[3][1] = DIFF_DELETE, DIFF_DELETE
    end
    return diffs
  end
  longtext, shorttext = nil, nil  -- Garbage collect.

  -- Check to see if the problem can be split in two.
  do
    local
     text1_a, text1_b,
     text2_a, text2_b,
     mid_common        = _diff_halfMatch(text1, text2)

    if text1_a then
      -- A half-match was found, sort out the return data.
      -- Send both pairs off for separate processing.
      local diffs_a = diff_main(text1_a, text2_a, checklines)
      local diffs_b = diff_main(text1_b, text2_b, checklines)
      -- Merge the results.
      local diffs_a_len = #diffs_a
      diffs = diffs_a
      diffs[diffs_a_len + 1] = {DIFF_EQUAL, mid_common}
      for i, b_diff in ipairs(diffs_b) do
        diffs[diffs_a_len + 1 + i] = b_diff
      end
      return diffs
    end
  end
  -- Perform a real diff.
  if checklines and (#text1 < 100 or #text2 < 100) then
    -- Too trivial for the overhead.
    checklines = false
  end
  local linearray
  if checklines then
    -- Scan the text on a line-by-line basis first.
    text1, text2, linearray = _diff_toLines(text1, text2)
  end
  diffs = _diff_map(text1, text2)
  if (diffs == nil) then
    -- No acceptable result.
    diffs = {{DIFF_DELETE, text1}, {DIFF_INSERT, text2}}
  end
  if checklines then
    -- Convert the diff back to original text.
    _diff_fromLines(diffs, linearray)
    -- Eliminate freak matches (e.g. blank lines)
    diff_cleanupSemantic(diffs)
    -- Rediff any replacement blocks, this time character-by-character.
    -- Add a dummy entry at the end.
    diffs[#diffs + 1] = {DIFF_EQUAL, ''}
    local pointer = 1
    local count_delete = 0
    local count_insert = 0
    local text_delete = ''
    local text_insert = ''
    while (pointer <= #diffs) do
      local diff_type = diffs[pointer][1]
      if (diff_type == DIFF_INSERT) then
        count_insert = count_insert + 1
        text_insert = text_insert .. diffs[pointer][2]
      elseif (diff_type == DIFF_DELETE) then
        count_delete = count_delete + 1
        text_delete = text_delete .. diffs[pointer][2]
      else--if (diff_type == DIFF_EQUAL) then
        -- Upon reaching an equality, check for prior redundancies.
        if (count_delete >= 1) and (count_insert >= 1) then
          -- Delete the offending records and add the merged ones.
          local a = diff_main(text_delete, text_insert, false)
          pointer = pointer - count_delete - count_insert
          for i = 1, count_delete + count_insert do
            tremove(diffs, pointer)
          end
          for j = #a, 1, -1 do
            tinsert(diffs, pointer, a[j])
          end
          pointer = pointer + #a
        end
        count_insert = 0
        count_delete = 0
        text_delete = ''
        text_insert = ''
      end
      pointer = pointer + 1
    end
    diffs[#diffs] = nil  -- Remove the dummy entry at the end.
  end
  return diffs
end

--[[
* Split two texts into an array of strings.  Reduce the texts to a string of
* hashes where each Unicode character represents one line.
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {Array.<string|Array.<string>>} Three element Array, containing the
*     encoded text1, the encoded text2 and the array of unique strings.  The
*     zeroth element of the array of unique strings is intentionally blank.
* @private
--]]
function _diff_toLines(text1, text2)
  local lineArray = {}  -- e.g. lineArray[4] == 'Hello\n'
  local lineHash = {}   -- e.g. lineHash['Hello\n'] == 4

  --[[
  * Split a text into an array of strings.  Reduce the texts to a string of
  * hashes where each Unicode character represents one line.
  * Modifies linearray and linehash through being a closure.
  * @param {string} text String to encode.
  * @return {string} Encoded string.
  * @private
  --]]
  local _diff_toLinesMunge = function(text)
    local lines = {}
    -- Walk the text, pulling out a substring for each line.
    -- text.split('\n') would would temporarily double our memory footprint.
    -- Modifying text would create many large strings to garbage collect.
    local lineStart = 1
    local lineEnd = 0
    -- Keeping our own length variable is faster than looking it up.
    local lineArrayLength = #lineArray
    while (lineEnd < #text) do
      lineEnd = indexOf(text, '\n', lineStart) or #text
      local line = strsub(text, lineStart, lineEnd)
      lineStart = lineEnd + 1

      local hash = lineHash[line]
      if hash then
        lines[#lines + 1] = hash
      else
        lineArrayLength = lineArrayLength + 1
        lines[#lines + 1] = lineArrayLength
        lineHash[line] = lineArrayLength
        lineArray[lineArrayLength] = line
      end
    end
    return lines
  end

  local lines1 = _diff_toLinesMunge(text1)
  local lines2 = _diff_toLinesMunge(text2)
  return lines1, lines2, lineArray
end

--[[
* Rehydrate the text in a diff from a string of line hashes to real lines of
* text.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @param {Array.<string>} lineArray Array of unique strings.
* @private
--]]
function _diff_fromLines(diffs, lineArray)
  for x, diff in ipairs(diffs) do
    local lines = diff[2]
    local text = {}
    for idx, line in ipairs(lines) do
      text[idx] = lineArray[line]
    end
    diff[2] = tconcat(text)
  end
end

--[[
* Explore the intersection points between the two texts.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @return {Array.<Array.<number|string>>?} Array of diff tuples or nil if no
*    diff available.
* @private
--]]
function _diff_map(text1, text2)
  -- Don't run for too long.
  local s_end = clock() + Diff_Timeout
  -- Cache the text lengths to prevent multiple calls.
  local text1_length = #text1
  local text2_length = #text2
  local _sub, _element
  -- LUANOTE: Due to the lack of Unicode in Lua, tables are used
  -- for the linemode speedup.
  if type(text1) == 'table' then
    _sub, _element = tsub, telement
  else
    _sub, _element = strsub, strelement
  end
  local max_d = text1_length + text2_length - 1
  local doubleEnd = (Diff_DualThreshold * 2 < max_d)
  local v_map1 = {}
  local v_map2 = {}
  local v1 = {}
  local v2 = {}
  v1[1] = 1
  v2[1] = 1
  local x, y
  local footstep  -- Used to track overlapping paths.
  local footsteps = {}
  local done = false
  -- If the total number of characters is odd, then
  -- the front path will collide with the reverse path.
  local front = (((text1_length + text2_length) % 2) == 1)
  for d = 0, max_d - 1 do
    -- Bail out if timeout reached.
    if (Diff_Timeout > 0) and (clock() > s_end) then
      return nil
    end

    -- Walk the front path one step.
    v_map1[d + 1] = {}
    for k = -d, d, 2 do
      if (k == -d) or ((k ~= d) and (v1[k - 1] < v1[k + 1])) then
        x = v1[k + 1]
      else
        x = v1[k - 1] + 1
      end
      y = x - k
      if doubleEnd then
        footstep = x .. ',' .. y
        if front and footsteps[footstep] then
          done = true
        end
        if not front then
          footsteps[footstep] = d
        end
      end

      while (not done) and (x <= text1_length) and (y <= text2_length)
          and (_element(text1, x) == _element(text2, y)) do
        x = x + 1
        y = y + 1
        if doubleEnd then
          footstep = x .. ',' .. y
          if front and footsteps[footstep] then
            done = true
          end
          if not front then
            footsteps[footstep] = d
          end
        end
      end

      v1[k] = x
      v_map1[d + 1][x .. ',' .. y] = true
      if (x == text1_length + 1) and (y == text2_length + 1) then
        -- Reached the end in single-path mode.
        return _diff_path1(v_map1, text1, text2)
      elseif done then
        -- Front path ran over reverse path.
        for i = footsteps[footstep] + 2, #v_map2 do
          v_map2[i] = nil
        end
        local a = _diff_path1(v_map1,
            _sub(text1, 1, x - 1),
            _sub(text2, 1, y - 1)
        )
        local a_len = #a
        local a2 = _diff_path2(v_map2,
            _sub(text1, x),
            _sub(text2, y)
        )
        for i, v in ipairs(a2) do
          a[a_len + i] = v
        end
        return a
      end
    end

    if doubleEnd then
      -- Walk the reverse path one step.
      v_map2[d + 1] = {}
      for k = -d, d, 2 do
        if (k == -d) or ((k ~= d) and (v2[k - 1] < v2[k + 1])) then
          x = v2[k + 1]
        else
          x = v2[k - 1] + 1
        end
        y = x - k
        footstep = (text1_length + 2 - x) .. ',' .. (text2_length + 2 - y)
        if (not front) and footsteps[footstep] then
          done = true
        end
        if front then
          footsteps[footstep] = d
        end

        while (not done) and (x <= text1_length) and (y <= text2_length)
           and (_element(text1, -x) == _element(text2, -y)) do
          x, y = x + 1, y + 1
          footstep = (text1_length + 2 - x) .. ',' .. (text2_length + 2 - y)
          if (not front) and footsteps[footstep] then
            done = true
          end
          if front then
            footsteps[footstep] = d
          end
        end

        v2[k] = x
        v_map2[d + 1][x .. ',' .. y] = true
        if done then
          -- Reverse path ran over front path.
          for i = footsteps[footstep] + 2, #v_map1 do
            v_map1[i] = nil
          end
          local a = _diff_path1(v_map1, _sub(text1, 1, -x), _sub(text2, 1, -y))
          -- LUANOTE: #text1-x+2 instead of -x+1 because if x is -1,
          -- you get the whole string instead of the empty string.
          local appending = _diff_path2(v_map2,
              _sub(text1, #text1 - x + 2), _sub(text2, #text2 - y + 2))
          local a_len = #a
          for i, v in ipairs(appending) do
            a[a_len + i] = v
          end
          return a
        end
      end
    end
  end
  -- Number of diffs equals number of characters, no commonality at all.
  return nil
end


--[[
* Work from the middle back to the start to determine the path.
* @param {Array.<Object>} v_map Array of paths.
* @param {string} text1 Old string fragment to be diffed.
* @param {string} text2 New string fragment to be diffed.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @private
--]]
function _diff_path1(v_map, text1, text2)
  local path = {}
  local _prepend, _element, _sub
  -- LUANOTE: Due to the lack of Unicode in Lua, tables are used
  -- for the linemode speedup.
  if type(text1) == 'table' then
    _prepend, _element, _sub = tprepend, telement, tsub
  else
    _prepend, _element, _sub = strprepend, strelement, strsub
  end
  local x = #text1 + 1
  local y = #text2 + 1
  --[[ @type {number?} ]]
  local last_op = nil
  for d = #v_map - 1, 1, -1 do
    while true do
      if v_map[d][(x - 1) .. ',' .. y] then
        x = x - 1
        if (last_op == DIFF_DELETE) then
          path[1][2] = _prepend(path[1][2], _element(text1, x))
        else
          tinsert(path, 1, {DIFF_DELETE, _sub(text1, x, x)})
        end
        last_op = DIFF_DELETE
        break
      elseif v_map[d][x .. ',' .. (y - 1)] then
        y = y - 1
        if (last_op == DIFF_INSERT) then
          path[1][2] = _prepend(path[1][2], _element(text2, y))
        else
          tinsert(path, 1, {DIFF_INSERT, _sub(text2, y, y)})
        end
        last_op = DIFF_INSERT
        break
      else
        x = x - 1
        y = y - 1
        if (_element(text1, x) ~= _element(text2, y)) then
           error('No diagonal.  Can\'t happen. (_diff_path1)')
        end
        if (last_op == DIFF_EQUAL) then
          path[1][2] = _prepend(path[1][2], _element(text1, x))
        else
          tinsert(path, 1, {DIFF_EQUAL, _sub(text1, x, x)})
        end
        last_op = DIFF_EQUAL
      end
    end
  end
  return path
end


--[[
* Work from the middle back to the end to determine the path.
* @param {Array.<Object>} v_map Array of paths.
* @param {string} text1 Old string fragment to be diffed.
* @param {string} text2 New string fragment to be diffed.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @private
--]]
function _diff_path2(v_map, text1, text2)
  local path = {}
  local pathLength = 0
  local _append, _element, _sub
  -- LUANOTE: Due to the lack of Unicode in Lua, tables are used
  -- for the linemode speedup.
  if type(text1) == 'table' then
    _append, _element, _sub = tappend, telement, tsub
  else
    _append, _element, _sub = strappend, strelement, strsub
  end
  local x = #text1 + 1
  local y = #text2 + 1
  --[[ @type {number?} ]]
  local last_op = nil
  for d = #v_map - 1, 1, -1 do
    while true do
      if v_map[d][(x - 1) .. ',' .. y] then
        x = x - 1
        if (last_op == DIFF_DELETE) then
          path[pathLength][2]
              = _append(path[pathLength][2], _element(text1, -x))
        else
          pathLength = pathLength + 1
          path[pathLength] = {DIFF_DELETE, _sub(text1, -x, -x)}
        end
        last_op = DIFF_DELETE
        break
      elseif v_map[d][x .. ',' .. (y - 1)] then
        y = y - 1
        if (last_op == DIFF_INSERT) then
          path[pathLength][2]
              = _append(path[pathLength][2], _element(text2, -y))
        else
          pathLength = pathLength + 1
          path[pathLength] = {DIFF_INSERT, _sub(text2, -y, -y)}
        end
        last_op = DIFF_INSERT
        break
      else
        x = x - 1
        y = y - 1
        if (strsub(text1, -x, -x) ~= strsub(text2, -y, -y)) then
          error('No diagonal.  Can\'t happen. (_diff_path2)')
        end
        if (last_op == DIFF_EQUAL) then
          path[pathLength][2]
              = _append(path[pathLength][2], _element(text1, -x))
        else
          pathLength = pathLength + 1
          path[pathLength] = {DIFF_EQUAL, _sub(text1, -x, -x)}
        end
        last_op = DIFF_EQUAL
      end
    end
  end
  return path
end

--[[
* Determine the common prefix of two strings
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {number} The number of characters common to the start of each
*    string.
--]]
function _diff_commonPrefix(text1, text2)
  -- Quick check for common null cases.
  if (#text1 == 0) or (#text2 == 0) or (strbyte(text1, 1) ~= strbyte(text2, 1))
      then
    return 0
  end
  -- Binary search.
  -- Performance analysis: http://neil.fraser.name/news/2007/10/09/
  local pointermin = 1
  local pointermax = min(#text1, #text2)
  local pointermid = pointermax
  local pointerstart = 1
  while (pointermin < pointermid) do
    if (strsub(text1, pointerstart, pointermid)
        == strsub(text2, pointerstart, pointermid)) then
      pointermin = pointermid
      pointerstart = pointermin
    else
      pointermax = pointermid
    end
    pointermid = floor(pointermin + (pointermax - pointermin) / 2)
  end
  return pointermid
end


--[[
* Determine the common suffix of two strings
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {number} The number of characters common to the end of each string.
--]]
function _diff_commonSuffix(text1, text2)
  -- Quick check for common null cases.
  if (#text1 == 0) or (#text2 == 0)
      or (strbyte(text1, -1) ~= strbyte(text2, -1)) then
    return 0
  end
  -- Binary search.
  -- Performance analysis: http://neil.fraser.name/news/2007/10/09/
  local pointermin = 1
  local pointermax = min(#text1, #text2)
  local pointermid = pointermax
  local pointerend = 1
  while (pointermin < pointermid) do
    if (strsub(text1, -pointermid, -pointerend)
        == strsub(text2, -pointermid, -pointerend)) then
      pointermin = pointermid
      pointerend = pointermin
    else
      pointermax = pointermid
    end
    pointermid = floor(pointermin + (pointermax - pointermin) / 2)
  end
  return pointermid
end

--[[
* Does a substring of shorttext exist within longtext such that the substring
* is at least half the length of longtext?
* Closure, but does not reference any external variables.
* @param {string} longtext Longer string.
* @param {string} shorttext Shorter string.
* @param {number} i Start index of quarter length substring within longtext
* @return {Array.<string>?} Five element Array, containing the prefix of
*    longtext, the suffix of longtext, the prefix of shorttext, the suffix
*    of shorttext and the common middle.  Or nil if there was no match.
* @private
--]]
function _diff_halfMatchI(longtext, shorttext, i)
  -- Start with a 1/4 length substring at position i as a seed.
  local seed = strsub(longtext, i, i + floor(#longtext / 4))
  local j = 0  -- LUANOTE: do not change to 1, was originally -1
  local best_common = ''
  local best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b
  while true do
    j = indexOf(shorttext, seed, j + 1)
    if (j == nil) then
      break
    end
    local prefixLength = _diff_commonPrefix(strsub(longtext, i),
        strsub(shorttext, j))
    local suffixLength = _diff_commonSuffix(strsub(longtext, 1, i - 1),
        strsub(shorttext, 1, j - 1))
    if (#best_common < suffixLength+prefixLength) then
      best_common = strsub(shorttext, j - suffixLength, j - 1)
          .. strsub(shorttext, j, j + prefixLength - 1)
      best_longtext_a = strsub(longtext, 1, i - suffixLength - 1)
      best_longtext_b = strsub(longtext, i + prefixLength)
      best_shorttext_a = strsub(shorttext, 1, j - suffixLength - 1)
      best_shorttext_b = strsub(shorttext, j + prefixLength)
    end
  end
  if (#best_common >= #longtext / 2) then
    return {best_longtext_a, best_longtext_b,
        best_shorttext_a, best_shorttext_b, best_common}
  else
    return nil
  end
end

--[[
* Do the two texts share a substring which is at least half the length of the
* longer text?
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {Array.<string>?} Five element Array, containing the prefix of
*    text1, the suffix of text1, the prefix of text2, the suffix of
*    text2 and the common middle.  Or nil if there was no match.
* @private
--]]
function _diff_halfMatch(text1, text2)
  local longtext = (#text1 > #text2) and text1 or text2
  local shorttext = (#text1 > #text2) and text2 or text1
  if (#longtext < 10) or (#shorttext < 1) then
    return nil  -- Pointless.
  end

  -- First check if the second quarter is the seed for a half-match.
  local hm1 = _diff_halfMatchI(longtext, shorttext, ceil(#longtext / 4))
  -- Check again based on the third quarter.
  local hm2 = _diff_halfMatchI(longtext, shorttext, ceil(#longtext / 2))
  local hm
  if not hm1 and not hm2 then
    return nil
  elseif not hm2 then
    hm = hm1
  elseif not hm1 then
    hm = hm2
  else
    -- Both matched.  Select the longest.
    hm = (#hm1[5] > #hm2[5]) and hm1 or hm2
  end

  -- A half-match was found, sort out the return data.
  local text1_a, text1_b, text2_a, text2_b
  if (#text1 > #text2) then
    text1_a, text1_b = hm[1], hm[2]
    text2_a, text2_b = hm[3], hm[4]
  else
    text2_a, text2_b = hm[1], hm[2]
    text1_a, text1_b = hm[3], hm[4]
  end
  local mid_common = hm[5]
  return text1_a, text1_b, text2_a, text2_b, mid_common
end

-- Define some string matching patterns for matching boundaries.
local punctuation = '^[^a-zA-Z0-9]'
local whitespace = '^%s'
local linebreak = '^[\r\n]'
local blanklineEnd = '\n\r?\n$'
local blanklineStart = '^\r?\n\r?\n'

--[[
* Given two strings, compute a score representing whether the internal
* boundary falls on logical boundaries.
* Scores range from 5 (best) to 0 (worst).
* Closure, makes reference to regex patterns defined above.
* @param {string} one First string.
* @param {string} two Second string.
* @return {number} The score.
--]]
function _diff_cleanupSemanticScore(one, two)
  if (#one == 0) or (#two == 0) then
    -- Edges are the best.
    return 5
  end

  -- Each port of this function behaves slightly differently due to
  -- subtle differences in each language's definition of things like
  -- 'whitespace'.  Since this function's purpose is largely cosmetic,
  -- the choice has been made to use each language's native features
  -- rather than force total conformity.
  local score = 0
  -- One point for non-alphanumeric.
  if strmatch(one, punctuation, -1) or strmatch(two, punctuation) then
    score = score + 1
    -- Two points for whitespace.
    if strmatch(one, whitespace, -1) or strmatch(two, whitespace) then
      score = score + 1
      -- Three points for line breaks.
      if strmatch(one, linebreak, -1) or strmatch(two, linebreak) then
        score = score + 1
        -- Four points for blank lines.
        if strmatch(strsub(one, -3), blanklineEnd)
            or strmatch(two, blanklineStart) then
          score = score + 1
        end
      end
    end
  end
  return score
end

--[[
* Look for single edits surrounded on both sides by equalities
* which can be shifted sideways to align the edit to a word boundary.
* e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function _diff_cleanupSemanticLossless(diffs)
  local pointer = 2
  -- Intentionally ignore the first and last element (don't need checking).
  while diffs[pointer + 1] do
    local prevDiff, nextDiff = diffs[pointer - 1], diffs[pointer + 1]
    if (prevDiff[1] == DIFF_EQUAL) and (nextDiff[1] == DIFF_EQUAL) then
      -- This is a single edit surrounded by equalities.
      local diff = diffs[pointer]

      local equality1 = prevDiff[2]
      local edit = diff[2]
      local equality2 = nextDiff[2]

      -- First, shift the edit as far left as possible.
      local commonOffset = _diff_commonSuffix(equality1, edit)
      if (commonOffset > 0) then
        local commonString = strsub(edit, -commonOffset)
        equality1 = strsub(equality1, 1, -commonOffset - 1)
        edit = commonString .. strsub(edit, 1, -commonOffset - 1)
        equality2 = commonString .. equality2
      end

      -- Second, step character by character right, looking for the best fit.
      local bestEquality1 = equality1
      local bestEdit = edit
      local bestEquality2 = equality2
      local bestScore = _diff_cleanupSemanticScore(equality1, edit)
          + _diff_cleanupSemanticScore(edit, equality2)

      while (strbyte(edit, 1) == strbyte(equality2, 1)) do
        equality1 = equality1 .. strsub(edit, 1, 1)
        edit = strsub(edit, 2) .. strsub(equality2, 1, 1)
        equality2 = strsub(equality2, 2)
        local score = _diff_cleanupSemanticScore(equality1, edit)
            + _diff_cleanupSemanticScore(edit, equality2)
        -- The >= encourages trailing rather than leading whitespace on edits.
        if (score >= bestScore) then
          bestScore = score
          bestEquality1 = equality1
          bestEdit = edit
          bestEquality2 = equality2
        end
      end
      if (prevDiff[2] ~= bestEquality1) then
        -- We have an improvement, save it back to the diff.
        if (#bestEquality1 > 0) then
          diffs[pointer - 1][2] = bestEquality1
        else
          tremove(diffs, pointer - 1)
          pointer = pointer - 1
        end
        diffs[pointer][2] = bestEdit
        if (#bestEquality2 > 0) then
          diffs[pointer + 1][2] = bestEquality2
        else
          tremove(diffs, pointer + 1, 1)
          pointer = pointer - 1
        end
      end
    end
    pointer = pointer + 1
  end
end

--[[
* Reorder and merge like edit sections.  Merge equalities.
* Any edit section can move as long as it doesn't cross an equality.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function _diff_cleanupMerge(diffs)
  diffs[#diffs + 1] = {DIFF_EQUAL, ''}  -- Add a dummy entry at the end.
  local pointer = 1
  local count_delete, count_insert = 0, 0
  local text_delete, text_insert = '', ''
  local commonlength
  while diffs[pointer] do
    local diff_type = diffs[pointer][1]
    if (diff_type == DIFF_INSERT) then
      count_insert = count_insert + 1
      text_insert = text_insert .. diffs[pointer][2]
      pointer = pointer + 1
    elseif (diff_type == DIFF_DELETE) then
      count_delete = count_delete + 1
      text_delete = text_delete .. diffs[pointer][2]
      pointer = pointer + 1
    elseif (diff_type == DIFF_EQUAL) then
      -- Upon reaching an equality, check for prior redundancies.
      if (count_delete > 0) or (count_insert > 0) then
        if (count_delete > 0) and (count_insert > 0) then
          -- Factor out any common prefixies.
          commonlength = _diff_commonPrefix(text_insert, text_delete)
          if (commonlength > 0) then
            local back_pointer = pointer - count_delete - count_insert
            if (back_pointer > 1) and (diffs[back_pointer - 1][1] == DIFF_EQUAL)
                then
              diffs[back_pointer - 1][2] = diffs[back_pointer - 1][2]
                  .. strsub(text_insert, 1, commonlength)
            else
              tinsert(diffs, 1,
                  {DIFF_EQUAL, strsub(text_insert, 1, commonlength)})
              pointer = pointer + 1
            end
            text_insert = strsub(text_insert, commonlength + 1)
            text_delete = strsub(text_delete, commonlength + 1)
          end
          -- Factor out any common suffixies.
          commonlength = _diff_commonSuffix(text_insert, text_delete)
          if (commonlength ~= 0) then
            diffs[pointer][2] =
            strsub(text_insert, -commonlength) .. diffs[pointer][2]
            text_insert = strsub(text_insert, 1, -commonlength - 1)
            text_delete = strsub(text_delete, 1, -commonlength - 1)
          end
        end
        -- Delete the offending records and add the merged ones.
        if (count_delete == 0) then
          tsplice(diffs, pointer - count_insert,
          count_insert, {DIFF_INSERT, text_insert})
        elseif (count_insert == 0) then
          tsplice(diffs, pointer - count_delete,
          count_delete, {DIFF_DELETE, text_delete})
        else
          tsplice(diffs, pointer - count_delete - count_insert,
          count_delete + count_insert,
          {DIFF_DELETE, text_delete}, {DIFF_INSERT, text_insert})
        end
        pointer = pointer - count_delete - count_insert
            + (count_delete>0 and 1 or 0) + (count_insert>0 and 1 or 0) + 1
      elseif (pointer > 1) and (diffs[pointer - 1][1] == DIFF_EQUAL) then
        -- Merge this equality with the previous one.
        diffs[pointer - 1][2] = diffs[pointer - 1][2] .. diffs[pointer][2]
        tremove(diffs, pointer)
      else
        pointer = pointer + 1
      end
      count_insert, count_delete = 0, 0
      text_delete, text_insert = '', ''
    end
  end
  if (diffs[#diffs][2] == '') then
    diffs[#diffs] = nil  -- Remove the dummy entry at the end.
  end

  -- Second pass: look for single edits surrounded on both sides by equalities
  -- which can be shifted sideways to eliminate an equality.
  -- e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
  local changes = false
  pointer = 2
  -- Intentionally ignore the first and last element (don't need checking).
  while (pointer < #diffs) do
    local prevDiff, nextDiff = diffs[pointer - 1], diffs[pointer + 1]
    if (prevDiff[1] == DIFF_EQUAL) and (nextDiff[1] == DIFF_EQUAL) then
      -- This is a single edit surrounded by equalities.
      local diff = diffs[pointer]
      local currentText = diff[2]
      local prevText = prevDiff[2]
      local nextText = nextDiff[2]
      if (strsub(currentText, -#prevText) == prevText) then
        -- Shift the edit over the previous equality.
        diff[2] = prevText .. strsub(currentText, 1, -#prevText - 1)
        nextDiff[2] = prevText .. nextDiff[2]
        tremove(diffs, pointer - 1)
        changes = true
      elseif (strsub(currentText, 1, #nextText) == nextText) then
        -- Shift the edit over the next equality.
        prevDiff[2] = prevText .. nextText
        diff[2] = strsub(currentText, #nextText + 1) .. nextText
        tremove(diffs, pointer + 1)
        changes = true
      end
    end
    pointer = pointer + 1
  end
  -- If shifts were made, the diff needs reordering and another shift sweep.
  if changes then
    -- LUANOTE: no return value, but necessary to use 'return' to get
    -- tail calls.
    return _diff_cleanupMerge(diffs)
  end
end

--[[
* loc is a location in text1, compute and return the equivalent location in
* text2.
* e.g. 'The cat' vs 'The big cat', 1->1, 5->8
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @param {number} loc Location within text1.
* @return {number} Location within text2.
--]]
function _diff_xIndex(diffs, loc)
  local chars1 = 1
  local chars2 = 1
  local last_chars1 = 1
  local last_chars2 = 1
  local x
  for _x, diff in ipairs(diffs) do
    x = _x
    if (diff[1] ~= DIFF_INSERT) then   -- Equality or deletion.
      chars1 = chars1 + #diff[2]
    end
    if (diff[1] ~= DIFF_DELETE) then   -- Equality or insertion.
      chars2 = chars2 + #diff[2]
    end
    if (chars1 > loc) then   -- Overshot the location.
      break
    end
    last_chars1 = chars1
    last_chars2 = chars2
  end
  -- Was the location deleted?
  if diffs[x + 1] and (diffs[x][1] == DIFF_DELETE) then
    return last_chars2
  end
  -- Add the remaining character length.
  return last_chars2 + (loc - last_chars1)
end

--[[
* Compute and return the source text (all equalities and deletions).
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Source text.
--]]
function _diff_text1(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    if (diff[1] ~= DIFF_INSERT) then
      text[#text + 1] = diff[2]
    end
  end
  return tconcat(text)
end

--[[
* Compute and return the destination text (all equalities and insertions).
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Destination text.
--]]
function _diff_text2(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    if (diff[1] ~= DIFF_DELETE) then
      text[#text + 1] = diff[2]
    end
  end
  return tconcat(text)
end

--[[
* Crush the diff into an encoded string which describes the operations
* required to transform text1 into text2.
* E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
* Operations are tab-separated.  Inserted text is escaped using %xx notation.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Delta text.
--]]
function _diff_toDelta(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    local op, data = diff[1], diff[2]
    if (op == DIFF_INSERT) then
      text[x] = '+' .. gsub(data, percentEncode_pattern, percentEncode_replace)
    elseif (op == DIFF_DELETE) then
      text[x] = '-' .. #data
    else--if (op == DIFF_EQUAL) then
      text[x] = '=' .. #data
    end
  end
  return tconcat(text, '\t')
end


--[[
* Given the original text1, and an encoded string which describes the
* operations required to transform text1 into text2, compute the full diff.
* @param {string} text1 Source string for the diff.
* @param {string} delta Delta text.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @throws {Errorend If invalid input.
--]]
function _diff_fromDelta(text1, delta)
  local diffs = {}
  local diffsLength = 0  -- Keeping our own length var is faster
  local pointer = 1  -- Cursor in text1
  for token in gmatch(delta, '[^\t]+') do
    -- Each token begins with a one character parameter which specifies the
    -- operation of this token (delete, insert, equality).
    local tokenchar, param = strsub(token, 1, 1), strsub(token, 2)
    if (tokenchar == '+') then
      local invalidDecode = false
      local decoded = gsub(param, '%%(.?.?)',
          function(c)
            local n = tonumber(c, 16)
            if (#c ~= 2) or (n == nil) then
              invalidDecode = true
              return ''
            end
            return strchar(n)
          end)
      if invalidDecode then
        -- Malformed URI sequence.
        error('Illegal escape in _diff_fromDelta: ' .. param)
      end
      diffsLength = diffsLength + 1
      diffs[diffsLength] = {DIFF_INSERT, decoded}
    elseif (tokenchar == '-') or (tokenchar == '=') then
      local n = tonumber(param)
      if (n == nil) or (n < 0) then
        error('Invalid number in _diff_fromDelta: ' .. param)
      end
      local text = strsub(text1, pointer, pointer + n - 1)
      pointer = pointer + n
      if (tokenchar == '=') then
        diffsLength = diffsLength + 1
        diffs[diffsLength] = {DIFF_EQUAL, text}
      else
        diffsLength = diffsLength + 1
        diffs[diffsLength] = {DIFF_DELETE, text}
      end
    else
      error('Invalid diff operation in _diff_fromDelta: ' .. token)
    end
  end
  if (pointer ~= #text1 + 1) then
    error('Delta length (' .. (pointer - 1)
        .. ') does not equal source text length (' .. #text1 .. ').')
  end
  return diffs
end

-- ---------------------------------------------------------------------------
--  MATCH API
-- ---------------------------------------------------------------------------

local _match_bitap, _match_alphabet

--[[
* Locate the best instance of 'pattern' in 'text' near 'loc'.
* @param {string} text The text to search.
* @param {string} pattern The pattern to search for.
* @param {number} loc The location to search around.
* @return {number?} Best match index or -1.
--]]
function match_main(text, pattern, loc)
  if (text == pattern) then
    -- Shortcut (potentially not guaranteed by the algorithm)
    return 1
  elseif (#text == 0) then
    -- Nothing to match.
    return -1
  end
  loc = max(1, min(loc, #text))
  if (strsub(text, loc, loc + #pattern - 1) == pattern) then
    -- Perfect match at the perfect spot!  (Includes case of null pattern)
    return loc
  else
    -- Do a fuzzy compare.
    return _match_bitap(text, pattern, loc)
  end
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE MATCH FUNCTIONS
-- ---------------------------------------------------------------------------

--[[
* Initialise the alphabet for the Bitap algorithm.
* @param {string} pattern The text to encode.
* @return {Object} Hash of character locations.
* @private
--]]
function _match_alphabet (pattern)
  local s = {}
  local i = 0
  for c in gmatch(pattern, '.') do
    s[c] = bor(s[c] or 0, lshift(1, #pattern - i - 1))
    i = i + 1
  end
  return s
end

--[[
* Locate the best instance of 'pattern' in 'text' near 'loc' using the
* Bitap algorithm.
* @param {string} text The text to search.
* @param {string} pattern The pattern to search for.
* @param {number} loc The location to search around.
* @return {number?} Best match index or -1.
* @private
--]]
function _match_bitap(text, pattern, loc)
  if (#pattern > Match_MaxBits) then
    error('Pattern too long.')
  end

  -- Initialise the alphabet.
  local s = _match_alphabet(pattern)

  --[[
  * Compute and return the score for a match with e errors and x location.
  * Accesses loc and pattern through being a closure.
  * @param {number} e Number of errors in match.
  * @param {number} x Location of match.
  * @return {number} Overall score for match (0.0 = good, 1.0 = bad).
  * @private
  --]]
  local function _match_bitapScore(e, x)
    local accuracy = e / #pattern
    local proximity = abs(loc - x)
    if (Match_Distance == 0) then
      -- Dodge divide by zero error.
      return (proximity == 0) and 1 or accuracy
    end
    return accuracy + (proximity / Match_Distance)
  end

  -- Highest score beyond which we give up.
  local score_threshold = Match_Threshold
  -- Is there a nearby exact match? (speedup)
  local best_loc = indexOf(text, pattern, loc)
  if best_loc then
    score_threshold = min(_match_bitapScore(0, best_loc), score_threshold)
    -- LUANOTE: Ideally we'd also check from the other direction, but Lua
    -- doesn't have an efficent lastIndexOf function.
  end

  -- Initialise the bit arrays.
  local matchmask = lshift(1, #pattern - 1)
  best_loc = -1

  local bin_min, bin_mid
  local bin_max = #pattern + #text
  local last_rd
  for d = 0, #pattern - 1, 1 do
    -- Scan for the best match; each iteration allows for one more error.
    -- Run a binary search to determine how far from 'loc' we can stray at this
    -- error level.
    bin_min = 0
    bin_mid = bin_max
    while (bin_min < bin_mid) do
      if (_match_bitapScore(d, loc + bin_mid) <= score_threshold) then
        bin_min = bin_mid
      else
        bin_max = bin_mid
      end
      bin_mid = floor(bin_min + (bin_max - bin_min) / 2)
    end
    -- Use the result from this iteration as the maximum for the next.
    bin_max = bin_mid
    local start = max(1, loc - bin_mid + 1)
    local finish = min(loc + bin_mid, #text) + #pattern

    local rd = {}
    for j = start, finish do
      rd[j] = 0
    end
    rd[finish + 1] = lshift(1, d) - 1
    for j = finish, start, -1 do
      local charMatch = s[strsub(text, j - 1, j - 1)] or 0
      if (d == 0) then  -- First pass: exact match.
        rd[j] = band(bor((rd[j + 1] * 2), 1), charMatch)
      else
        -- Subsequent passes: fuzzy match.
        -- Functions instead of operators make this hella messy.
        rd[j] = bor(
                band(
                  bor(
                    lshift(rd[j + 1], 1),
                    1
                  ),
                  charMatch
                ),
                bor(
                  bor(
                    lshift(bor(last_rd[j + 1], last_rd[j]), 1),
                    1
                  ),
                  last_rd[j + 1]
                )
              )
      end
      if (band(rd[j], matchmask) ~= 0) then
        local score = _match_bitapScore(d, j - 1)
        -- This match will almost certainly be better than any existing match.
        -- But check anyway.
        if (score <= score_threshold) then
          -- Told you so.
          score_threshold = score
          best_loc = j - 1
          if (best_loc > loc) then
            -- When passing loc, don't exceed our current distance from loc.
            start = max(1, loc * 2 - best_loc)
          else
            -- Already passed loc, downhill from here on in.
            break
          end
        end
      end
    end
    -- No hope for a (better) match at greater error levels.
    if (_match_bitapScore(d + 1, loc) > score_threshold) then
      break
    end
    last_rd = rd
  end
  return best_loc
end

-- -----------------------------------------------------------------------------
-- PATCH API
-- -----------------------------------------------------------------------------

local _patch_addContext,
      _patch_deepCopy,
      _patch_addPadding,
      _patch_splitMax,
      _patch_appendText,
      _new_patch_obj

--[[
* Compute a list of patches to turn text1 into text2.
* Use diffs if provided, otherwise compute it ourselves.
* There are four ways to call this function, depending on what data is
* available to the caller:
* Method 1:
* a = text1, b = text2
* Method 2:
* a = diffs
* Method 3 (optimal):
* a = text1, b = diffs
* Method 4 (deprecated, use method 3):
* a = text1, b = text2, c = diffs
*
* @param {string|Array.<Array.<number|string>>} a text1 (methods 1,3,4) or
* Array of diff tuples for text1 to text2 (method 2).
* @param {string|Array.<Array.<number|string>>} opt_b text2 (methods 1,4) or
* Array of diff tuples for text1 to text2 (method 3) or undefined (method 2).
* @param {string|Array.<Array.<number|string>>} opt_c Array of diff tuples for
* text1 to text2 (method 4) or undefined (methods 1,2,3).
* @return {Array.<_new_patch_obj>} Array of patch objects.
--]]
function patch_make(a, opt_b, opt_c)
  local text1, diffs
  local type_a, type_b, type_c = type(a), type(opt_b), type(opt_c)
  if (type_a == 'string') and (type_b == 'string') and (type_c == 'nil') then
    -- Method 1: text1, text2
    -- Compute diffs from text1 and text2.
    text1 = a
    diffs = diff_main(text1, opt_b, true)
    if (#diffs > 2) then
      diff_cleanupSemantic(diffs)
      diff_cleanupEfficiency(diffs)
    end
  elseif (type_a == 'table') and (type_b == 'nil') and (type_c == 'nil') then
    -- Method 2: diffs
    -- Compute text1 from diffs.
    diffs = a
    text1 = _diff_text1(diffs)
  elseif (type_a == 'string') and (type_b == 'table') and (type_c == 'nil') then
    -- Method 3: text1, diffs
    text1 = a
    diffs = opt_b
  elseif (type_a == 'string') and (type_b == 'string') and (type_c == 'table')
      then
    -- Method 4: text1, text2, diffs
    -- text2 is not used.
    text1 = a
    diffs = opt_c
  else
    error('Unknown call format to patch_make.')
  end

  if (diffs[1] == nil) then
    return {}  -- Get rid of the null case.
  end

  local patches = {}
  local patch = _new_patch_obj()
  local patchDiffLength = 0  -- Keeping our own length var is faster.
  local char_count1 = 0  -- Number of characters into the text1 string.
  local char_count2 = 0  -- Number of characters into the text2 string.
  -- Start with text1 (prepatch_text) and apply the diffs until we arrive at
  -- text2 (postpatch_text).  We recreate the patches one by one to determine
  -- context info.
  local prepatch_text, postpatch_text = text1, text1
  for x, diff in ipairs(diffs) do
    local diff_type, diff_text = diff[1], diff[2]

    if (patchDiffLength == 0) and (diff_type ~= DIFF_EQUAL) then
      -- A new patch starts here.
      patch.start1 = char_count1 + 1
      patch.start2 = char_count2 + 1
    end

    if (diff_type == DIFF_INSERT) then
      patchDiffLength = patchDiffLength + 1
      patch.diffs[patchDiffLength] = diff
      patch.length2 = patch.length2 + #diff_text
      postpatch_text = strsub(postpatch_text, 1, char_count2)
          .. diff_text .. strsub(postpatch_text, char_count2 + 1)
    elseif (diff_type == DIFF_DELETE) then
      patch.length1 = patch.length1 + #diff_text
      patchDiffLength = patchDiffLength + 1
      patch.diffs[patchDiffLength] = diff
      postpatch_text = strsub(postpatch_text, 1, char_count2)
          .. strsub(postpatch_text, char_count2 + #diff_text + 1)
    else--if (diff_type == DIFF_EQUAL) then
      if (#diff_text <= Patch_Margin * 2)
          and (patchDiffLength ~= 0) and (#diffs ~= x) then
        -- Small equality inside a patch.
        patchDiffLength = patchDiffLength + 1
        patch.diffs[patchDiffLength] = diff
        patch.length1 = patch.length1 + #diff_text
        patch.length2 = patch.length2 + #diff_text
      elseif (#diff_text >= Patch_Margin * 2) then
        -- Time for a new patch.
        if (patchDiffLength ~= 0) then
          _patch_addContext(patch, prepatch_text)
          patches[#patches + 1] = patch
          patch = _new_patch_obj()
          patchDiffLength = 0
          -- Unlike Unidiff, our patch lists have a rolling context.
          -- http://code.google.com/p/google-diff-match-patch/wiki/Unidiff
          -- Update prepatch text & pos to reflect the application of the
          -- just completed patch.
          prepatch_text = postpatch_text
          char_count1 = char_count2
        end
      end
    end

    -- Update the current character count.
    if (diff_type ~= DIFF_INSERT) then
      char_count1 = char_count1 + #diff_text
    end
    if (diff_type ~= DIFF_DELETE) then
      char_count2 = char_count2 + #diff_text
    end
  end

  -- Pick up the leftover patch if not empty.
  if (patchDiffLength > 0) then
    _patch_addContext(patch, prepatch_text)
    patches[#patches + 1] = patch
  end

  return patches
end

--[[
* Merge a set of patches onto the text.  Return a patched text, as well
* as a list of true/false values indicating which patches were applied.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @param {string} text Old text.
* @return {Array.<string|Array.<boolean>>} Two return values, the
*     new text and an array of boolean values.
--]]
function patch_apply(patches, text)
  if (patches[1] == nil) then
    return {text, {}}
  end

  -- Deep copy the patches so that no changes are made to originals.
  patches = _patch_deepCopy(patches)

  local nullPadding = _patch_addPadding(patches)
  text = nullPadding .. text .. nullPadding

  _patch_splitMax(patches)
  -- delta keeps track of the offset between the expected and actual location
  -- of the previous patch. If there are patches expected at positions 10 and
  -- 20, but the first patch was found at 12, delta is 2 and the second patch
  -- has an effective expected position of 22.
  local delta = 0
  local results = {}
  for x, patch in ipairs(patches) do
    local expected_loc = patch.start2 + delta
    local text1 = _diff_text1(patch.diffs)
    local start_loc
    local end_loc = -1
    if (#text1 > Match_MaxBits) then
      -- _patch_splitMax will only provide an oversized pattern in
      -- the case of a monster delete.
      start_loc = match_main(text,
          strsub(text1, 1, Match_MaxBits), expected_loc)
      if (start_loc ~= -1) then
        end_loc = match_main(text, strsub(text1, -Match_MaxBits),
            expected_loc + #text1 - Match_MaxBits)
        if (end_loc == -1) or (start_loc >= end_loc) then
          -- Can't find valid trailing context.  Drop this patch.
          start_loc = -1
        end
      end
    else
      start_loc = match_main(text, text1, expected_loc)
    end
    if (start_loc == -1) then
      -- No match found.  :(
      results[x] = false
      -- Subtract the delta for this failed patch from subsequent patches.
      delta = delta - patch.length2 - patch.length1
    else
      -- Found a match.  :)
      results[x] = true
      delta = start_loc - expected_loc
      local text2
      if (end_loc == -1) then
        text2 = strsub(text, start_loc, start_loc + #text1 - 1)
      else
        text2 = strsub(text, start_loc, end_loc + Match_MaxBits - 1)
      end
      if (text1 == text2) then
        -- Perfect match, just shove the replacement text in.
        text = strsub(text, 1, start_loc - 1) .. _diff_text2(patch.diffs)
            .. strsub(text, start_loc + #text1)
      else
        -- Imperfect match.  Run a diff to get a framework of equivalent
        -- indices.
        local diffs = diff_main(text1, text2, false)
        if (#text1 > Match_MaxBits)
            and (diff_levenshtein(diffs) / #text1 > Patch_DeleteThreshold) then
          -- The end points match, but the content is unacceptably bad.
          results[x] = false
        else
          _diff_cleanupSemanticLossless(diffs)
          local index1 = 1
          local index2
          for y, mod in ipairs(patch.diffs) do
            if (mod[1] ~= DIFF_EQUAL) then
              index2 = _diff_xIndex(diffs, index1)
            end
            if (mod[1] == DIFF_INSERT) then
              text = strsub(text, 1, start_loc + index2 - 2)
                  .. mod[2] .. strsub(text, start_loc + index2 - 1)
            elseif (mod[1] == DIFF_DELETE) then
              text = strsub(text, 1, start_loc + index2 - 2) .. strsub(text,
                  start_loc + _diff_xIndex(diffs, index1 + #mod[2] - 1))
            end
            if (mod[1] ~= DIFF_DELETE) then
              index1 = index1 + #mod[2]
            end
          end
        end
      end
    end
  end
  -- Strip the padding off.
  text = strsub(text, #nullPadding + 1, -#nullPadding - 1)
  return text, results
end

--[[
* Take a list of patches and return a textual representation.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {string} Text representation of patches.
--]]
function patch_toText(patches)
  local text = {}
  for x, patch in ipairs(patches) do
    _patch_appendText(patch, text)
  end
  return tconcat(text)
end

--[[
* Parse a textual representation of patches and return a list of patch objects.
* @param {string} textline Text representation of patches.
* @return {Array.<_new_patch_obj>} Array of patch objects.
* @throws {Error} If invalid input.
--]]
function patch_fromText(textline)
  local patches = {}
  if (#textline == 0) then
    return patches
  end
  local text = {}
  for line in gmatch(textline, '([^\n]*)') do
    text[#text + 1] = line
  end
  local textPointer = 1
  while (textPointer <= #text) do
    local start1, length1, start2, length2
     = strmatch(text[textPointer], '^@@ %-(%d+),?(%d*) %+(%d+),?(%d*) @@$')
    if (start1 == nil) then
      error('Invalid patch string: "' .. text[textPointer] .. '"')
    end
    local patch = _new_patch_obj()
    patches[#patches + 1] = patch

    start1 = tonumber(start1)
    length1 = tonumber(length1) or 1
    if (length1 == 0) then
      start1 = start1 + 1
    end
    patch.start1 = start1
    patch.length1 = length1

    start2 = tonumber(start2)
    length2 = tonumber(length2) or 1
    if (length2 == 0) then
      start2 = start2 + 1
    end
    patch.start2 = start2
    patch.length2 = length2

    textPointer = textPointer + 1

    while true do
      local line = text[textPointer]
      if (line == nil) then
        break
      end
      local sign; sign, line = strsub(line, 1, 1), strsub(line, 2)

      local invalidDecode = false
      local decoded = gsub(line, '%%(.?.?)',
          function(c)
            local n = tonumber(c, 16)
            if (#c ~= 2) or (n == nil) then
              invalidDecode = true
              return ''
            end
            return strchar(n)
          end)
      if invalidDecode then
        -- Malformed URI sequence.
        error('Illegal escape in patch_fromText: ' .. line)
      end

      line = decoded

      if (sign == '-') then
        -- Deletion.
        patch.diffs[#patch.diffs + 1] = {DIFF_DELETE, line}
      elseif (sign == '+') then
        -- Insertion.
        patch.diffs[#patch.diffs + 1] = {DIFF_INSERT, line}
      elseif (sign == ' ') then
        -- Minor equality.
        patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, line}
      elseif (sign == '@') then
        -- Start of next patch.
        break
      elseif (sign == '') then
        -- Blank line?  Whatever.
      else
        -- WTF?
        error('Invalid patch mode "' .. sign .. '" in: ' .. line)
      end
      textPointer = textPointer + 1
    end
  end
  return patches
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE PATCH FUNCTIONS
-- ---------------------------------------------------------------------------

local patch_meta = {
  __tostring = function(patch)
    local buf = {}
    _patch_appendText(patch, buf)
    return tconcat(buf)
  end
}

--[[
* Class representing one patch operation.
* @constructor
--]]
function _new_patch_obj()
  return setmetatable({
    --[[ @type {Array.<Array.<number|string>>} ]]
    diffs = {};
    --[[ @type {number?} ]]
    start1 = 1; -- nil;
    --[[ @type {number?} ]]
    start2 = 1; -- nil;
    --[[ @type {number} ]]
    length1 = 0;
    --[[ @type {number} ]]
    length2 = 0;
  }, patch_meta)
end

--[[
* Increase the context until it is unique,
* but don't let the pattern expand beyond Match_MaxBits.
* @param {_new_patch_obj} patch The patch to grow.
* @param {string} text Source text.
* @private
--]]
function _patch_addContext(patch, text)
  if (#text == 0) then
    return
  end
  local pattern = strsub(text, patch.start2, patch.start2 + patch.length1 - 1)
  local padding = 0

  -- LUANOTE: Lua's lack of a lastIndexOf function results in slightly
  -- different logic here than in other language ports.
  -- Look for the first two matches of pattern in text.  If two are found,
  -- increase the pattern length.
  local firstMatch = indexOf(text, pattern)
  local secondMatch = nil
  if (firstMatch ~= nil) then
    secondMatch = indexOf(text, pattern, firstMatch + 1)
  end
  while (#pattern == 0 or secondMatch ~= nil)
      and (#pattern < Match_MaxBits - Patch_Margin - Patch_Margin) do
    padding = padding + Patch_Margin
    pattern = strsub(text, max(1, patch.start2 - padding),
    patch.start2 + patch.length1 - 1 + padding)
    firstMatch = indexOf(text, pattern)
    if (firstMatch ~= nil) then
      secondMatch = indexOf(text, pattern, firstMatch + 1)
    else
      secondMatch = nil
    end
  end
  -- Add one chunk for good luck.
  padding = padding + Patch_Margin

  -- Add the prefix.
  local prefix = strsub(text, max(1, patch.start2 - padding), patch.start2 - 1)
  if (#prefix > 0) then
    tinsert(patch.diffs, 1, {DIFF_EQUAL, prefix})
  end
  -- Add the suffix.
  local suffix = strsub(text, patch.start2 + patch.length1,
  patch.start2 + patch.length1 - 1 + padding)
  if (#suffix > 0) then
    patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, suffix}
  end

  -- Roll back the start points.
  patch.start1 = patch.start1 - #prefix
  patch.start2 = patch.start2 - #prefix
  -- Extend the lengths.
  patch.length1 = patch.length1 + #prefix + #suffix
  patch.length2 = patch.length2 + #prefix + #suffix
end

--[[
* Given an array of patches, return another array that is identical.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {Array.<_new_patch_obj>} Array of patch objects.
--]]
function _patch_deepCopy(patches)
  local patchesCopy = {}
  for x, patch in ipairs(patches) do
    local patchCopy = _new_patch_obj()
    local diffsCopy = {}
    for i, diff in ipairs(patch.diffs) do
      diffsCopy[i] = {diff[1], diff[2]}
    end
    patchCopy.diffs = diffsCopy
    patchCopy.start1 = patch.start1
    patchCopy.start2 = patch.start2
    patchCopy.length1 = patch.length1
    patchCopy.length2 = patch.length2
    patchesCopy[x] = patchCopy
  end
  return patchesCopy
end

--[[
* Add some padding on text start and end so that edges can match something.
* Intended to be called only from within patch_apply.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {string} The padding string added to each side.
--]]
function _patch_addPadding(patches)
  local paddingLength = Patch_Margin
  local nullPadding = ''
  for x = 1, paddingLength do
    nullPadding = nullPadding .. strchar(x)
  end

  -- Bump all the patches forward.
  for x, patch in ipairs(patches) do
    patch.start1 = patch.start1 + paddingLength
    patch.start2 = patch.start2 + paddingLength
  end

  -- Add some padding on start of first diff.
  local patch = patches[1]
  local diffs = patch.diffs
  local firstDiff = diffs[1]
  if (firstDiff == nil) or (firstDiff[1] ~= DIFF_EQUAL) then
    -- Add nullPadding equality.
    tinsert(diffs, 1, {DIFF_EQUAL, nullPadding})
    patch.start1 = patch.start1 - paddingLength  -- Should be 0.
    patch.start2 = patch.start2 - paddingLength  -- Should be 0.
    patch.length1 = patch.length1 + paddingLength
    patch.length2 = patch.length2 + paddingLength
  elseif (paddingLength > #firstDiff[2]) then
    -- Grow first equality.
    local extraLength = paddingLength - #firstDiff[2]
    firstDiff[2] = strsub(nullPadding, #firstDiff[2] + 1) .. firstDiff[2]
    patch.start1 = patch.start1 - extraLength
    patch.start2 = patch.start2 - extraLength
    patch.length1 = patch.length1 + extraLength
    patch.length2 = patch.length2 + extraLength
  end

  -- Add some padding on end of last diff.
  patch = patches[#patches]
  diffs = patch.diffs
  local lastDiff = diffs[#diffs]
  if (lastDiff == nil) or (lastDiff[1] ~= DIFF_EQUAL) then
    -- Add nullPadding equality.
    diffs[#diffs + 1] = {DIFF_EQUAL, nullPadding}
    patch.length1 = patch.length1 + paddingLength
    patch.length2 = patch.length2 + paddingLength
  elseif (paddingLength > #lastDiff[2]) then
    -- Grow last equality.
    local extraLength = paddingLength - #lastDiff[2]
    lastDiff[2] = lastDiff[2] .. strsub(nullPadding, 1, extraLength)
    patch.length1 = patch.length1 + extraLength
    patch.length2 = patch.length2 + extraLength
  end

  return nullPadding
end

--[[
* Look through the patches and break up any which are longer than the maximum
* limit of the match algorithm.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
--]]
function _patch_splitMax(patches)
  local x = 1
  while true do
    local patch = patches[x]
    if (patch == nil) then
      return
    end
    if (patch.length1 > Match_MaxBits) then
      local bigpatch = patch
      -- Remove the big old patch.
      tremove(patches, x)
      x = x - 1
      local patch_size = Match_MaxBits
      local start1 = bigpatch.start1
      local start2 = bigpatch.start2
      local precontext = ''
      while bigpatch.diffs[1] do
        -- Create one of several smaller patches.
        local patch = _new_patch_obj()
        local empty = true
        patch.start1 = start1 - #precontext
        patch.start2 = start2 - #precontext
        if (precontext ~= '') then
          patch.length1, patch.length2 = #precontext, #precontext
          patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, precontext}
        end
        while bigpatch.diffs[1] and (patch.length1 < patch_size-Patch_Margin) do
          local diff_type = bigpatch.diffs[1][1]
          local diff_text = bigpatch.diffs[1][2]
          if (diff_type == DIFF_INSERT) then
            -- Insertions are harmless.
            patch.length2 = patch.length2 + #diff_text
            start2 = start2 + #diff_text
            patch.diffs[#(patch.diffs) + 1] = bigpatch.diffs[1]
            tremove(bigpatch.diffs, 1)
            empty = false
          elseif (diff_type == DIFF_DELETE) and (#patch.diffs == 1)
           and (patch.diffs[1][1] == DIFF_EQUAL)
           and (#diff_text > 2 * patch_size) then
            -- This is a large deletion.  Let it pass in one chunk.
            patch.length1 = patch.length1 + #diff_text
            start1 = start1 + #diff_text
            empty = false
            patch.diffs[#patch.diffs + 1] = {diff_type, diff_text}
            tremove(bigpatch.diffs, 1)
          else
            -- Deletion or equality.
            -- Only take as much as we can stomach.
            diff_text = strsub(diff_text, 1,
            patch_size - patch.length1 - Patch_Margin)
            patch.length1 = patch.length1 + #diff_text
            start1 = start1 + #diff_text
            if (diff_type == DIFF_EQUAL) then
              patch.length2 = patch.length2 + #diff_text
              start2 = start2 + #diff_text
            else
              empty = false
            end
            patch.diffs[#patch.diffs + 1] = {diff_type, diff_text}
            if (diff_text == bigpatch.diffs[1][2]) then
              tremove(bigpatch.diffs, 1)
            else
              bigpatch.diffs[1][2]
                  = strsub(bigpatch.diffs[1][2], #diff_text + 1)
            end
          end
        end
        -- Compute the head context for the next patch.
        precontext = _diff_text2(patch.diffs)
        precontext = strsub(precontext, -Patch_Margin)
        -- Append the end context for this patch.
        local postcontext = strsub(_diff_text1(bigpatch.diffs), 1, Patch_Margin)
        if (postcontext ~= '') then
          patch.length1 = patch.length1 + #postcontext
          patch.length2 = patch.length2 + #postcontext
          if patch.diffs[1]
              and (patch.diffs[#patch.diffs][1] == DIFF_EQUAL) then
            patch.diffs[#patch.diffs][2] = patch.diffs[#patch.diffs][2]
                .. postcontext
          else
            patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, postcontext}
          end
        end
        if not empty then
          x = x + 1
          tinsert(patches, x, patch)
        end
      end
    end
    x = x + 1
  end
end

--[[
* Emulate GNU diff's format.
* Header: @@ -382,8 +481,9 @@
* @return {string} The GNU diff string.
--]]
function _patch_appendText(patch, text)
  local coords1, coords2
  local length1, length2 = patch.length1, patch.length2
  local start1, start2 = patch.start1, patch.start2
  local diffs = patch.diffs

  if (length1 == 1) then
    coords1 = start1
  else
    coords1 = ((length1 == 0) and (start1 - 1) or start1) .. ',' .. length1
  end

  if (length2 == 1) then
    coords2 = start2
  else
    coords2 = ((length2 == 0) and (start2 - 1) or start2) .. ',' .. length2
  end
  text[#text + 1] = '@@ -' .. coords1 .. ' +' .. coords2 .. ' @@\n'

  local op
  -- Escape the body of the patch with %xx notation.
  for x, diff in ipairs(patch.diffs) do
    local diff_type = diff[1]
    if (diff_type == DIFF_INSERT) then
      op = '+'
    elseif (diff_type == DIFF_DELETE) then
      op = '-'
    else--if (diff_type == DIFF_EQUAL) then
      op = ' '
    end
    text[#text + 1] = op
        .. gsub(diffs[x][2], percentEncode_pattern, percentEncode_replace)
        .. '\n'
  end

  return text
end

-- Expose the API
_M.DIFF_DELETE = DIFF_DELETE
_M.DIFF_INSERT = DIFF_INSERT
_M.DIFF_EQUAL = DIFF_EQUAL

_M.diff_main = diff_main
_M.diff_cleanupSemantic = diff_cleanupSemantic
_M.diff_cleanupEfficiency = diff_cleanupEfficiency
_M.diff_levenshtein = diff_levenshtein
_M.diff_prettyHtml = diff_prettyHtml

_M.match_main = match_main

_M.patch_make = patch_make
_M.patch_toText = patch_toText
_M.patch_fromText = patch_fromText
_M.patch_apply = patch_apply

-- Expose some non-API functions as well, for testing purposes etc.
_M.diff_commonPrefix = _diff_commonPrefix
_M.diff_commonSuffix = _diff_commonSuffix
_M.diff_halfMatch = _diff_halfMatch
_M.diff_cleanupMerge = _diff_cleanupMerge
_M.diff_cleanupSemanticLossless = _diff_cleanupSemanticLossless
_M.diff_text1 = _diff_text1
_M.diff_text2 = _diff_text2
_M.diff_toDelta = _diff_toDelta
_M.diff_fromDelta = _diff_fromDelta
_M.diff_xIndex = _diff_xIndex
_M.diff_path1 = _diff_path1
_M.diff_path2 = _diff_path2
_M.match_alphabet = _match_alphabet
_M.match_bitap = _match_bitap
_M.new_patch_obj = _new_patch_obj
_M.patch_addContext = _patch_addContext
_M.patch_splitMax = _patch_splitMax
_M.diff_toLines = _diff_toLines
_M.diff_fromLines = _diff_fromLines
_M.diff_map = _diff_map
_M.patch_addPadding = _patch_addPadding

