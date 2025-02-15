// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Joins paths so that the two paths are connected by only 1 '/'.
 * @param {string} a Path.
 * @param {string} b Path.
 * @return {string} Joined path.
 */
function joinPath(a, b) {
  return a.replace(/\/+$/, '') + '/' + b.replace(/^\/+/, '');
}

/**
 * Mock class for DOMFileSystem.
 *
 * @param {string} volumeId Volume ID.
 * @param {string=} opt_rootURL URL string of root which is used in
 *     MockEntry.toURL.
 * @constructor
 */
function MockFileSystem(volumeId, opt_rootURL) {
  /** @type {string} */
  this.name = volumeId;

  /** @type {!Object<!Entry>} */
  this.entries = {};
  this.entries['/'] = new MockDirectoryEntry(this, '');

  /** @type {string} */
  this.rootURL = opt_rootURL || 'filesystem:' + volumeId;
}

MockFileSystem.prototype = {
  get root() { return this.entries['/']; }
};

/**
 * Creates file and directory entries for all the given entries.  Entries can
 * be either string paths or objects containing properties 'fullPath',
 * 'metadata', 'content'.  Paths ending in slashes are interpreted as
 * directories.  All intermediate directories leading up to the
 * files/directories to be created, are also created.
 * @param {!Array<string|Object>} entries An array of either string file paths,
 *     or objects containing 'fullPath' and 'metadata' to populate in this
 *     file system.
 * @param {boolean=} opt_clear Optional, if true clears all entries before
 *     populating.
 */
MockFileSystem.prototype.populate = function(entries, opt_clear) {
  if (opt_clear)
    this.entries = {'/': new MockDirectoryEntry(this, '')};
  entries.forEach(function(entry) {
    var path = entry.fullPath || entry;
    var metadata = entry.metadata || {size: 0};
    var content = entry.content;
    var pathElements = path.split('/');
    pathElements.forEach(function(_, i) {
      var subpath = pathElements.slice(0, i).join('/');
      if (subpath && !(subpath in this.entries))
        this.entries[subpath] = new MockDirectoryEntry(this, subpath, metadata);
    }.bind(this));

    // If the path doesn't end in a slash, create a file.
    if (!/\/$/.test(path))
      this.entries[path] = new MockFileEntry(this, path, metadata, content);
  }.bind(this));
};

/**
 * Returns all children of the supplied directoryEntry.
 * @param  {!DirectoryEntry} directoryEntry
 * @return {!Array<!Entry>}
 * @private
 */
MockFileSystem.prototype.findChildren_ = function(directory) {
  var parentPath = directory.fullPath.replace(/\/?$/, '/');
  var children = [];
  for (var path in this.entries) {
    if (path.indexOf(parentPath) === 0 && path !== parentPath) {
      var nextSeparator = path.indexOf('/', parentPath.length);
      // Add immediate children files and directories...
      if (nextSeparator === -1 ||
          nextSeparator === path.length - 1) {
        children.push(this.entries[path]);
      }
    }
  }
  return children;
};

/**
 * Base class of mock entries.
 *
 * @param {MockFileSystem} filesystem File system where the entry is localed.
 * @param {string} fullPath Full path of the entry.
 * @param {Object} metadata Metadata.
 * @constructor
 */
function MockEntry(filesystem, fullPath, metadata) {
  filesystem.entries[fullPath] = this;
  this.filesystem = filesystem;
  this.fullPath = fullPath;
  this.metadata = metadata;
  this.removed_ = false;
}

