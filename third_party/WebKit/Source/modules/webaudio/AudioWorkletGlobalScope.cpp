// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletGlobalScope.h"

#include <memory>
#include <utility>

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/ToV8ForCore.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8ObjectBuilder.h"
#include "bindings/core/v8/V8ObjectParser.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "bindings/modules/v8/V8AudioParamDescriptor.h"
#include "bindings/modules/v8/V8AudioWorkletProcessor.h"
#include "core/dom/ExceptionCode.h"
#include "core/messaging/MessagePort.h"
#include "core/typed_arrays/DOMTypedArray.h"
#include "core/workers/GlobalScopeCreationParams.h"
#include "modules/webaudio/AudioBuffer.h"
#include "modules/webaudio/AudioParamDescriptor.h"
#include "modules/webaudio/AudioWorkletProcessor.h"
#include "modules/webaudio/AudioWorkletProcessorDefinition.h"
#include "modules/webaudio/AudioWorkletProcessorErrorState.h"
#include "modules/webaudio/CrossThreadAudioWorkletProcessorInfo.h"
#include "platform/audio/AudioBus.h"
#include "platform/audio/AudioUtilities.h"
#include "platform/bindings/V8BindingMacros.h"
#include "platform/bindings/V8ObjectConstructor.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

AudioWorkletGlobalScope* AudioWorkletGlobalScope::Create(
    std::unique_ptr<GlobalScopeCreationParams> creation_params,
    v8::Isolate* isolate,
    WorkerThread* thread) {
  return new AudioWorkletGlobalScope(std::move(creation_params), isolate,
                                     thread);
}

AudioWorkletGlobalScope::AudioWorkletGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params,
    v8::Isolate* isolate,
    WorkerThread* thread)
    : ThreadedWorkletGlobalScope(std::move(creation_params), isolate, thread) {}

AudioWorkletGlobalScope::~AudioWorkletGlobalScope() = default;

void AudioWorkletGlobalScope::Dispose() {
  DCHECK(IsContextThread());
  is_closing_ = true;
  ThreadedWorkletGlobalScope::Dispose();
}

void AudioWorkletGlobalScope::registerProcessor(
    const String& name,
    const ScriptValue& class_definition,
    ExceptionState& exception_state) {
  DCHECK(IsContextThread());

  if (processor_definition_map_.Contains(name)) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "A class with name:'" + name + "' is already registered.");
    return;
  }

  // TODO(hongchan): this is not stated in the spec, but seems necessary.
  // https://github.com/WebAudio/web-audio-api/issues/1172
  if (name.IsEmpty()) {
    exception_state.ThrowTypeError("The empty string is not a valid name.");
    return;
  }

  v8::Isolate* isolate = ScriptController()->GetScriptState()->GetIsolate();
  v8::Local<v8::Context> context = ScriptController()->GetContext();

  DCHECK(class_definition.V8Value()->IsFunction());
  v8::Local<v8::Function> constructor =
      v8::Local<v8::Function>::Cast(class_definition.V8Value());

  v8::Local<v8::Object> prototype;
  if (!V8ObjectParser::ParsePrototype(context, constructor, &prototype,
                                      &exception_state))
    return;

  v8::Local<v8::Function> process;
  if (!V8ObjectParser::ParseFunction(context, prototype, "process", &process,
                                     &exception_state))
    return;

  // constructor() and process() functions are successfully parsed from the
  // script code, thus create the definition. The rest of parsing process
  // (i.e. parameterDescriptors) is optional.
  AudioWorkletProcessorDefinition* definition =
      AudioWorkletProcessorDefinition::Create(isolate, name, constructor,
                                              process);
  DCHECK(definition);

  v8::Local<v8::Value> parameter_descriptors_value_local;
  bool did_get_parameter_descriptor =
      constructor->Get(context, V8AtomicString(isolate, "parameterDescriptors"))
          .ToLocal(&parameter_descriptors_value_local);

  // If parameterDescriptor() is parsed and has a valid value, create a vector
  // of |AudioParamDescriptor| and pass it to the definition.
  if (did_get_parameter_descriptor &&
      !parameter_descriptors_value_local->IsNullOrUndefined()) {
    HeapVector<AudioParamDescriptor> audio_param_descriptors =
        NativeValueTraits<IDLSequence<AudioParamDescriptor>>::NativeValue(
            isolate, parameter_descriptors_value_local, exception_state);

    if (exception_state.HadException())
      return;

    definition->SetAudioParamDescriptors(audio_param_descriptors);
  }

  processor_definition_map_.Set(name, definition);
}

