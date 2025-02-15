#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for extract_unwind_tables.py

This test suite contains various tests for extracting CFI tables from breakpad
symbol files.
"""

import optparse
import os
import struct
import sys
import tempfile
import unittest

import extract_unwind_tables

sys.path.append(os.path.join(os.path.dirname(__file__), "gyp"))
from util import build_utils


class TestExtractUnwindTables(unittest.TestCase):
  def testExtractCfi(self):
    with tempfile.NamedTemporaryFile() as input_file, \
        tempfile.NamedTemporaryFile() as output_file:
      input_file.write("""
MODULE Linux arm CDE12FE1DF2B37A9C6560B4CBEE056420 lib_chrome.so
INFO CODE_ID E12FE1CD2BDFA937C6560B4CBEE05642
FILE 0 ../../base/allocator/allocator_check.cc
FILE 1 ../../base/allocator/allocator_extension.cc
FILE 2 ../../base/allocator/allocator_shim.cc
FUNC 1adcb60 54 0 i2d_name_canon
1adcb60 1a 509 17054
3b94c70 2 69 40
PUBLIC e17001 0 assist_ranker::(anonymous namespace)::FakePredict::Initialize()
PUBLIC e17005 0 (anonymous namespace)::FileDeleter(base::File)
STACK CFI INIT e17000 4 .cfa: sp 0 + .ra: lr
STACK CFI INIT 0 4 .cfa: sp 0 + .ra: lr
STACK CFI 2 .cfa: sp 4 +
STACK CFI 4 .cfa: sp 12 + .ra: .cfa -8 + ^ r7: .cfa -12 + ^
STACK CFI 6 .cfa: sp 16 +
STACK CFI INIT e1a96e 20 .cfa: sp 0 + .ra: lr
STACK CFI e1a970 .cfa: sp 4 +
STACK CFI e1a972 .cfa: sp 12 + .ra: .cfa -8 + ^ r7: .cfa -12 + ^
STACK CFI e1a974 .cfa: sp 16 +
STACK CFI INIT e1a1e4 b0 .cfa: sp 0 + .ra: lr
STACK CFI e1a1e6 .cfa: sp 16 + .ra: .cfa -4 + ^ r4: .cfa -16 + ^ r5: .cfa -12 +
STACK CFI e1a1e8 .cfa: sp 80 +
STACK CFI INIT 0 4 .cfa: sp 0 + .ra: lr
STACK CFI INIT 3b92e24 3c .cfa: sp 0 + .ra: lr
STACK CFI 3b92e4c .cfa: sp 16 + .ra: .cfa -12 + ^
STACK CFI INIT e17004 0 .cfa: sp 0 + .ra: lr
STACK CFI e17004 2 .cfa: sp 0 + .ra: lr
STACK CFI INIT 3b92e70 38 .cfa: sp 0 + .ra: lr
STACK CFI 3b92e74 .cfa: sp 8 + .ra: .cfa -4 + ^ r4: .cfa -8 + ^
STACK CFI 3b92e90 .cfa: sp 0 + .ra: .ra r4: r4
STACK CFI INIT 3b93114 6c .cfa: sp 0 + .ra: lr
STACK CFI 3b93118 .cfa: r7 16 + .ra: .cfa -4 + ^
""")
      input_file.flush()
      extract_unwind_tables._ParseCfiData(input_file.name, output_file.name)

      expected_output_rows = [
        0xe1a1e4 | 1, 0xb0,
        0xe1a1e6    , 16 + 4 / 4,
        0xe1a1e8    , 80 + 0,

        0xe1a96e | 1, 0x20,
        0xe1a970    , 4 + 0,
        0xe1a972    , 12 + 8 / 4,
        0xe1a974    , 16 + 0,

        0x3b92e24 | 1, 0x3c,
        0x3b92e4c    , 16 + 12 / 4
      ]
      actual_output = []
      with open(output_file.name, 'rb') as f:
        while True:
          read = f.read(4)
          if not read:
            break
          actual_output.append(struct.unpack('i', read)[0])
      self.assertEqual(expected_output_rows, actual_output)


if __name__ == '__main__':
  unittest.main()
