/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MediaRecorder_h
#define MediaRecorder_h

#include "mozilla/dom/MediaRecorderBinding.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/MozPromise.h"
#include "nsIDocumentActivity.h"

// Max size for allowing queue encoded data in memory
#define MAX_ALLOW_MEMORY_BUFFER 1024000
namespace mozilla {

class AudioNodeTrack;
class DOMMediaStream;
class ErrorResult;
struct MediaRecorderOptions;
class GlobalObject;

namespace dom {

class AudioNode;
class Blob;
class Document;
class DOMException;

/**
 * Implementation of
 * https://dvcs.w3.org/hg/dap/raw-file/default/media-stream-capture/MediaRecorder.html
 * The MediaRecorder accepts a mediaStream as input source passed from UA. When
 * recorder starts, a MediaEncoder will be created and accept the mediaStream as
 * input source. Encoder will get the raw data by track data changes, encode it
 * by selected MIME Type, then store the encoded in a MutableBlobStorage object.
 * The encoded data will be extracted on every timeslice passed from Start
 * function call or by RequestData function. Thread model: When the recorder
 * starts, it creates a "Media Encoder" thread to read data from MediaEncoder
 * object and store buffer in MutableBlobStorage object. Also extract the
 * encoded data and create blobs on every timeslice passed from start function
 * or RequestData function called by UA.
 */

class MediaRecorder final : public DOMEventTargetHelper,
                            public nsIDocumentActivity {
 public:
  class Session;

  explicit MediaRecorder(nsPIDOMWindowInner* aOwnerWindow);

  static nsTArray<RefPtr<Session>> GetSessions();

  // nsWrapperCache
  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaRecorder, DOMEventTargetHelper)

  // WebIDL
  // Start recording. If timeSlice has been provided, mediaRecorder will
  // raise a dataavailable event containing the Blob of collected data on every
  // timeSlice milliseconds. If timeSlice isn't provided, UA should call the
  // RequestData to obtain the Blob data, also set the mTimeSlice to zero.
  void Start(const Optional<uint32_t>& timeSlice, ErrorResult& aResult);
  // Stop the recording activiy. Including stop the Media Encoder thread,
  // un-hook the mediaStreamListener to encoder.
  void Stop(ErrorResult& aResult);
  // Pause a recording.
  void Pause(ErrorResult& aResult);
  // Resume a paused recording.
  void Resume(ErrorResult& aResult);
  // Extract encoded data Blob from MutableBlobStorage.
  void RequestData(ErrorResult& aResult);
  // Return the The DOMMediaStream passed from UA.
  DOMMediaStream* Stream() const { return mStream; }
  // Return the current encoding MIME type selected by the MediaEncoder.
  void GetMimeType(nsString& aMimeType);
  // The current state of the MediaRecorder object.
  RecordingState State() const { return mState; }

  static bool IsTypeSupported(GlobalObject& aGlobal,
                              const nsAString& aMIMEType);
  static bool IsTypeSupported(const nsAString& aMIMEType);

  // Construct a recorder with a DOM media stream object as its source.
  static already_AddRefed<MediaRecorder> Constructor(
      const GlobalObject& aGlobal, DOMMediaStream& aStream,
      const MediaRecorderOptions& aOptions, ErrorResult& aRv);
  // Construct a recorder with a Web Audio destination node as its source.
  static already_AddRefed<MediaRecorder> Constructor(
      const GlobalObject& aGlobal, AudioNode& aAudioNode,
      uint32_t aAudioNodeOutput, const MediaRecorderOptions& aOptions,
      ErrorResult& aRv);

  /*
   * Measure the size of the buffer, and heap memory in bytes occupied by
   * mAudioEncoder and mVideoEncoder.
   */
  typedef MozPromise<size_t, size_t, true> SizeOfPromise;
  RefPtr<SizeOfPromise> SizeOfExcludingThis(
      mozilla::MallocSizeOf aMallocSizeOf);
  // EventHandler
  IMPL_EVENT_HANDLER(start)
  IMPL_EVENT_HANDLER(stop)
  IMPL_EVENT_HANDLER(dataavailable)
  IMPL_EVENT_HANDLER(pause)
  IMPL_EVENT_HANDLER(resume)
  IMPL_EVENT_HANDLER(error)

  NS_DECL_NSIDOCUMENTACTIVITY

  uint32_t AudioBitsPerSecond() const { return mAudioBitsPerSecond; }
  uint32_t VideoBitsPerSecond() const { return mVideoBitsPerSecond; }

 protected:
  virtual ~MediaRecorder();

  MediaRecorder& operator=(const MediaRecorder& x) = delete;
  // Create dataavailable event with Blob data and it runs in main thread
  nsresult CreateAndDispatchBlobEvent(Blob* aBlob);
  // Creating a simple event to notify UA simple event.
  void DispatchSimpleEvent(const nsAString& aStr);
  // Creating a error event with message.
  void NotifyError(nsresult aRv);

  MediaRecorder(const MediaRecorder& x) = delete;  // prevent bad usage
  // Remove session pointer.
  void RemoveSession(Session* aSession);
  // Create DOMExceptions capturing the JS stack for async errors. These are
  // created ahead of time rather than on demand when firing an error as the JS
  // stack of the operation that started the async behavior will not be
  // available at the time the error event is fired. Note, depending on when
  // this is called there may not be a JS stack to capture.
  void InitializeDomExceptions();
  // Runs the "Inactivate the recorder" algorithm.
  void Inactivate();
  // Stop the recorder and its internal session. This should be used by
  // sessions that are in the process of being destroyed.
  void StopForSessionDestruction();
  // DOM wrapper for source media stream. Will be null when input is audio node.
  RefPtr<DOMMediaStream> mStream;
  // Source audio node. Will be null when input is a media stream.
  RefPtr<AudioNode> mAudioNode;
  // Source audio node's output index. Will be zero when input is a media
  // stream.
  uint32_t mAudioNodeOutput = 0;

  // The current state of the MediaRecorder object.
  RecordingState mState = RecordingState::Inactive;
  // Hold the sessions reference and clean it when the DestroyRunnable for a
  // session is running.
  nsTArray<RefPtr<Session>> mSessions;

  RefPtr<Document> mDocument;

  nsString mMimeType;
  nsString mConstrainedMimeType;

  uint32_t mAudioBitsPerSecond = 0;
  uint32_t mVideoBitsPerSecond = 0;
  Maybe<uint32_t> mConstrainedBitsPerSecond;

  // DOMExceptions that are created early and possibly thrown in NotifyError.
  // Creating them early allows us to capture the JS stack for which cannot be
  // done at the time the error event is fired.
  RefPtr<DOMException> mSecurityDomException;
  RefPtr<DOMException> mUnknownDomException;

 private:
  // Register MediaRecorder into Document to listen the activity changes.
  void RegisterActivityObserver();
  void UnRegisterActivityObserver();

  bool CheckPermission(const nsString& aType);
};

}  // namespace dom
}  // namespace mozilla

#endif