AudioWorkletProcessor* AudioWorkletGlobalScope::CreateProcessor(
    const String& name,
    MessagePortChannel message_port_channel,
    scoped_refptr<SerializedScriptValue> node_options) {
  DCHECK(IsContextThread());

  // The registered definition is already checked by AudioWorkletNode
  // construction process, so the |definition| here must be valid.
  AudioWorkletProcessorDefinition* definition = FindDefinition(name);
  DCHECK(definition);

  ScriptState* script_state = ScriptController()->GetScriptState();
  ScriptState::Scope scope(script_state);

  // V8 object instance construction: this construction process is here to make
  // the AudioWorkletProcessor class a thin wrapper of V8::Object instance.
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::TryCatch block(isolate);

  // Routes errors/exceptions to the dev console.
  block.SetVerbose(true);

  DCHECK(!processor_creation_params_);
  processor_creation_params_ = std::make_unique<ProcessorCreationParams>(
      name, std::move(message_port_channel));

  v8::Local<v8::Value> argv[] = {
    ToV8(node_options->Deserialize(isolate),
         script_state->GetContext()->Global(),
         isolate)
  };

  // This invokes the static constructor of AudioWorkletProcessor. There is no
  // way to pass additional constructor arguments that are not described in
  // WebIDL, the static constructor will look up |processor_creation_params_| in
  // the global scope to perform the construction properly.
  v8::Local<v8::Value> result;
  bool did_construct =
      V8ScriptRunner::CallAsConstructor(isolate,
                                        definition->ConstructorLocal(isolate),
                                        ExecutionContext::From(script_state),
                                        WTF_ARRAY_LENGTH(argv),
                                        argv)
          .ToLocal(&result);
  processor_creation_params_.reset();

  // If 1) the attempt to call the constructor fails, 2) an error was thrown
  // by the user-supplied constructor code. The invalid construction process
  if (!did_construct || block.HasCaught()) {
    return nullptr;
  }

  // ToImplWithTypeCheck() may return nullptr when the type does not match.
  AudioWorkletProcessor* processor =
      V8AudioWorkletProcessor::ToImplWithTypeCheck(isolate, result);

  if (processor) {
    processor_instances_.push_back(processor);
  }

  return processor;
}

