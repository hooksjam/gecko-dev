/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebGLContext.h"

#include <limits>

#include "GLContext.h"
#include "WebGLBuffer.h"
#include "WebGLTransformFeedback.h"
#include "WebGLVertexArray.h"

namespace mozilla {

WebGLRefPtr<WebGLBuffer>* WebGLContext::ValidateBufferSlot(GLenum target) {
  WebGLRefPtr<WebGLBuffer>* slot = nullptr;

  switch (target) {
    case LOCAL_GL_ARRAY_BUFFER:
      slot = &mBoundArrayBuffer;
      break;

    case LOCAL_GL_ELEMENT_ARRAY_BUFFER:
      slot = &(mBoundVertexArray->mElementArrayBuffer);
      break;
  }

  if (IsWebGL2()) {
    switch (target) {
      case LOCAL_GL_COPY_READ_BUFFER:
        slot = &mBoundCopyReadBuffer;
        break;

      case LOCAL_GL_COPY_WRITE_BUFFER:
        slot = &mBoundCopyWriteBuffer;
        break;

      case LOCAL_GL_PIXEL_PACK_BUFFER:
        slot = &mBoundPixelPackBuffer;
        break;

      case LOCAL_GL_PIXEL_UNPACK_BUFFER:
        slot = &mBoundPixelUnpackBuffer;
        break;

      case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER:
        slot = &mBoundTransformFeedbackBuffer;
        break;

      case LOCAL_GL_UNIFORM_BUFFER:
        slot = &mBoundUniformBuffer;
        break;
    }
  }

  if (!slot) {
    ErrorInvalidEnumInfo("target", target);
    return nullptr;
  }

  return slot;
}

WebGLBuffer* WebGLContext::ValidateBufferSelection(GLenum target) {
  const auto& slot = ValidateBufferSlot(target);
  if (!slot) return nullptr;
  const auto& buffer = *slot;

  if (!buffer) {
    ErrorInvalidOperation("Buffer for `target` is null.");
    return nullptr;
  }

  if (target == LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER) {
    if (mBoundTransformFeedback->IsActiveAndNotPaused()) {
      ErrorInvalidOperation(
          "Cannot select TRANSFORM_FEEDBACK_BUFFER when"
          " transform feedback is active and unpaused.");
      return nullptr;
    }
    const auto tfBuffers = std::vector<webgl::BufferAndIndex>{{
        {buffer},
    }};

    if (!ValidateBuffersForTf(tfBuffers)) return nullptr;
  } else {
    if (mBoundTransformFeedback && !ValidateBufferForNonTf(buffer, target))
      return nullptr;
  }

  return buffer.get();
}

IndexedBufferBinding* WebGLContext::ValidateIndexedBufferSlot(GLenum target,
                                                              GLuint index) {
  decltype(mIndexedUniformBufferBindings)* bindings;
  const char* maxIndexEnum;
  switch (target) {
    case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER:
      bindings = &(mBoundTransformFeedback->mIndexedBindings);
      maxIndexEnum = "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS";
      break;

    case LOCAL_GL_UNIFORM_BUFFER:
      bindings = &mIndexedUniformBufferBindings;
      maxIndexEnum = "MAX_UNIFORM_BUFFER_BINDINGS";
      break;

    default:
      ErrorInvalidEnumInfo("target", target);
      return nullptr;
  }

  if (index >= bindings->size()) {
    ErrorInvalidValue("`index` >= %s.", maxIndexEnum);
    return nullptr;
  }

  return &(*bindings)[index];
}

////////////////////////////////////////

void WebGLContext::BindBuffer(GLenum target, WebGLBuffer* buffer) {
  const FuncScope funcScope(*this, "bindBuffer");
  if (IsContextLost()) return;

  if (buffer && !ValidateObject("buffer", *buffer)) return;

  const auto& slot = ValidateBufferSlot(target);
  if (!slot) return;

  if (buffer && !buffer->ValidateCanBindToTarget(target)) return;

  if (!IsVirtualBufferTarget(target)) {
    gl->fBindBuffer(target, buffer ? buffer->mGLName : 0);
  }

  *slot = buffer;
  if (buffer) {
    buffer->SetContentAfterBind(target);
  }
}

////////////////////////////////////////

bool WebGLContext::ValidateIndexedBufferBinding(
    GLenum target, GLuint index,
    WebGLRefPtr<WebGLBuffer>** const out_genericBinding,
    IndexedBufferBinding** const out_indexedBinding) {
  *out_genericBinding = ValidateBufferSlot(target);
  if (!*out_genericBinding) return false;

  *out_indexedBinding = ValidateIndexedBufferSlot(target, index);
  if (!*out_indexedBinding) return false;

  if (target == LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER &&
      mBoundTransformFeedback->mIsActive) {
    ErrorInvalidOperation(
        "Cannot update indexed buffer bindings on active"
        " transform feedback objects.");
    return false;
  }

  return true;
}

void WebGLContext::BindBufferRangeImpl(GLenum target, GLuint index,
                                       WebGLBuffer* buffer, WebGLintptr offset,
                                       WebGLsizeiptr size) {
  if (buffer && !ValidateObject("buffer", *buffer)) return;

  WebGLRefPtr<WebGLBuffer>* genericBinding;
  IndexedBufferBinding* indexedBinding;
  if (!ValidateIndexedBufferBinding(target, index, &genericBinding,
                                    &indexedBinding)) {
    return;
  }

  if (buffer && !buffer->ValidateCanBindToTarget(target)) return;

  ////

  switch (target) {
    case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER:
      if (offset % 4 != 0 || size % 4 != 0) {
        ErrorInvalidValue("For %s, `offset` and `size` must be multiples of 4.",
                          "TRANSFORM_FEEDBACK_BUFFER");
        return;
      }
      break;

    case LOCAL_GL_UNIFORM_BUFFER: {
      GLuint offsetAlignment = 0;
      gl->GetUIntegerv(LOCAL_GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
                       &offsetAlignment);
      if (offset % offsetAlignment != 0) {
        ErrorInvalidValue("For %s, `offset` must be a multiple of %s.",
                          "UNIFORM_BUFFER", "UNIFORM_BUFFER_OFFSET_ALIGNMENT");
        return;
      }
    } break;
  }

  ////

  bool needsPrebind = false;
  needsPrebind |= gl->IsANGLE();
#ifdef XP_MACOSX
  needsPrebind = true;
#endif

  if (gl->WorkAroundDriverBugs() && buffer && needsPrebind) {
    // BindBufferBase/Range will fail (on some drivers) if the buffer name has
    // never been bound. (GenBuffers makes a name, but BindBuffer initializes
    // that name as a real buffer object)
    gl->fBindBuffer(target, buffer->mGLName);
  }

  if (size) {
    gl->fBindBufferRange(target, index, buffer ? buffer->mGLName : 0, offset,
                         size);
  } else {
    gl->fBindBufferBase(target, index, buffer ? buffer->mGLName : 0);
  }

  if (buffer) {
    gl->fBindBuffer(target, 0);  // Reset generic.
  }

  ////

  *genericBinding = buffer;
  indexedBinding->mBufferBinding = buffer;
  indexedBinding->mRangeStart = offset;
  indexedBinding->mRangeSize = size;

  if (buffer) {
    buffer->SetContentAfterBind(target);
  }
}

////////////////////////////////////////

void WebGLContext::BufferDataImpl(GLenum target, uint64_t dataLen,
                                  const uint8_t* data, GLenum usage) {
  const auto& buffer = ValidateBufferSelection(target);
  if (!buffer) return;

  buffer->BufferData(target, dataLen, data, usage);
}

////

void WebGLContext::BufferData(GLenum target, WebGLsizeiptr size, GLenum usage) {
  const FuncScope funcScope(*this, "bufferData");
  if (IsContextLost()) return;

  if (!ValidateNonNegative("size", size)) return;

  ////

  const auto checkedSize = CheckedInt<size_t>(size);
  if (!checkedSize.isValid())
    return ErrorOutOfMemory("size too large for platform.");

#if defined(XP_MACOSX)
  // bug 1573048
  if (gl->WorkAroundDriverBugs() && size > 1200000000) {
    return ErrorOutOfMemory("Allocations larger than 1200000000 fail on macOS.");
  }
#endif

  const UniqueBuffer zeroBuffer(calloc(checkedSize.value(), 1u));
  if (!zeroBuffer) return ErrorOutOfMemory("Failed to allocate zeros.");

  BufferDataImpl(target, uint64_t{checkedSize.value()},
                 (const uint8_t*)zeroBuffer.get(), usage);
}

void WebGLContext::BufferData(GLenum target,
                              const dom::Nullable<dom::ArrayBuffer>& maybeSrc,
                              GLenum usage) {
  const FuncScope funcScope(*this, "bufferData");
  if (IsContextLost()) return;

  if (!ValidateNonNull("src", maybeSrc)) return;
  const auto& src = maybeSrc.Value();

  src.ComputeLengthAndData();
  BufferDataImpl(target, src.LengthAllowShared(), src.DataAllowShared(), usage);
}

void WebGLContext::BufferData(GLenum target, const dom::ArrayBufferView& src,
                              GLenum usage, GLuint srcElemOffset,
                              GLuint srcElemCountOverride) {
  const FuncScope funcScope(*this, "bufferData");
  if (IsContextLost()) return;

  uint8_t* bytes;
  size_t byteLen;
  if (!ValidateArrayBufferView(src, srcElemOffset, srcElemCountOverride,
                               LOCAL_GL_INVALID_VALUE, &bytes, &byteLen)) {
    return;
  }

  BufferDataImpl(target, byteLen, bytes, usage);
}

////////////////////////////////////////

void WebGLContext::BufferSubDataImpl(GLenum target, WebGLsizeiptr dstByteOffset,
                                     uint64_t dataLen, const uint8_t* data) {
  const FuncScope funcScope(*this, "bufferSubData");

  if (!ValidateNonNegative("byteOffset", dstByteOffset)) return;

  const auto& buffer = ValidateBufferSelection(target);
  if (!buffer) return;

  buffer->BufferSubData(target, uint64_t(dstByteOffset), dataLen, data);
}

////

void WebGLContext::BufferSubData(GLenum target, WebGLsizeiptr dstByteOffset,
                                 const dom::ArrayBuffer& src) {
  const FuncScope funcScope(*this, "bufferSubData");
  if (IsContextLost()) return;

  src.ComputeLengthAndData();
  BufferSubDataImpl(target, dstByteOffset, src.LengthAllowShared(),
                    src.DataAllowShared());
}

void WebGLContext::BufferSubData(GLenum target, WebGLsizeiptr dstByteOffset,
                                 const dom::ArrayBufferView& src,
                                 GLuint srcElemOffset,
                                 GLuint srcElemCountOverride) {
  const FuncScope funcScope(*this, "bufferSubData");
  if (IsContextLost()) return;

  uint8_t* bytes;
  size_t byteLen;
  if (!ValidateArrayBufferView(src, srcElemOffset, srcElemCountOverride,
                               LOCAL_GL_INVALID_VALUE, &bytes, &byteLen)) {
    return;
  }

  BufferSubDataImpl(target, dstByteOffset, byteLen, bytes);
}

////////////////////////////////////////

already_AddRefed<WebGLBuffer> WebGLContext::CreateBuffer() {
  const FuncScope funcScope(*this, "createBuffer");
  if (IsContextLost()) return nullptr;

  GLuint buf = 0;
  gl->fGenBuffers(1, &buf);

  RefPtr<WebGLBuffer> globj = new WebGLBuffer(this, buf);
  return globj.forget();
}

void WebGLContext::DeleteBuffer(WebGLBuffer* buffer) {
  const FuncScope funcScope(*this, "deleteBuffer");
  if (!ValidateDeleteObject(buffer)) return;

  ////

  const auto fnClearIfBuffer = [&](GLenum target,
                                   WebGLRefPtr<WebGLBuffer>& bindPoint) {
    if (bindPoint == buffer) {
      bindPoint = nullptr;
    }
  };

  fnClearIfBuffer(0, mBoundArrayBuffer);
  fnClearIfBuffer(0, mBoundVertexArray->mElementArrayBuffer);

  for (auto& cur : mBoundVertexArray->mAttribs) {
    fnClearIfBuffer(0, cur.mBuf);
  }

  // WebGL binding points
  if (IsWebGL2()) {
    fnClearIfBuffer(0, mBoundCopyReadBuffer);
    fnClearIfBuffer(0, mBoundCopyWriteBuffer);
    fnClearIfBuffer(0, mBoundPixelPackBuffer);
    fnClearIfBuffer(0, mBoundPixelUnpackBuffer);
    fnClearIfBuffer(0, mBoundUniformBuffer);
    fnClearIfBuffer(LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER,
                    mBoundTransformFeedbackBuffer);

    if (!mBoundTransformFeedback->mIsActive) {
      for (auto& binding : mBoundTransformFeedback->mIndexedBindings) {
        fnClearIfBuffer(LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER,
                        binding.mBufferBinding);
      }
    }

    for (auto& binding : mIndexedUniformBufferBindings) {
      fnClearIfBuffer(0, binding.mBufferBinding);
    }
  }

  ////

  buffer->RequestDelete();
}

}  // namespace mozilla
