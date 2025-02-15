// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UNZIP_SERVICE_PUBLIC_CPP_UNZIP_H_
#define COMPONENTS_UNZIP_SERVICE_PUBLIC_CPP_UNZIP_H_

#include <memory>

#include "base/callback_forward.h"

namespace base {
class FilePath;
}

namespace service_manager {
class Connector;
}

namespace unzip {

// Unzips |zip_file| into |output_dir|.
using UnzipCallback = base::OnceCallback<void(bool result)>;
void Unzip(std::unique_ptr<service_manager::Connector> connector,
           const base::FilePath& zip_file,
           const base::FilePath& output_dir,
           UnzipCallback result_callback);

// Similar to |Unzip| but only unzips files that |filter_callback| vetted.
// Note that |filter_callback| may be invoked from a background thread.
using UnzipFilterCallback =
    base::RepeatingCallback<bool(const base::FilePath& path)>;
void UnzipWithFilter(std::unique_ptr<service_manager::Connector> connector,
                     const base::FilePath& zip_file,
                     const base::FilePath& output_dir,
                     UnzipFilterCallback filter_callback,
                     UnzipCallback result_callback);

}  // namespace unzip

#endif  // COMPONENTS_UNZIP_SERVICE_PUBLIC_CPP_UNZIP_H_
