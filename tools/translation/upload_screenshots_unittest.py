#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import os
import sys

here = os.path.realpath(__file__)
testdata_path = os.path.normpath(os.path.join(here, '..', 'testdata'))

import upload_screenshots


class UploadTests(unittest.TestCase):

  def test_find_screenshots(self):
    screenshots = upload_screenshots.find_screenshots(
        testdata_path,
        os.path.join(testdata_path, 'translation_expectations.pyl'))
    self.assertEquals(1, len(screenshots))
    self.assertEquals(
        os.path.join(testdata_path, 'test_grd', 'IDS_TEST_STRING1.png'),
        screenshots[0])


if __name__ == '__main__':
  unittest.main()
