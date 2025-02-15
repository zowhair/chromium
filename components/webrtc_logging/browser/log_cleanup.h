// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBRTC_LOGGING_BROWSER_LOG_CLEANUP_H_
#define COMPONENTS_WEBRTC_LOGGING_BROWSER_LOG_CLEANUP_H_

namespace base {
class FilePath;
class Time;
}  // namespace base

namespace webrtc_logging {

// Deletes logs files older that 5 days. Updates the log file list. Must be
// called on the FILE thread.
void DeleteOldWebRtcLogFiles(const base::FilePath& log_dir);

// Deletes logs files older that 5 days and logs younger than
// |delete_begin_time|. Updates the log file list. If |delete_begin_time| is
// base::time::Max(), no recent logs will be deleted, and the function is
// equal to DeleteOldWebRtcLogFiles(). Must be called on the FILE thread.
void DeleteOldAndRecentWebRtcLogFiles(const base::FilePath& log_dir,
                                      const base::Time& delete_begin_time);

}  // namespace webrtc_logging

#endif  // COMPONENTS_WEBRTC_LOGGING_BROWSER_LOG_CLEANUP_H_
