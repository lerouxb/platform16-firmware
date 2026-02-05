#include <cmath>
#ifndef PLATFORM_QUANTIZE_H
#define PLATFORM_QUANTIZE_H

#define SCALE_UNQUANTIZED 0
#define SCALE_CHROMATIC 1
#define SCALE_MAJOR 2
#define SCALE_NATURAL_MINOR 3
#define SCALE_HARMONIC_MINOR 4
#define SCALE_MELODIC_MINOR 5
#define SCALE_PENTATONIC_MAJOR 6
#define SCALE_PENTATONIC_MINOR 7
// maybe the blues scale, whole tone scale or diminished scale, although those
// are rarely used.


namespace platform {

// chords as semitone offsets from the root note
int majorChordOffsets[] = {0, 4, 7};
int minorChordOffsets[] = {0, 3, 7};
int diminishedChordOffsets[] = {0, 3, 6};
int augmentedChordOffsets[] = {0, 4, 8};
// other chords are more situational, probably not as easily fit into chord
// scales in a robotic way.


/*
scales
0 unquantized
1 chromatic
2 major
3 natural minor
4 harmonic minor
5 melodic minor
6 pentatonic major
7 pentatonic minor
*/
int chromaticOffsets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
int majorOffsets[] = {0, 2, 4, 5, 7, 9, 11};
int naturalMinorOffsets[] = {0, 2, 3, 5, 7, 8, 10};
int harmonicMinorOffsets[] = {0, 2, 3, 5, 7, 8, 11};
int melodicMinorOffsets[] = {0, 2, 3, 5, 7, 9, 11};
// pentatonic major skips 5 and 11 from major scale
int pentatonicMajorOffsets[] = {0, 2, 4, 7, 9};
// pentatonic minor skips 2 and 8 from the natural minor scale
int pentatonicMinorOffsets[] = {0, 3, 5, 7, 10};

// chord scales as major (0) vs minor (1) vs diminished (2) vs augmented (3)
// chord of the note at that point in the scale
int majorChordScale[] = {0, 1, 1, 0, 0, 1, 2};
int naturalMinorChordScale[] = {1, 2, 0, 1, 1, 0, 0};
int harmonicMinorChordScale[] = {1, 2, 3, 1, 0, 0, 2};
int melodicMinorChordScale[] = {1, 1, 3, 0, 0, 2, 2};

// 76+12 = 88
float notes[] = {
  /*A0*/ 27.5,
  /*A♯0/B♭0*/ 29.13524,
  /*B0*/ 30.86771,
  /*C1*/ 32.7032,
  /*C♯1/D♭1*/ 34.64783,
  /*D1*/ 36.7081,
  /*D♯1/E♭1*/ 38.89087,
  /*E1*/ 41.20344,
  /*F1*/ 43.65353,
  /*F♯1/G♭1*/ 46.2493,
  /*G1*/ 48.99943,
  /*G♯1/A♭1*/ 51.91309,
  /*A1*/ 55.f,
  /*A♯1/B♭1*/ 58.27047,
  /*B1*/ 61.73541,
  /*C2*/ 65.40639,
  /*C♯2/D♭2*/ 69.29566,
  /*D2*/ 73.41619,
  /*D♯2/E♭2*/ 77.78175,
  /*E2*/ 82.40689,
  /*F2*/ 87.30706,
  /*F♯2/G♭2*/ 92.49861,
  /*G2*/ 97.99886,
  /*G♯2/A♭2*/ 103.8262,
  /*A2*/ 110.f,
  /*A♯2/B♭2*/ 116.5409,
  /*B2*/ 123.4708,
  /*C3*/ 130.8128,
  /*C♯3/D♭3*/ 138.5913,
  /*D3*/ 146.8324,
  /*D♯3/E♭3*/ 155.5635,
  /*E3*/ 164.8138,
  /*F3*/ 174.6141,
  /*F♯3/G♭3*/ 184.9972,
  /*G3*/ 195.9977,
  /*G♯3/A♭3*/ 207.6523,
  /*A3*/ 220.f,
  /*A♯3/B♭3*/ 233.0819,
  /*B3*/ 246.9417,
  /*C4*/ 261.6256,
  /*C♯4/D♭4*/ 277.1826,
  /*D4*/ 293.6648,
  /*D♯4/E♭4*/ 311.127,
  /*E4*/ 329.6276,
  /*F4*/ 349.2282,
  /*F♯4/G♭4*/ 369.9944,
  /*G4*/ 391.9954,
  /*G♯4/A♭4*/ 415.3047,
  /*A4*/ 440.f,
  /*A♯4/B♭4*/ 466.1638,
  /*B4*/ 493.8833,
  /*C5*/ 523.2511,
  /*C♯5/D♭5*/ 554.3653,
  /*D5*/ 587.3295,
  /*D♯5/E♭5*/ 622.254,
  /*E5*/ 659.2551,
  /*F5*/ 698.4565,
  /*F♯5/G♭5*/ 739.9888,
  /*G5*/ 783.9909,
  /*G♯5/A♭5*/ 830.6094,
  /*A5*/ 880.f,
  /*A♯5/B♭5*/ 932.3275,
  /*B5*/ 987.7666,
  /*C6*/ 1046.502,
  /*C♯6/D♭6*/ 1108.731,
  /*D6*/ 1174.659,
  /*D♯6/E♭6*/ 1244.508,
  /*E6*/ 1318.51,
  /*F6*/ 1396.913,
  /*F♯6/G♭6*/ 1479.978,
  /*G6*/ 1567.982,
  /*G♯6/A♭6*/ 1661.219,
  /*A6*/ 1760.f,
  /*A♯6/B♭6*/ 1864.655,
  /*B6*/ 1975.533,
  /*C7*/ 2093.005,
  /*C♯7/D♭7*/ 2217.461,
  /*D7*/ 2349.318,
  /*D♯7/E♭7*/ 2489.016,
  /*E7*/ 2637.02,
  /*F7*/ 2793.826,
  /*F♯7/G♭7*/ 2959.955,
  /*G7*/ 3135.963,
  /*G♯7/A♭7*/ 3322.438,
  /*A7*/ 3520.f,
  /*A♯7/B♭7*/ 3729.31,
  /*B7*/ 3951.066,
  /*C8*/ 4186.009};


float getFrequencyForNote(int scale, float note) {
  int noteIndex = (int)note;
  float noteFraction = note - noteIndex;
  float value = notes[noteIndex];

  if (!scale) {
    // TODO: this should probably be some kind of curve rather than linear
    value = value + (notes[noteIndex + 1] - notes[noteIndex]) * noteFraction;
  }

  return value;
}

int getScaleNotesForScale(int scale, int** scaleNotes) {
  switch (scale) {
    case SCALE_MAJOR:
      *scaleNotes = majorOffsets;
      return sizeof(majorOffsets) / sizeof(int);
    case SCALE_NATURAL_MINOR:
      *scaleNotes = naturalMinorOffsets;
      return sizeof(naturalMinorOffsets) / sizeof(int);
    case SCALE_HARMONIC_MINOR:
      *scaleNotes = harmonicMinorOffsets;
      return sizeof(harmonicMinorOffsets) / sizeof(int);
    case SCALE_MELODIC_MINOR:
      *scaleNotes = melodicMinorOffsets;
      return sizeof(melodicMinorOffsets) / sizeof(int);
    case SCALE_PENTATONIC_MAJOR:
      *scaleNotes = pentatonicMajorOffsets;
      return sizeof(pentatonicMajorOffsets) / sizeof(int);
    case SCALE_PENTATONIC_MINOR:
      *scaleNotes = pentatonicMinorOffsets;
      return sizeof(pentatonicMinorOffsets) / sizeof(int);
    default:
      *scaleNotes = chromaticOffsets;
      return sizeof(chromaticOffsets) / sizeof(int);
  }
}

int getScaleOffsetForNote(float amount, int numScaleNotes) {
  /*
  amount is -1 to 1. We're multiplying numScaleNotes by 0 to 1, rounding to the closest integer.
  assuming major scale: scaleOffset 0 means it returns the root note, 7 means it
  returns one octave up, 14 means two octaves up, etc.
  */
  int scaleOffset = (int)(roundf(fabs(amount) * numScaleNotes * 2.f));
  return scaleOffset;
}

float getSemitoneOffsetForNote(int scale, float amount) {
  if (scale) {
    int* scaleNotes;
    int numScaleNotes = getScaleNotesForScale(scale, &scaleNotes);
    int scaleOffset = getScaleOffsetForNote(amount, numScaleNotes);

    int semitoneOffset = 0;

    while (scaleOffset >= numScaleNotes) {
      scaleOffset -= numScaleNotes;
      semitoneOffset += 12;
    }
    //{0, 2, 3, 5, 7, 9, 11}
    semitoneOffset += scaleNotes[scaleOffset];

    if (amount < 0.f) {
      return -semitoneOffset;
    }
    return semitoneOffset;
  } else {
    return amount * 24.f;
  }
}

int getChordScaleDegreeForNote(int scale, float amount) {
  if (scale) {
    int* scaleNotes;
    int numScaleNotes = getScaleNotesForScale(scale, &scaleNotes);
    int scaleOffset = getScaleOffsetForNote(amount, numScaleNotes);

    /*
    so let's say it is the c major scale. There are 7 notes in the scale. If we
    go up 7 positions then we're back on c, just one octave higher.
    Going up one step one degree takes you to d. Going down one step takes you to b.

    We don't care what octave you land on, we're only interested in which
    position in the scale it is so we can map the chord type.
    */
    while (scaleOffset >= numScaleNotes) {
      scaleOffset -= numScaleNotes;
    }
    if (amount < 0.f) {
      scaleOffset = numScaleNotes - scaleOffset;
    }

    for (int i = 0; i < numScaleNotes; i++) {
      if (scaleOffset == scaleNotes[i]) {
        return i;
      }
    }
    return 0;

  } else {
    // TODO: this only makes sense for scales
    return 0;
  }
}

int getChordTypeForNote(int scale, int degree) {
  switch (scale) {
    case SCALE_MAJOR:
      return majorChordScale[degree];
    case SCALE_NATURAL_MINOR:
      return naturalMinorChordScale[degree];
    case SCALE_HARMONIC_MINOR:
      return harmonicMinorChordScale[degree];
    case SCALE_MELODIC_MINOR:
      return melodicMinorChordScale[degree];
    default:
      return majorChordScale[degree];
  }
}

float addSemitonesToFrequency(float frequency, float semitones) {
  return frequency * powf(2.f, semitones / 12.f);
}

int* getChordOffsetsForType(int chordType) {
  switch (chordType) {
    case 0:
      return majorChordOffsets;
    case 1:
      return minorChordOffsets;
    case 2:
      return diminishedChordOffsets;
    case 3:
      return augmentedChordOffsets;
    default:
      return majorChordOffsets;
  }
}

struct NoteQuantizer {
  NoteQuantizer() : scale(0), lastPitchValue(0.f), lastPitchAmount(0.f) {}
  ~NoteQuantizer() {}

