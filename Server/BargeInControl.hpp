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

#endif // BARIGE_IN_CONTROL_HPP

