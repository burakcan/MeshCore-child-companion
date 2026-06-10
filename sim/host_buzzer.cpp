#include "host_buzzer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>

// RTTTL defaults (Nokia spec): quarter notes, octave 6, 63 bpm.
static bool s_muted = false;

void hostBuzzerToggleMute() { s_muted = !s_muted; }
bool hostBuzzerMuted() { return s_muted; }

// note letter -> semitone index used by NonBlockingRtttl: c=1..b=12, rest=0.
static int noteIndex(char c) {
  switch (c) {
    case 'c': return 1;  case 'd': return 3;  case 'e': return 5;
    case 'f': return 6;  case 'g': return 8;  case 'a': return 10;
    case 'b': return 12; default:  return 0;   // 'p' / unknown = rest
  }
}

// MIDI -> Hz (equal temperament). Matches Arduino pitches.h to the nearest Hz.
static double midiToFreq(int midi) {
  return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

struct Tone { double freq; int ms; };  // freq 0 = rest

static void parseRtttl(const char* s, std::vector<Tone>& out) {
  const char* p = strchr(s, ':');           // skip "name"
  if (!p) return;
  p++;
  int d = 4, o = 6, bpm = 63;
  const char* sec2 = strchr(p, ':');        // end of defaults section
  for (const char* q = p; sec2 && q < sec2; q++) {
    if ((q[0] == 'd' || q[0] == 'o' || q[0] == 'b') && q[1] == '=') {
      int v = atoi(q + 2);
      if (q[0] == 'd') d = v; else if (q[0] == 'o') o = v; else bpm = v;
    }
  }
  if (!sec2) return;
  const char* n = sec2 + 1;
  long wholenote = (60L * 1000L / bpm) * 4;

  while (*n) {
    while (*n == ',' || *n == ' ') n++;
    if (!*n) break;

    int dur = 0;
    while (*n >= '0' && *n <= '9') { dur = dur * 10 + (*n - '0'); n++; }
    if (dur == 0) dur = d;

    int idx = noteIndex(*n);
    if (*n >= 'a' && *n <= 'g') n++; else if (*n == 'p') n++;

    if (*n == '#') { idx++; n++; }

    long ms = wholenote / dur;
    if (*n == '.') { ms += ms / 2; n++; }

    int scale = o;
    if (*n >= '0' && *n <= '9') { scale = *n - '0'; n++; }

    double freq = 0.0;
    if (idx > 0) {
      int midi = 12 * (scale + 1) + (idx - 1);  // idx 1 = C
      freq = midiToFreq(midi);
    }
    out.push_back({ freq, (int)ms });
  }
}

#ifdef __APPLE__
static void writeLE(std::vector<uint8_t>& b, uint32_t v, int n) {
  for (int i = 0; i < n; i++) b.push_back((v >> (8 * i)) & 0xFF);
}

void hostBuzzerPlay(const char* rtttl) {
  if (s_muted || !rtttl) return;
  std::vector<Tone> tones;
  parseRtttl(rtttl, tones);
  if (tones.empty()) return;

  const int SR = 44100;
  const int16_t AMP = 7000;
  std::vector<int16_t> pcm;
  for (const Tone& t : tones) {
    int total = (int)((long)SR * t.ms / 1000);
    int sound = total * 9 / 10;   // 90% tone, 10% gap for articulation
    for (int i = 0; i < total; i++) {
      int16_t s = 0;
      if (t.freq > 0 && i < sound) {
        double period = SR / t.freq;
        s = (std::fmod(i, period) < period / 2) ? AMP : -AMP;  // square wave
      }
      pcm.push_back(s);
    }
  }

  std::vector<uint8_t> wav;
  uint32_t dataBytes = (uint32_t)(pcm.size() * 2);
  wav.insert(wav.end(), {'R','I','F','F'});
  writeLE(wav, 36 + dataBytes, 4);
  wav.insert(wav.end(), {'W','A','V','E','f','m','t',' '});
  writeLE(wav, 16, 4);            // fmt chunk size
  writeLE(wav, 1, 2);             // PCM
  writeLE(wav, 1, 2);             // mono
  writeLE(wav, SR, 4);
  writeLE(wav, SR * 2, 4);        // byte rate
  writeLE(wav, 2, 2);            // block align
  writeLE(wav, 16, 2);           // bits
  wav.insert(wav.end(), {'d','a','t','a'});
  writeLE(wav, dataBytes, 4);
  for (int16_t s : pcm) writeLE(wav, (uint16_t)s, 2);

  char path[64];
  static int seq = 0;
  snprintf(path, sizeof(path), "/tmp/meshsim_buzz_%d.wav", (seq++) & 3);
  FILE* f = fopen(path, "wb");
  if (!f) return;
  fwrite(wav.data(), 1, wav.size(), f);
  fclose(f);

  char cmd[128];
  snprintf(cmd, sizeof(cmd), "afplay '%s' >/dev/null 2>&1 &", path);
  if (system(cmd) != 0) { /* best-effort */ }
}
#else
void hostBuzzerPlay(const char* rtttl) {
  if (s_muted || !rtttl) return;
  fputc('\a', stdout);   // fallback: terminal bell
  fflush(stdout);
}
#endif