bool AudioWorkletGlobalScope::Process(
    AudioWorkletProcessor* processor,
    Vector<AudioBus*>* input_buses,
    Vector<AudioBus*>* output_buses,
    HashMap<String, std::unique_ptr<AudioFloatArray>>* param_value_map) {
  CHECK_GE(input_buses->size(), 0u);
  CHECK_GE(output_buses->size(), 0u);

  ScriptState* script_state = ScriptController()->GetScriptState();
  ScriptState::Scope scope(script_state);

  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Context> current_context = script_state->GetContext();
  AudioWorkletProcessorDefinition* definition =
      FindDefinition(processor->Name());
  DCHECK(definition);

  v8::TryCatch block(isolate);
  block.SetVerbose(true);

  // Prepare arguments of JS callback (inputs, outputs and param_values) with
  // directly using V8 API because the overhead of
  // ToV8(HeapVector<HeapVector<DOMFloat32Array>>) is not negligible and there
  // is no need to externalize the array buffers.

  // 1st arg of JS callback: inputs
  v8::Local<v8::Array> inputs = v8::Array::New(isolate, input_buses->size());
  uint32_t input_bus_index = 0;
  for (const auto& input_bus : *input_buses) {
    v8::Local<v8::Array> channels =
        v8::Array::New(isolate, input_bus->NumberOfChannels());
    bool success;
    if (!inputs
             ->CreateDataProperty(current_context, input_bus_index++, channels)
             .To(&success)) {
      return false;
    }
    for (uint32_t channel_index = 0;
         channel_index < input_bus->NumberOfChannels(); ++channel_index) {
      v8::Local<v8::ArrayBuffer> array_buffer =
          v8::ArrayBuffer::New(isolate, input_bus->length() * sizeof(float));
      v8::Local<v8::Float32Array> float32_array =
          v8::Float32Array::New(array_buffer, 0, input_bus->length());
      if (!channels
               ->CreateDataProperty(current_context, channel_index,
                                    float32_array)
               .To(&success)) {
        return false;
      }
      const v8::ArrayBuffer::Contents& contents = array_buffer->GetContents();
      memcpy(contents.Data(), input_bus->Channel(channel_index)->Data(),
             input_bus->length() * sizeof(float));
    }
  }

  // 2nd arg of JS callback: outputs
  v8::Local<v8::Array> outputs = v8::Array::New(isolate, output_buses->size());
  uint32_t output_bus_index = 0;
  // |js_output_raw_ptrs| stores raw pointers to underlying array buffers so
  // that we can copy them back to |output_buses|. The raw pointers are valid
  // as long as the v8::ArrayBuffers are alive, i.e. as long as |outputs| is
  // holding v8::ArrayBuffers.
  Vector<Vector<void*>> js_output_raw_ptrs;
  js_output_raw_ptrs.ReserveInitialCapacity(output_buses->size());
  for (const auto& output_bus : *output_buses) {
    js_output_raw_ptrs.UncheckedAppend(Vector<void*>());
    js_output_raw_ptrs.back().ReserveInitialCapacity(
        output_bus->NumberOfChannels());
    v8::Local<v8::Array> channels =
        v8::Array::New(isolate, output_bus->NumberOfChannels());
    bool success;
    if (!outputs
             ->CreateDataProperty(current_context, output_bus_index++, channels)
             .To(&success)) {
      return false;
    }
    for (uint32_t channel_index = 0;
         channel_index < output_bus->NumberOfChannels(); ++channel_index) {
      v8::Local<v8::ArrayBuffer> array_buffer =
          v8::ArrayBuffer::New(isolate, output_bus->length() * sizeof(float));
      v8::Local<v8::Float32Array> float32_array =
          v8::Float32Array::New(array_buffer, 0, output_bus->length());
      if (!channels
               ->CreateDataProperty(current_context, channel_index,
                                    float32_array)
               .To(&success)) {
        return false;
      }
      const v8::ArrayBuffer::Contents& contents = array_buffer->GetContents();
      js_output_raw_ptrs.back().UncheckedAppend(contents.Data());
    }
  }

  // 3rd arg of JS callback: param_values
  v8::Local<v8::Object> param_values = v8::Object::New(isolate);
  for (const auto& param : *param_value_map) {
    const String& param_name = param.key;
    const AudioFloatArray* param_array = param.value.get();
    v8::Local<v8::ArrayBuffer> array_buffer =
        v8::ArrayBuffer::New(isolate, param_array->size() * sizeof(float));
    v8::Local<v8::Float32Array> float32_array =
        v8::Float32Array::New(array_buffer, 0, param_array->size());
    bool success;
    if (!param_values
             ->CreateDataProperty(current_context,
                                  V8String(isolate, param_name.IsolatedCopy()),
                                  float32_array)
             .To(&success)) {
      return false;
    }
    const v8::ArrayBuffer::Contents& contents = array_buffer->GetContents();
    memcpy(contents.Data(), param_array->Data(),
           param_array->size() * sizeof(float));
  }

  v8::Local<v8::Value> argv[] = {inputs, outputs, param_values};

  // Perform JS function process() in AudioWorkletProcessor instance. The actual
  // V8 operation happens here to make the AudioWorkletProcessor class a thin
  // wrapper of v8::Object instance.
  v8::Local<v8::Value> processor_handle = ToV8(processor, script_state);
  v8::Local<v8::Value> local_result;
  if (!V8ScriptRunner::CallFunction(definition->ProcessLocal(isolate),
                                    ExecutionContext::From(script_state),
                                    processor_handle, WTF_ARRAY_LENGTH(argv),
                                    argv, isolate)
           .ToLocal(&local_result) ||
      block.HasCaught()) {
    // process() method call method call failed for some reason or an exception
    // was thrown by the user supplied code. Disable the processor to exclude
    // it from the subsequent rendering task.
    processor->SetErrorState(AudioWorkletProcessorErrorState::kProcessError);
    return false;
  }

  // TODO(hongchan): Sanity check on length, number of channels, and object
  // type.

  // Copy |sequence<sequence<Float32Array>>| back to the original
  // |Vector<AudioBus*>|.
  for (uint32_t output_bus_index = 0; output_bus_index < output_buses->size();
       ++output_bus_index) {
    AudioBus* output_bus = (*output_buses)[output_bus_index];
    for (uint32_t channel_index = 0;
         channel_index < output_bus->NumberOfChannels(); ++channel_index) {
      memcpy(output_bus->Channel(channel_index)->MutableData(),
             js_output_raw_ptrs[output_bus_index][channel_index],
             output_bus->length() * sizeof(float));
    }
  }

  // Return the value from the user-supplied |process()| function. It is
  // used to maintain the lifetime of the node and the processor.
  return local_result->IsTrue() && !block.HasCaught();
}