  /*
  This is useful so you can change the scale, base pitch and amount only when
  notes get triggered when trying to stick to a scale which should guarantee no
  oscillation between two notes.
  */
  void setScaleAndPitchAmount(int scaleIn, float pitchValueIn, float pitchAmountIn) {
    scale = scaleIn;
    lastPitchValue = pitchValueIn;
    lastPitchAmount = pitchAmountIn;
  }

  /*
  If there's a scale, we'll use lastPitchValue and lastPitchAmount to determine
  the quantized pitch. If not, we'll use the immediate values. This way when not
  quantized you can vary the pitch continuously, but when quantized it only
  changes when notes are triggered.
  */
  float getOscillatorFrequency(float immediatePitchValue, float immediatePitchAmount) {
    // when quantizing to a scale, only change the pitch when new notes get triggered
    float rawValue = scale ? lastPitchValue : immediatePitchValue;
    float note = 76.f * rawValue;

    float baseFrequency = getFrequencyForNote(scale, note);

    // scale up up to 1 octave
    // TODO: 2 might be nice?
    float rawAmount = scale ? lastPitchAmount : immediatePitchAmount;

    float pitchAmountOffsetSemitones = getSemitoneOffsetForNote(scale, rawAmount);

    float value;
    if (scale) {
      int newNote = note + pitchAmountOffsetSemitones;

      // clamp it just in case
      if (newNote < 0) {
        newNote = 0;
      } else if (newNote > 87) {
        newNote = 87;
      }

      value = notes[newNote];

    } else {
      value = addSemitonesToFrequency(baseFrequency, pitchAmountOffsetSemitones);

      // clamp it just in case
      value = fclamp(value, 0.f, 22050.f);
    }

    return value;
  }

  private:
  int scale;
  float lastPitchValue;
  float lastPitchAmount;
};

}  // namespace platform


#endif