MockEntry.prototype = {
  /**
   * @return {string} Name of the entry.
   */
  get name() {
    return this.fullPath.replace(/^.*\//, '');
  }
};

/**
 * Obtains metadata of the entry.
 *
 * @param {function(!Metadata)} onSuccess Function to take the metadata.
 * @param {function(Error)} onError
 */
MockEntry.prototype.getMetadata = function(onSuccess, onError) {
  new Promise(function(fulfill, reject) {
    if (this.filesystem && !this.filesystem.entries[this.fullPath])
      reject(new DOMError('NotFoundError'));
    else
      onSuccess(this.metadata);
  }.bind(this))
      .then(onSuccess, onError);
};

/**
 * Returns fake URL.
 *
 * @return {string} Fake URL.
 */
MockEntry.prototype.toURL = function() {
  var segments = this.fullPath.split('/');
  for (var i = 0; i < segments.length; i++) {
    segments[i] = encodeURIComponent(segments[i]);
  }

  return this.filesystem.rootURL + segments.join('/');
};

/**
 * Obtains parent directory.
 *
 * @param {function(MockDirectoryEntry)} onSuccess Callback invoked with
 *     the parent directory.
 * @param {function(Object)} onError Callback invoked with an error
 *     object.
 */
MockEntry.prototype.getParent = function(
    onSuccess, onError) {
  var path = this.fullPath.replace(/\/[^\/]+$/, '') || '/';
  if (this.filesystem.entries[path])
    onSuccess(this.filesystem.entries[path]);
  else
    onError({name: util.FileError.NOT_FOUND_ERR});
};

/**
 * Moves the entry to the directory.
 *
 * @param {MockDirectoryEntry} parent Destination directory.
 * @param {string=} opt_newName New name.
 * @param {function(MockDirectoryEntry)} onSuccess Callback invoked with the
 *     moved entry.
 * @param {function(Object)} onError Callback invoked with an error object.
 */
MockEntry.prototype.moveTo = function(parent, opt_newName, onSuccess, onError) {
  Promise.resolve().then(function() {
    this.filesystem.entries[this.fullPath] = null;
    return this.clone(
        joinPath(parent.fullPath, opt_newName || this.name),
        parent.filesystem);
  }.bind(this)).then(onSuccess, onError);
};

/**
 * @param {MockDirectoryEntry} parent
 * @param {string=} opt_newName
 * @param {function(!MockEntry)} successCallback
 * @param {function} errorCallback
 */
MockEntry.prototype.copyTo =
    function(parent, opt_newName, successCallback, errorCallback) {
  Promise.resolve().then(function() {
    return this.clone(
        joinPath(parent.fullPath, opt_newName || this.name),
        parent.filesystem);
  }.bind(this)).then(successCallback, errorCallback);
};

/**
 * Removes the entry.
 *
 * @param {function()} onSuccess Success callback.
 * @param {function(Object)} onError Callback invoked with an error object.
 */
MockEntry.prototype.remove = function(onSuccess, onError) {
  this.removed_ = true;
  Promise.resolve().then(function() {
    this.filesystem.entries[this.fullPath] = null;
  }.bind(this)).then(onSuccess, onError);
};

/**
 * Asserts that the entry was removed.
 */
MockEntry.prototype.assertRemoved = function() {
  assertTrue(this.removed_);
};

/**
 * Clones the entry with the new fullpath.
 *
 * @param {string} fullpath New fullpath.
 * @param {FileSystem=} opt_filesystem New file system
 * @return {MockEntry} Cloned entry.
 */
MockEntry.prototype.clone = function(fullpath, opt_filesystem) {
  throw new Error('Not implemented.');
};

/**
 * Mock class for FileEntry.
 *
 * @param {FileSystem} filesystem File system where the entry is localed.
 * @param {string} fullPath Full path for the entry.
 * @param {Object} metadata Metadata.
 * @param {Blob=} opt_content Optional content.
 * @extends {MockEntry}
 * @constructor
 */
function MockFileEntry(filesystem, fullPath, metadata, opt_content) {
  MockEntry.call(this, filesystem, fullPath, metadata);
  this.content = opt_content || new Blob([]);
  this.isFile = true;
  this.isDirectory = false;
}

MockFileEntry.prototype = {
  __proto__: MockEntry.prototype
};

/**
 * Returns a File that this represents.
 *
 * @param {function(!File)} onSuccess Function to take the file.
 * @param {function(Error)} onError
 */
MockFileEntry.prototype.file = function(onSuccess, onError) {
  onSuccess(new File([this.content], this.toURL()));
};

/**
 * @override
 */
MockFileEntry.prototype.clone = function(path, opt_filesystem) {
  return new MockFileEntry(
      opt_filesystem || this.filesystem, path, this.metadata, this.content);
};

/**
 * Mock class for DirectoryEntry.
 *
 * @param {FileSystem} filesystem File system where the entry is localed.
 * @param {string} fullPath Full path for the entry.
 * @param {Object} metadata Metadata.
 * @extends {MockEntry}
 * @constructor
 */
function MockDirectoryEntry(filesystem, fullPath, metadata) {
  MockEntry.call(this, filesystem, fullPath, metadata);
  this.isFile = false;
  this.isDirectory = true;
}

MockDirectoryEntry.prototype = {
  __proto__: MockEntry.prototype
};

/**
 * @override
 */
MockDirectoryEntry.prototype.clone = function(path, opt_filesystem) {
  return new MockDirectoryEntry(opt_filesystem || this.filesystem, path);
};

/**
 * Returns all children of the supplied directoryEntry.
 * @return {!Array<!Entry>}
 */
MockDirectoryEntry.prototype.getAllChildren = function() {
  return this.filesystem.findChildren_(this);
};

  /**
 * Returns a file under the directory.
 *
 * @param {string} path Path.
 * @param {Object} option Option.
 * @param {callback(MockFileEntry)} onSuccess Success callback.
 * @param {callback(Object)} onError Failure callback;
 */
MockDirectoryEntry.prototype.getFile = function(
    path, option, onSuccess, onError) {
  var fullPath = path[0] === '/' ? path : joinPath(this.fullPath, path);
  if (!this.filesystem.entries[fullPath])
    onError({name: util.FileError.NOT_FOUND_ERR});
  else if (!(this.filesystem.entries[fullPath] instanceof MockFileEntry))
    onError({name: util.FileError.TYPE_MISMATCH_ERR});
  else
    onSuccess(this.filesystem.entries[fullPath]);
};

/**
 * Returns a directory under the directory.
 *
 * @param {string} path Path.
 * @param {Object} option Option.
 * @param {callback(MockDirectoryEntry)} onSuccess Success callback.
 * @param {callback(Object)} onError Failure callback;
 */
MockDirectoryEntry.prototype.getDirectory =
    function(path, option, onSuccess, onError) {
  var fullPath = path[0] === '/' ? path : joinPath(this.fullPath, path);
  var result = this.filesystem.entries[fullPath];
  if (result) {
    if (!(result instanceof MockDirectoryEntry))
      onError({name: util.FileError.TYPE_MISMATCH_ERR});
    else if (option['create'] && option['exclusive'])
      onError({name: util.FileError.PATH_EXISTS_ERR});
    else
      onSuccess(result);
  } else {
    if (!option['create']) {
      onError({name: util.FileError.NOT_FOUND_ERR});
    } else {
      var newEntry = new MockDirectoryEntry(this.filesystem, fullPath);
      this.filesystem.entries[fullPath] = newEntry;
      onSuccess(newEntry);
    }
  }
};

/**
 * Creates a MockDirectoryReader for the entry.
 * @return {DirectoryReader} A directory reader.
 */
MockDirectoryEntry.prototype.createReader = function() {
  return new MockDirectoryReader(this.filesystem.findChildren_(this));
};

/**
 * Mock class for DirectoryReader.
 * @param {!Array<!Entry>} entries
 */
function MockDirectoryReader(entries) {
  this.entries_ = entries;
}

/**
 * Returns entries from the filesystem associated with this directory
 * in chunks of 2.
 *
 * @param {function(Array)} success Success callback.
 * @param {function} error Error callback.
 */
MockDirectoryReader.prototype.readEntries = function(success, error) {
  if (this.entries_.length > 0) {
    var chunk = this.entries_.splice(0, 2);
    success(chunk);
  } else {
    success([]);
  }
};
