// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8EmbedderGraphBuilder_h
#define V8EmbedderGraphBuilder_h

#include "platform/bindings/ScriptWrappableVisitor.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"

namespace blink {

class V8EmbedderGraphBuilder : public ScriptWrappableVisitor,
                               public v8::PersistentHandleVisitor {
 public:
  using Traceable = const void*;
  using Graph = v8::EmbedderGraph;

  V8EmbedderGraphBuilder(v8::Isolate*, Graph*);

  static void BuildEmbedderGraphCallback(v8::Isolate*, v8::EmbedderGraph*);
  void BuildEmbedderGraph();

  // v8::PersistentHandleVisitor override.
  void VisitPersistentHandle(v8::Persistent<v8::Value>*,
                             uint16_t class_id) override;

 protected:
  // ScriptWrappableVisitor overrides.
  void Visit(const TraceWrapperV8Reference<v8::Value>&) const final;
  void Visit(const TraceWrapperDescriptor&) const final;
  void Visit(DOMWrapperMap<ScriptWrappable>*,
             const ScriptWrappable*) const final;

 private:
  class EmbedderNode : public Graph::Node {
   public:
    EmbedderNode(const char* name, Graph::Node* wrapper)
        : name_(name), wrapper_(wrapper) {}

    // Graph::Node overrides.
    const char* Name() override { return name_; }
    size_t SizeInBytes() override { return 0; }
    Graph::Node* WrapperNode() override { return wrapper_; }

   private:
    const char* name_;
    Graph::Node* wrapper_;
  };

  class EmbedderRootNode : public EmbedderNode {
   public:
    explicit EmbedderRootNode(const char* name) : EmbedderNode(name, nullptr) {}
    // Graph::Node override.
    bool IsRootNode() { return true; }
  };

  class ParentScope {
    STACK_ALLOCATED();

   public:
    ParentScope(V8EmbedderGraphBuilder* visitor, Graph::Node* parent)
        : visitor_(visitor) {
      DCHECK_EQ(visitor->current_parent_, nullptr);
      visitor->current_parent_ = parent;
    }
    ~ParentScope() { visitor_->current_parent_ = nullptr; }

   private:
    V8EmbedderGraphBuilder* visitor_;
  };

  struct WorklistItem {
    Graph::Node* node;
    Traceable traceable;
    TraceWrappersCallback trace_wrappers_callback;
  };

  WorklistItem ToWorklistItem(Graph::Node*,
                              const TraceWrapperDescriptor&) const;

  Graph::Node* GraphNode(const v8::Local<v8::Value>&) const;
  Graph::Node* GraphNode(Traceable,
                         const char* name,
                         Graph::Node* wrapper) const;

  void VisitPendingActivities();
  void VisitTransitiveClosure();

  v8::Isolate* isolate_;
  Graph::Node* current_parent_;
  mutable Graph* graph_;
  mutable HashSet<Traceable> visited_;
  mutable HashMap<Traceable, Graph::Node*> graph_node_;
  mutable Deque<WorklistItem> worklist_;
};

}  // namespace blink

#endif  // V8EmbedderGraphBuilder_h
