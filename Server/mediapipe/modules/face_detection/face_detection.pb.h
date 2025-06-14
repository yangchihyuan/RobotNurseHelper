// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: mediapipe/modules/face_detection/face_detection.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3019000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3019001 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
#include "mediapipe/calculators/tensor/inference_calculator.pb.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/gpu/gpu_origin.pb.h"
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto;
namespace mediapipe {
class FaceDetectionOptions;
struct FaceDetectionOptionsDefaultTypeInternal;
extern FaceDetectionOptionsDefaultTypeInternal _FaceDetectionOptions_default_instance_;
}  // namespace mediapipe
PROTOBUF_NAMESPACE_OPEN
template<> ::mediapipe::FaceDetectionOptions* Arena::CreateMaybeMessage<::mediapipe::FaceDetectionOptions>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace mediapipe {

// ===================================================================

class FaceDetectionOptions final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:mediapipe.FaceDetectionOptions) */ {
 public:
  inline FaceDetectionOptions() : FaceDetectionOptions(nullptr) {}
  ~FaceDetectionOptions() override;
  explicit constexpr FaceDetectionOptions(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  FaceDetectionOptions(const FaceDetectionOptions& from);
  FaceDetectionOptions(FaceDetectionOptions&& from) noexcept
    : FaceDetectionOptions() {
    *this = ::std::move(from);
  }

  inline FaceDetectionOptions& operator=(const FaceDetectionOptions& from) {
    CopyFrom(from);
    return *this;
  }
  inline FaceDetectionOptions& operator=(FaceDetectionOptions&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const FaceDetectionOptions& default_instance() {
    return *internal_default_instance();
  }
  static inline const FaceDetectionOptions* internal_default_instance() {
    return reinterpret_cast<const FaceDetectionOptions*>(
               &_FaceDetectionOptions_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(FaceDetectionOptions& a, FaceDetectionOptions& b) {
    a.Swap(&b);
  }
  inline void Swap(FaceDetectionOptions* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(FaceDetectionOptions* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  FaceDetectionOptions* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<FaceDetectionOptions>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const FaceDetectionOptions& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const FaceDetectionOptions& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to, const ::PROTOBUF_NAMESPACE_ID::Message& from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FaceDetectionOptions* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "mediapipe.FaceDetectionOptions";
  }
  protected:
  explicit FaceDetectionOptions(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kStridesFieldNumber = 24,
    kModelPathFieldNumber = 1,
    kDelegateFieldNumber = 6,
    kGpuOriginFieldNumber = 11,
    kTensorWidthFieldNumber = 21,
    kTensorHeightFieldNumber = 22,
    kNumLayersFieldNumber = 23,
    kNumBoxesFieldNumber = 31,
    kXScaleFieldNumber = 32,
    kYScaleFieldNumber = 33,
    kWScaleFieldNumber = 34,
    kHScaleFieldNumber = 35,
    kMinScoreThreshFieldNumber = 36,
    kInterpolatedScaleAspectRatioFieldNumber = 25,
  };
  // repeated int32 strides = 24;
  int strides_size() const;
  private:
  int _internal_strides_size() const;
  public:
  void clear_strides();
  private:
  int32_t _internal_strides(int index) const;
  const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
      _internal_strides() const;
  void _internal_add_strides(int32_t value);
  ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
      _internal_mutable_strides();
  public:
  int32_t strides(int index) const;
  void set_strides(int index, int32_t value);
  void add_strides(int32_t value);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
      strides() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
      mutable_strides();

  // optional string model_path = 1;
  bool has_model_path() const;
  private:
  bool _internal_has_model_path() const;
  public:
  void clear_model_path();
  const std::string& model_path() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_model_path(ArgT0&& arg0, ArgT... args);
  std::string* mutable_model_path();
  PROTOBUF_NODISCARD std::string* release_model_path();
  void set_allocated_model_path(std::string* model_path);
  private:
  const std::string& _internal_model_path() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_model_path(const std::string& value);
  std::string* _internal_mutable_model_path();
  public:

  // optional .mediapipe.InferenceCalculatorOptions.Delegate delegate = 6;
  bool has_delegate() const;
  private:
  bool _internal_has_delegate() const;
  public:
  void clear_delegate();
  const ::mediapipe::InferenceCalculatorOptions_Delegate& delegate() const;
  PROTOBUF_NODISCARD ::mediapipe::InferenceCalculatorOptions_Delegate* release_delegate();
  ::mediapipe::InferenceCalculatorOptions_Delegate* mutable_delegate();
  void set_allocated_delegate(::mediapipe::InferenceCalculatorOptions_Delegate* delegate);
  private:
  const ::mediapipe::InferenceCalculatorOptions_Delegate& _internal_delegate() const;
  ::mediapipe::InferenceCalculatorOptions_Delegate* _internal_mutable_delegate();
  public:
  void unsafe_arena_set_allocated_delegate(
      ::mediapipe::InferenceCalculatorOptions_Delegate* delegate);
  ::mediapipe::InferenceCalculatorOptions_Delegate* unsafe_arena_release_delegate();

  // optional .mediapipe.GpuOrigin.Mode gpu_origin = 11;
  bool has_gpu_origin() const;
  private:
  bool _internal_has_gpu_origin() const;
  public:
  void clear_gpu_origin();
  ::mediapipe::GpuOrigin_Mode gpu_origin() const;
  void set_gpu_origin(::mediapipe::GpuOrigin_Mode value);
  private:
  ::mediapipe::GpuOrigin_Mode _internal_gpu_origin() const;
  void _internal_set_gpu_origin(::mediapipe::GpuOrigin_Mode value);
  public:

  // optional int32 tensor_width = 21;
  bool has_tensor_width() const;
  private:
  bool _internal_has_tensor_width() const;
  public:
  void clear_tensor_width();
  int32_t tensor_width() const;
  void set_tensor_width(int32_t value);
  private:
  int32_t _internal_tensor_width() const;
  void _internal_set_tensor_width(int32_t value);
  public:

  // optional int32 tensor_height = 22;
  bool has_tensor_height() const;
  private:
  bool _internal_has_tensor_height() const;
  public:
  void clear_tensor_height();
  int32_t tensor_height() const;
  void set_tensor_height(int32_t value);
  private:
  int32_t _internal_tensor_height() const;
  void _internal_set_tensor_height(int32_t value);
  public:

  // optional int32 num_layers = 23;
  bool has_num_layers() const;
  private:
  bool _internal_has_num_layers() const;
  public:
  void clear_num_layers();
  int32_t num_layers() const;
  void set_num_layers(int32_t value);
  private:
  int32_t _internal_num_layers() const;
  void _internal_set_num_layers(int32_t value);
  public:

  // optional int32 num_boxes = 31;
  bool has_num_boxes() const;
  private:
  bool _internal_has_num_boxes() const;
  public:
  void clear_num_boxes();
  int32_t num_boxes() const;
  void set_num_boxes(int32_t value);
  private:
  int32_t _internal_num_boxes() const;
  void _internal_set_num_boxes(int32_t value);
  public:

  // optional float x_scale = 32 [default = 0];
  bool has_x_scale() const;
  private:
  bool _internal_has_x_scale() const;
  public:
  void clear_x_scale();
  float x_scale() const;
  void set_x_scale(float value);
  private:
  float _internal_x_scale() const;
  void _internal_set_x_scale(float value);
  public:

  // optional float y_scale = 33 [default = 0];
  bool has_y_scale() const;
  private:
  bool _internal_has_y_scale() const;
  public:
  void clear_y_scale();
  float y_scale() const;
  void set_y_scale(float value);
  private:
  float _internal_y_scale() const;
  void _internal_set_y_scale(float value);
  public:

  // optional float w_scale = 34 [default = 0];
  bool has_w_scale() const;
  private:
  bool _internal_has_w_scale() const;
  public:
  void clear_w_scale();
  float w_scale() const;
  void set_w_scale(float value);
  private:
  float _internal_w_scale() const;
  void _internal_set_w_scale(float value);
  public:

  // optional float h_scale = 35 [default = 0];
  bool has_h_scale() const;
  private:
  bool _internal_has_h_scale() const;
  public:
  void clear_h_scale();
  float h_scale() const;
  void set_h_scale(float value);
  private:
  float _internal_h_scale() const;
  void _internal_set_h_scale(float value);
  public:

  // optional float min_score_thresh = 36;
  bool has_min_score_thresh() const;
  private:
  bool _internal_has_min_score_thresh() const;
  public:
  void clear_min_score_thresh();
  float min_score_thresh() const;
  void set_min_score_thresh(float value);
  private:
  float _internal_min_score_thresh() const;
  void _internal_set_min_score_thresh(float value);
  public:

  // optional float interpolated_scale_aspect_ratio = 25 [default = 1];
  bool has_interpolated_scale_aspect_ratio() const;
  private:
  bool _internal_has_interpolated_scale_aspect_ratio() const;
  public:
  void clear_interpolated_scale_aspect_ratio();
  float interpolated_scale_aspect_ratio() const;
  void set_interpolated_scale_aspect_ratio(float value);
  private:
  float _internal_interpolated_scale_aspect_ratio() const;
  void _internal_set_interpolated_scale_aspect_ratio(float value);
  public:

  static const int kExtFieldNumber = 374290926;
  static ::PROTOBUF_NAMESPACE_ID::internal::ExtensionIdentifier< ::mediapipe::CalculatorOptions,
      ::PROTOBUF_NAMESPACE_ID::internal::MessageTypeTraits< ::mediapipe::FaceDetectionOptions >, 11, false >
    ext;
  // @@protoc_insertion_point(class_scope:mediapipe.FaceDetectionOptions)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t > strides_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr model_path_;
  ::mediapipe::InferenceCalculatorOptions_Delegate* delegate_;
  int gpu_origin_;
  int32_t tensor_width_;
  int32_t tensor_height_;
  int32_t num_layers_;
  int32_t num_boxes_;
  float x_scale_;
  float y_scale_;
  float w_scale_;
  float h_scale_;
  float min_score_thresh_;
  float interpolated_scale_aspect_ratio_;
  friend struct ::TableStruct_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// FaceDetectionOptions

// optional string model_path = 1;
inline bool FaceDetectionOptions::_internal_has_model_path() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_model_path() const {
  return _internal_has_model_path();
}
inline void FaceDetectionOptions::clear_model_path() {
  model_path_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& FaceDetectionOptions::model_path() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.model_path)
  return _internal_model_path();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void FaceDetectionOptions::set_model_path(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000001u;
 model_path_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.model_path)
}
inline std::string* FaceDetectionOptions::mutable_model_path() {
  std::string* _s = _internal_mutable_model_path();
  // @@protoc_insertion_point(field_mutable:mediapipe.FaceDetectionOptions.model_path)
  return _s;
}
inline const std::string& FaceDetectionOptions::_internal_model_path() const {
  return model_path_.Get();
}
inline void FaceDetectionOptions::_internal_set_model_path(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  model_path_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArenaForAllocation());
}
inline std::string* FaceDetectionOptions::_internal_mutable_model_path() {
  _has_bits_[0] |= 0x00000001u;
  return model_path_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArenaForAllocation());
}
inline std::string* FaceDetectionOptions::release_model_path() {
  // @@protoc_insertion_point(field_release:mediapipe.FaceDetectionOptions.model_path)
  if (!_internal_has_model_path()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  auto* p = model_path_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (model_path_.IsDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited())) {
    model_path_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void FaceDetectionOptions::set_allocated_model_path(std::string* model_path) {
  if (model_path != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  model_path_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), model_path,
      GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (model_path_.IsDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited())) {
    model_path_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:mediapipe.FaceDetectionOptions.model_path)
}

// optional .mediapipe.GpuOrigin.Mode gpu_origin = 11;
inline bool FaceDetectionOptions::_internal_has_gpu_origin() const {
  bool value = (_has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_gpu_origin() const {
  return _internal_has_gpu_origin();
}
inline void FaceDetectionOptions::clear_gpu_origin() {
  gpu_origin_ = 0;
  _has_bits_[0] &= ~0x00000004u;
}
inline ::mediapipe::GpuOrigin_Mode FaceDetectionOptions::_internal_gpu_origin() const {
  return static_cast< ::mediapipe::GpuOrigin_Mode >(gpu_origin_);
}
inline ::mediapipe::GpuOrigin_Mode FaceDetectionOptions::gpu_origin() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.gpu_origin)
  return _internal_gpu_origin();
}
inline void FaceDetectionOptions::_internal_set_gpu_origin(::mediapipe::GpuOrigin_Mode value) {
  assert(::mediapipe::GpuOrigin_Mode_IsValid(value));
  _has_bits_[0] |= 0x00000004u;
  gpu_origin_ = value;
}
inline void FaceDetectionOptions::set_gpu_origin(::mediapipe::GpuOrigin_Mode value) {
  _internal_set_gpu_origin(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.gpu_origin)
}

// optional int32 tensor_width = 21;
inline bool FaceDetectionOptions::_internal_has_tensor_width() const {
  bool value = (_has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_tensor_width() const {
  return _internal_has_tensor_width();
}
inline void FaceDetectionOptions::clear_tensor_width() {
  tensor_width_ = 0;
  _has_bits_[0] &= ~0x00000008u;
}
inline int32_t FaceDetectionOptions::_internal_tensor_width() const {
  return tensor_width_;
}
inline int32_t FaceDetectionOptions::tensor_width() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.tensor_width)
  return _internal_tensor_width();
}
inline void FaceDetectionOptions::_internal_set_tensor_width(int32_t value) {
  _has_bits_[0] |= 0x00000008u;
  tensor_width_ = value;
}
inline void FaceDetectionOptions::set_tensor_width(int32_t value) {
  _internal_set_tensor_width(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.tensor_width)
}

// optional int32 tensor_height = 22;
inline bool FaceDetectionOptions::_internal_has_tensor_height() const {
  bool value = (_has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_tensor_height() const {
  return _internal_has_tensor_height();
}
inline void FaceDetectionOptions::clear_tensor_height() {
  tensor_height_ = 0;
  _has_bits_[0] &= ~0x00000010u;
}
inline int32_t FaceDetectionOptions::_internal_tensor_height() const {
  return tensor_height_;
}
inline int32_t FaceDetectionOptions::tensor_height() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.tensor_height)
  return _internal_tensor_height();
}
inline void FaceDetectionOptions::_internal_set_tensor_height(int32_t value) {
  _has_bits_[0] |= 0x00000010u;
  tensor_height_ = value;
}
inline void FaceDetectionOptions::set_tensor_height(int32_t value) {
  _internal_set_tensor_height(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.tensor_height)
}

// optional int32 num_layers = 23;
inline bool FaceDetectionOptions::_internal_has_num_layers() const {
  bool value = (_has_bits_[0] & 0x00000020u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_num_layers() const {
  return _internal_has_num_layers();
}
inline void FaceDetectionOptions::clear_num_layers() {
  num_layers_ = 0;
  _has_bits_[0] &= ~0x00000020u;
}
inline int32_t FaceDetectionOptions::_internal_num_layers() const {
  return num_layers_;
}
inline int32_t FaceDetectionOptions::num_layers() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.num_layers)
  return _internal_num_layers();
}
inline void FaceDetectionOptions::_internal_set_num_layers(int32_t value) {
  _has_bits_[0] |= 0x00000020u;
  num_layers_ = value;
}
inline void FaceDetectionOptions::set_num_layers(int32_t value) {
  _internal_set_num_layers(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.num_layers)
}

// repeated int32 strides = 24;
inline int FaceDetectionOptions::_internal_strides_size() const {
  return strides_.size();
}
inline int FaceDetectionOptions::strides_size() const {
  return _internal_strides_size();
}
inline void FaceDetectionOptions::clear_strides() {
  strides_.Clear();
}
inline int32_t FaceDetectionOptions::_internal_strides(int index) const {
  return strides_.Get(index);
}
inline int32_t FaceDetectionOptions::strides(int index) const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.strides)
  return _internal_strides(index);
}
inline void FaceDetectionOptions::set_strides(int index, int32_t value) {
  strides_.Set(index, value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.strides)
}
inline void FaceDetectionOptions::_internal_add_strides(int32_t value) {
  strides_.Add(value);
}
inline void FaceDetectionOptions::add_strides(int32_t value) {
  _internal_add_strides(value);
  // @@protoc_insertion_point(field_add:mediapipe.FaceDetectionOptions.strides)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
FaceDetectionOptions::_internal_strides() const {
  return strides_;
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >&
FaceDetectionOptions::strides() const {
  // @@protoc_insertion_point(field_list:mediapipe.FaceDetectionOptions.strides)
  return _internal_strides();
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
FaceDetectionOptions::_internal_mutable_strides() {
  return &strides_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedField< int32_t >*
FaceDetectionOptions::mutable_strides() {
  // @@protoc_insertion_point(field_mutable_list:mediapipe.FaceDetectionOptions.strides)
  return _internal_mutable_strides();
}

// optional float interpolated_scale_aspect_ratio = 25 [default = 1];
inline bool FaceDetectionOptions::_internal_has_interpolated_scale_aspect_ratio() const {
  bool value = (_has_bits_[0] & 0x00001000u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_interpolated_scale_aspect_ratio() const {
  return _internal_has_interpolated_scale_aspect_ratio();
}
inline void FaceDetectionOptions::clear_interpolated_scale_aspect_ratio() {
  interpolated_scale_aspect_ratio_ = 1;
  _has_bits_[0] &= ~0x00001000u;
}
inline float FaceDetectionOptions::_internal_interpolated_scale_aspect_ratio() const {
  return interpolated_scale_aspect_ratio_;
}
inline float FaceDetectionOptions::interpolated_scale_aspect_ratio() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.interpolated_scale_aspect_ratio)
  return _internal_interpolated_scale_aspect_ratio();
}
inline void FaceDetectionOptions::_internal_set_interpolated_scale_aspect_ratio(float value) {
  _has_bits_[0] |= 0x00001000u;
  interpolated_scale_aspect_ratio_ = value;
}
inline void FaceDetectionOptions::set_interpolated_scale_aspect_ratio(float value) {
  _internal_set_interpolated_scale_aspect_ratio(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.interpolated_scale_aspect_ratio)
}

// optional int32 num_boxes = 31;
inline bool FaceDetectionOptions::_internal_has_num_boxes() const {
  bool value = (_has_bits_[0] & 0x00000040u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_num_boxes() const {
  return _internal_has_num_boxes();
}
inline void FaceDetectionOptions::clear_num_boxes() {
  num_boxes_ = 0;
  _has_bits_[0] &= ~0x00000040u;
}
inline int32_t FaceDetectionOptions::_internal_num_boxes() const {
  return num_boxes_;
}
inline int32_t FaceDetectionOptions::num_boxes() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.num_boxes)
  return _internal_num_boxes();
}
inline void FaceDetectionOptions::_internal_set_num_boxes(int32_t value) {
  _has_bits_[0] |= 0x00000040u;
  num_boxes_ = value;
}
inline void FaceDetectionOptions::set_num_boxes(int32_t value) {
  _internal_set_num_boxes(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.num_boxes)
}

// optional float x_scale = 32 [default = 0];
inline bool FaceDetectionOptions::_internal_has_x_scale() const {
  bool value = (_has_bits_[0] & 0x00000080u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_x_scale() const {
  return _internal_has_x_scale();
}
inline void FaceDetectionOptions::clear_x_scale() {
  x_scale_ = 0;
  _has_bits_[0] &= ~0x00000080u;
}
inline float FaceDetectionOptions::_internal_x_scale() const {
  return x_scale_;
}
inline float FaceDetectionOptions::x_scale() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.x_scale)
  return _internal_x_scale();
}
inline void FaceDetectionOptions::_internal_set_x_scale(float value) {
  _has_bits_[0] |= 0x00000080u;
  x_scale_ = value;
}
inline void FaceDetectionOptions::set_x_scale(float value) {
  _internal_set_x_scale(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.x_scale)
}

// optional float y_scale = 33 [default = 0];
inline bool FaceDetectionOptions::_internal_has_y_scale() const {
  bool value = (_has_bits_[0] & 0x00000100u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_y_scale() const {
  return _internal_has_y_scale();
}
inline void FaceDetectionOptions::clear_y_scale() {
  y_scale_ = 0;
  _has_bits_[0] &= ~0x00000100u;
}
inline float FaceDetectionOptions::_internal_y_scale() const {
  return y_scale_;
}
inline float FaceDetectionOptions::y_scale() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.y_scale)
  return _internal_y_scale();
}
inline void FaceDetectionOptions::_internal_set_y_scale(float value) {
  _has_bits_[0] |= 0x00000100u;
  y_scale_ = value;
}
inline void FaceDetectionOptions::set_y_scale(float value) {
  _internal_set_y_scale(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.y_scale)
}

// optional float w_scale = 34 [default = 0];
inline bool FaceDetectionOptions::_internal_has_w_scale() const {
  bool value = (_has_bits_[0] & 0x00000200u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_w_scale() const {
  return _internal_has_w_scale();
}
inline void FaceDetectionOptions::clear_w_scale() {
  w_scale_ = 0;
  _has_bits_[0] &= ~0x00000200u;
}
inline float FaceDetectionOptions::_internal_w_scale() const {
  return w_scale_;
}
inline float FaceDetectionOptions::w_scale() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.w_scale)
  return _internal_w_scale();
}
inline void FaceDetectionOptions::_internal_set_w_scale(float value) {
  _has_bits_[0] |= 0x00000200u;
  w_scale_ = value;
}
inline void FaceDetectionOptions::set_w_scale(float value) {
  _internal_set_w_scale(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.w_scale)
}

// optional float h_scale = 35 [default = 0];
inline bool FaceDetectionOptions::_internal_has_h_scale() const {
  bool value = (_has_bits_[0] & 0x00000400u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_h_scale() const {
  return _internal_has_h_scale();
}
inline void FaceDetectionOptions::clear_h_scale() {
  h_scale_ = 0;
  _has_bits_[0] &= ~0x00000400u;
}
inline float FaceDetectionOptions::_internal_h_scale() const {
  return h_scale_;
}
inline float FaceDetectionOptions::h_scale() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.h_scale)
  return _internal_h_scale();
}
inline void FaceDetectionOptions::_internal_set_h_scale(float value) {
  _has_bits_[0] |= 0x00000400u;
  h_scale_ = value;
}
inline void FaceDetectionOptions::set_h_scale(float value) {
  _internal_set_h_scale(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.h_scale)
}

// optional float min_score_thresh = 36;
inline bool FaceDetectionOptions::_internal_has_min_score_thresh() const {
  bool value = (_has_bits_[0] & 0x00000800u) != 0;
  return value;
}
inline bool FaceDetectionOptions::has_min_score_thresh() const {
  return _internal_has_min_score_thresh();
}
inline void FaceDetectionOptions::clear_min_score_thresh() {
  min_score_thresh_ = 0;
  _has_bits_[0] &= ~0x00000800u;
}
inline float FaceDetectionOptions::_internal_min_score_thresh() const {
  return min_score_thresh_;
}
inline float FaceDetectionOptions::min_score_thresh() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.min_score_thresh)
  return _internal_min_score_thresh();
}
inline void FaceDetectionOptions::_internal_set_min_score_thresh(float value) {
  _has_bits_[0] |= 0x00000800u;
  min_score_thresh_ = value;
}
inline void FaceDetectionOptions::set_min_score_thresh(float value) {
  _internal_set_min_score_thresh(value);
  // @@protoc_insertion_point(field_set:mediapipe.FaceDetectionOptions.min_score_thresh)
}

// optional .mediapipe.InferenceCalculatorOptions.Delegate delegate = 6;
inline bool FaceDetectionOptions::_internal_has_delegate() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  PROTOBUF_ASSUME(!value || delegate_ != nullptr);
  return value;
}
inline bool FaceDetectionOptions::has_delegate() const {
  return _internal_has_delegate();
}
inline const ::mediapipe::InferenceCalculatorOptions_Delegate& FaceDetectionOptions::_internal_delegate() const {
  const ::mediapipe::InferenceCalculatorOptions_Delegate* p = delegate_;
  return p != nullptr ? *p : reinterpret_cast<const ::mediapipe::InferenceCalculatorOptions_Delegate&>(
      ::mediapipe::_InferenceCalculatorOptions_Delegate_default_instance_);
}
inline const ::mediapipe::InferenceCalculatorOptions_Delegate& FaceDetectionOptions::delegate() const {
  // @@protoc_insertion_point(field_get:mediapipe.FaceDetectionOptions.delegate)
  return _internal_delegate();
}
inline void FaceDetectionOptions::unsafe_arena_set_allocated_delegate(
    ::mediapipe::InferenceCalculatorOptions_Delegate* delegate) {
  if (GetArenaForAllocation() == nullptr) {
    delete reinterpret_cast<::PROTOBUF_NAMESPACE_ID::MessageLite*>(delegate_);
  }
  delegate_ = delegate;
  if (delegate) {
    _has_bits_[0] |= 0x00000002u;
  } else {
    _has_bits_[0] &= ~0x00000002u;
  }
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:mediapipe.FaceDetectionOptions.delegate)
}
inline ::mediapipe::InferenceCalculatorOptions_Delegate* FaceDetectionOptions::release_delegate() {
  _has_bits_[0] &= ~0x00000002u;
  ::mediapipe::InferenceCalculatorOptions_Delegate* temp = delegate_;
  delegate_ = nullptr;
#ifdef PROTOBUF_FORCE_COPY_IN_RELEASE
  auto* old =  reinterpret_cast<::PROTOBUF_NAMESPACE_ID::MessageLite*>(temp);
  temp = ::PROTOBUF_NAMESPACE_ID::internal::DuplicateIfNonNull(temp);
  if (GetArenaForAllocation() == nullptr) { delete old; }
#else  // PROTOBUF_FORCE_COPY_IN_RELEASE
  if (GetArenaForAllocation() != nullptr) {
    temp = ::PROTOBUF_NAMESPACE_ID::internal::DuplicateIfNonNull(temp);
  }
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
  return temp;
}
inline ::mediapipe::InferenceCalculatorOptions_Delegate* FaceDetectionOptions::unsafe_arena_release_delegate() {
  // @@protoc_insertion_point(field_release:mediapipe.FaceDetectionOptions.delegate)
  _has_bits_[0] &= ~0x00000002u;
  ::mediapipe::InferenceCalculatorOptions_Delegate* temp = delegate_;
  delegate_ = nullptr;
  return temp;
}
inline ::mediapipe::InferenceCalculatorOptions_Delegate* FaceDetectionOptions::_internal_mutable_delegate() {
  _has_bits_[0] |= 0x00000002u;
  if (delegate_ == nullptr) {
    auto* p = CreateMaybeMessage<::mediapipe::InferenceCalculatorOptions_Delegate>(GetArenaForAllocation());
    delegate_ = p;
  }
  return delegate_;
}
inline ::mediapipe::InferenceCalculatorOptions_Delegate* FaceDetectionOptions::mutable_delegate() {
  ::mediapipe::InferenceCalculatorOptions_Delegate* _msg = _internal_mutable_delegate();
  // @@protoc_insertion_point(field_mutable:mediapipe.FaceDetectionOptions.delegate)
  return _msg;
}
inline void FaceDetectionOptions::set_allocated_delegate(::mediapipe::InferenceCalculatorOptions_Delegate* delegate) {
  ::PROTOBUF_NAMESPACE_ID::Arena* message_arena = GetArenaForAllocation();
  if (message_arena == nullptr) {
    delete reinterpret_cast< ::PROTOBUF_NAMESPACE_ID::MessageLite*>(delegate_);
  }
  if (delegate) {
    ::PROTOBUF_NAMESPACE_ID::Arena* submessage_arena =
        ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper<
            ::PROTOBUF_NAMESPACE_ID::MessageLite>::GetOwningArena(
                reinterpret_cast<::PROTOBUF_NAMESPACE_ID::MessageLite*>(delegate));
    if (message_arena != submessage_arena) {
      delegate = ::PROTOBUF_NAMESPACE_ID::internal::GetOwnedMessage(
          message_arena, delegate, submessage_arena);
    }
    _has_bits_[0] |= 0x00000002u;
  } else {
    _has_bits_[0] &= ~0x00000002u;
  }
  delegate_ = delegate;
  // @@protoc_insertion_point(field_set_allocated:mediapipe.FaceDetectionOptions.delegate)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace mediapipe

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_mediapipe_2fmodules_2fface_5fdetection_2fface_5fdetection_2eproto