AudioWorkletProcessorDefinition* AudioWorkletGlobalScope::FindDefinition(
    const String& name) {
  return processor_definition_map_.at(name);
}

unsigned AudioWorkletGlobalScope::NumberOfRegisteredDefinitions() {
  return processor_definition_map_.size();
}

std::unique_ptr<Vector<CrossThreadAudioWorkletProcessorInfo>>
AudioWorkletGlobalScope::WorkletProcessorInfoListForSynchronization() {
  auto processor_info_list =
      std::make_unique<Vector<CrossThreadAudioWorkletProcessorInfo>>();
  for (auto definition_entry : processor_definition_map_) {
    if (!definition_entry.value->IsSynchronized()) {
      definition_entry.value->MarkAsSynchronized();
      processor_info_list->emplace_back(*definition_entry.value);
    }
  }
  return processor_info_list;
}

ProcessorCreationParams* AudioWorkletGlobalScope::GetProcessorCreationParams() {
  return processor_creation_params_.get();
}

void AudioWorkletGlobalScope::SetCurrentFrame(size_t current_frame) {
  current_frame_ = current_frame;
}

void AudioWorkletGlobalScope::SetSampleRate(float sample_rate) {
  sample_rate_ = sample_rate;
}

double AudioWorkletGlobalScope::currentTime() const {
  return sample_rate_ > 0.0
        ? current_frame_ / static_cast<double>(sample_rate_)
        : 0.0;
}

void AudioWorkletGlobalScope::Trace(blink::Visitor* visitor) {
  visitor->Trace(processor_definition_map_);
  visitor->Trace(processor_instances_);
  ThreadedWorkletGlobalScope::Trace(visitor);
}

void AudioWorkletGlobalScope::TraceWrappers(
    const ScriptWrappableVisitor* visitor) const {
  for (auto definition : processor_definition_map_)
    visitor->TraceWrappers(definition.value);

  for (auto processor : processor_instances_)
    visitor->TraceWrappers(processor);

  ThreadedWorkletGlobalScope::TraceWrappers(visitor);
}

}  // namespace blink
