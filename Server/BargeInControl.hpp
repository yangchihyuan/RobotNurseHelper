#ifndef BARIGE_IN_CONTROL_HPP
#define BARIGE_IN_CONTROL_HPP

#include <atomic>

// Global, thread-safe flags for voice interruption (barge-in).
// - robotSpeaking: true while the robot is actively speaking (TTS started)
// - stopRequested: true once a Whisper interruption keyword is detected while robotSpeaking==true
//
// IMPORTANT: These are global variables so ThreadWhisper and ThreadStateControl can coordinate
// without redesigning your architecture.
inline std::atomic<bool> robotSpeaking{false};
inline std::atomic<bool> stopRequested{false};

// RMS amplitude (16-bit signed PCM, so full scale is 32767) that a chunk of
// the robot's incoming mic audio must exceed, while robotSpeaking==true, to
// be treated as a deliberate patient barge-in rather than the robot
// overhearing its own TTS through room echo.
//
// RMS (root-mean-square energy) rather than raw peak, because a single
// clipped sample or electrical click can spike the peak without representing
// sustained speech; RMS is the standard loudness measure for voice-activity
// style detection and rejects that kind of transient background noise.
// Typical steady background-room-noise RMS sits well under 1000; a person
// speaking directly at the robot's mic typically pushes RMS into the low
// thousands. This value has not been calibrated against the physical Zenbo
// mic/room acoustics -- treat it as a starting point and retune on-device
// (log the computed RMS in mainwindow.cpp's readSocket3() while talking at
// normal volume vs. just letting the robot talk, and set the threshold
// between the two observed levels).
inline constexpr double kBargeInAmplitudeThreshold = 3000.0;

#endif // BARIGE_IN_CONTROL_HPP

