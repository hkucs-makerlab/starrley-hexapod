
////////////////////////////////////////////////////////////////////////////////
//           Vorpal Hexapod Control Program
//
// Copyright (C) 2017, 2018 Vorpal Robotics, LLC.
//
// See below all the license comments for new features in this version. (Search for NEW FEATURES)

const char *Version = "#RV2r1a";

//////////// FOR MORE INFORMATION ///////////////////////////////////
// Main website:                  http://www.vorpalrobotics.com
// Store (for parts and kits):    http://store.vorpalrobotics.com
// VORPAL WIKI RELATED TECHNICAL LINKS
// Main entry for hexapod:   http://vorpalrobotics.com/wiki/index.php?title=Vorpal_The_Hexapod
// Radio protocol:           http://vorpalrobotics.com/wiki/index.php/Vorpal_The_Hexapod_Radio_Protocol_Technical_Information
// Grip arm add-on:          http://vorpalrobotics.com/wiki/index.php?title=Vorpal_The_Hexapod_Grip_Arm
// Downloading 3D files:     http://vorpalrobotics.com/wiki/index.php/Vorpal_Hexapod_Source_Files
// Other Vorpal projects:    http://www.vorpalrobotics.com/wiki
/////////////////////////////////////////////////////////////////////


///////////  LICENSE:
//
// This work is licensed under the Creative Commons
// Attribution-NonCommercial-ShareAlike 4.0 International License.
// To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/4.0/.
// Attribution for derivations of this work should be made to: Vorpal Robotics, LLC
//
// You may use this work for noncommercial purposes without cost as long as you give us
// credit for our work (attribution) and any improvements you make are licensed in a way
// no more restrictive than our license.
//
// For example, you may build a Hexapod yourself and use this code for your own experiments,
// or you can build one and give the hexapod running this code to a friend, as long as you
// don't charge for it.
//
// If you have a question about whether a contemplated use is in compliance with the license,
// just ask us. We're friendly. Email us at support@vorpalrobotics.com
//
// For information on licensing this work for commercial purposes,
// please send email to support@vorpalrobotics.com and we'll work with you to come up with
// something fair.
//
// This is the program that goes on the Robot, not the gamepad!
//
// For more technical informatio, see http://www.vorpalrobotics.com
//

// NOTICE:
// This software uses the Adafruit Servo Driver library. For more information
// see www.adafruit.com
//
// The following information is required to be posted because we are using
// the Adafruit library:

/***************************************************************************

  ADAFRUIT PWM SERVO DRIVER LIBRARY
  (See https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library)

  Software License Agreement (BSD License)

  Copyright (c) 2012, Adafruit Industries
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holders nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/
/////////////////////////////////////////////////////////////////////////////////

////////////// NEW IN THIS RELEASE /////////////////////
// This release has two major new items:
// 1) Grip Arm support. When the robot dial is set to a position between DEMO and RC this code replaces
//    F2 fight mode with grip arm controls. Forward and Backward DPAD buttons raise and lower
//    the grip arm. Left opens and right closes the claw. The grip arm kit is open source, see
//    the list of links near the top of this file for more information.
//
// 2) Safe Legs. [Currently disabled] The old code allowed gamepad users to rapidly switch between any two motions
//    and some combinations of switches would super stress the servos. We believe this was a
//    major cause of servo wear. For example, if you switched from D1 LEFT (three legs up dancing)
//    to D1 RIGHT (the other three legs up) then depending on timing, you could slam the robot to
//    the ground and then it would attempt to rise up using only 3 legs. It really takes at least 4
//    legs to get the robot off the ground but preferably when rising up from the ground you should
//    try to use all six to keep the servos happy. So this really stresses the three servos trying to lift
//    the robot. Often the robot would only partially rise in this situation, causing the three servos
//    on the ground to stall and they would rapidly overheat if the user keeps the robot like this
//    for any length of time. This kind of thing can also happen with certain combinations of D4 (Mr. Hands
//    Mode) and F1 or F2 (fight mode in which two legs are off the ground.
//    We noticed that in public demos, little kids would do things like this all the time, and usually by
//    the end of the day we'd have a dead servo on half the demo robots, or more if it was a really rough crowd.
//    Anway, to make a long story longer, this release attempts to detect this situation and inserts a short (0.2 second)
//    extra move where all six legs go to the ground, followed by the requested action.
//    The method employed is a bit crude, we just keep a flag SomeLegsUp that is set to 1 whenever a requested move
//    would have less than all the legs on the ground. If a new requested move also would not have all legs on the ground
//    then a short intermediate move is inserted to get the robot up using all the legs.
//    There's a better way that we will pursue in a future
//    release which involves keeping a model of how many legs are on the ground and detecting situations where the
//    intermediate move is needed. This other method is harder to implement but has the advantage that it would
//    detect and correct even Scratch programs that would overstress the servos. The method implemented in this release
//    will not be able to correct for an errant scratch program that triggers this situation.

#include <Servo.h>
#include <EEPROM.h>


byte SomeLegsUp = 0;  // this is a flag to detect situations where a user rapidly switches moves that would
// cause the robot to try to come up off the ground using fewer than all the legs
//(and thus over-stressing the servos).

// NOTE: For digital servos such as Genuine Tower Pro MG90S or Turnigy MG90S we recommend putting
// a smal rubber washer on the hip servo shaft before putting the servo horn on. This will reduce or eliminate
// "hunting" behavior which can cause the servo to rapidly oscillate around the target position. Adjusting
// the servo horn screw tightness to be just tight enough to stop any hunting is recommended.
// This is not needed for analog servos and it is not needed for the Vorpal MG90 branded servos.
//
//#define __DEBUG__
//#define __VORPAL_FRAME__
//#define __ULTRA_SND__
#define __GO_BLE__

#ifdef __GO_BLE__
#include "GoBLE.h"
#define CONSOLE_BAUD 115200
#define BLUETOOTH_BAUD 115200
#else 
#define CONSOLE_BAUD 38400
#define BLUETOOTH_BAUD 38400
#endif

// pins assignment
#ifdef __ULTRA_SND__
#define ULTRAOUTPUTPIN A0    // TRIG
#define ULTRAINPUTPIN  A1     // ECHO
#endif
#define BeeperPin A2           // digital A2 used for beeper
#define DIAL_PIN A3
#ifdef __DEBUG__
#include <SoftwareSerial.h>
#define BT_TX A4
#define BT_RX A5
#else
#define LED_INDICATOR A4
#endif
#define GripElbowCurrentPin A6  // current sensor for grip arm elbow servo, only used if GRIPARM mode
#define GripClawCurrentPin  A7  // current sensor for grip claw servo, only used if GRIPARM mode
//
#define BF_ERROR  100         // deep beep for error situations
#define BD_MED    50          // medium long beep duration


// Depending on your servo make, the pulse width min and max may vary, you
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you
// have!  If you hear buzzing or jittering, you went too far.
// These values are good for MG90S clone small metal gear servos and Genuine Tower Pro MG90S

#define SERVOMIN  (190) // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  (540) // this is the 'maximum' pulse length count (out of 4096)

// Basic functions that move legs take a bit pattern
// indicating which legs to move. The legs are numbered
// clockwise starting with the right front leg being
// number zero, going around
// to the left legs, and finishing with the left front leg
// being number 5

#define NUM_LEGS 6

// Bit patterns for different combinations of legs
// bottom six bits. LSB is leg number 0

#define ALL_LEGS      0b111111
#define LEFT_LEGS     0b111000
#define RIGHT_LEGS    0b000111
#define TRIPOD1_LEGS  0b010101
#define TRIPOD2_LEGS  0b101010
#define FRONT_LEGS    0b100001
#define MIDDLE_LEGS   0b010010
#define BACK_LEGS     0b001100
#define NO_LEGS       0b0

// individual leg bitmasks
#define LEG0 0b1
#define LEG1 0b10
#define LEG2 0b100
#define LEG3 0b1000
#define LEG4 0b10000
#define LEG5 0b100000

#define LEG0BIT  0b1
#define LEG1BIT  0b10
#define LEG2BIT  0b100
#define LEG3BIT  0b1000
#define LEG4BIT  0b10000
#define LEG5BIT  0b100000

#define ISFRONTLEG(LEG) (LEG==0||LEG==5)
#define ISMIDLEG(LEG)   (LEG==1||LEG==4)
#define ISBACKLEG(LEG)  (LEG==2||LEG==3)
#define ISLEFTLEG(LEG)  (LEG==0||LEG==1||LEG==2)
#define ISRIGHTLEG(LEG) (LEG==3||LEG==4||LEG==5)

// default positions for knee and hip. Note that hip position is
// automatically reversed for the left side by the setHip function
// These are in degrees

#define KNEE_UP_MAX 180
#define KNEE_UP    150
#define KNEE_RELAX  120
#define KNEE_NEUTRAL 90
#define KNEE_CROUCH 110
#define KNEE_HALF_CROUCH 80
#define KNEE_STAND 30
#define KNEE_DOWN  30
#define KNEE_TIPTOES 5
#define KNEE_FOLD 170

#define KNEE_SCAMPER (KNEE_NEUTRAL-20)

#define KNEE_TRIPOD_UP (KNEE_NEUTRAL-40)
#define KNEE_TRIPOD_ADJ 30

#define HIPSWING 25      // how far to swing hips on gaits like tripod or quadruped
#define HIPSMALLSWING 10  // when in fine adjust mode how far to move hips
#define HIPSWING_RIPPLE 20
#define HIP_FORWARD_MAX 175
#define HIP_FORWARD (HIP_NEUTRAL+HIPSWING)
#define HIP_FORWARD_SMALL (HIP_NEUTRAL+HIPSMALLSWING)
#define HIP_NEUTRAL 90
#define HIP_BACKWARD (HIP_NEUTRAL-HIPSWING)
#define HIP_BACKWARD_SMALL (HIP_NEUTRAL-HIPSMALLSWING)
#define HIP_BACKWARD_MAX 0
#define HIP_FORWARD_RIPPLE (HIP_NEUTRAL+HIPSWING_RIPPLE)
#define HIP_BACKWARD_RIPPLE (HIP_NEUTRAL-HIPSWING_RIPPLE)
#define HIP_FOLD 150

#define NOMOVE (-1)   // fake value meaning this aspect of the leg (knee or hip) shouldn't move

#define LEFT_START 3  // first leg that is on the left side
#define RIGHT_START 0 // first leg that is on the right side
#define KNEE_OFFSET 6 // add this to a leg number to get the knee servo number

// these modes are used to interpret incoming bluetooth commands

#define TRIPOD_CYCLE_TIME 750
#define RIPPLE_CYCLE_TIME 1800
#define FIGHT_CYCLE_TIME 660

#define MODE_WALK   'W'
#define MODE_DANCE  'D'
#define MODE_FIGHT  'F'
#define MODE_RECORD 'R'
#define MODE_LEG    'L'       // comes from scratch
#define MODE_GAIT   'G'       // comes from scratch
#define MODE_TRIM   'T'       // gamepad in trim mode

#define SUBMODE_1 '1'
#define SUBMODE_2 '2'
#define SUBMODE_3 '3'
#define SUBMODE_4 '4'

#define BATTERYSAVER 5000   // milliseconds in stand mode before servos all detach to save power and heat buildup

// Definitions for the Grip Arm optional attachment

#define GRIPARM_ELBOW_SERVO 12
#define GRIPARM_CLAW_SERVO  13
#define GRIPARM_ELBOW_DEFAULT 90
#define GRIPARM_CLAW_DEFAULT 90
#define GRIPARM_ELBOW_MIN 30
#define GRIPARM_ELBOW_MAX 180
#define GRIPARM_CLAW_MIN 30
#define GRIPARM_CLAW_MAX 120
#define GRIPARM_CURRENT_DANGER (980)

#define GRIPARM_ELBOW_NEUTRAL 90
#define GRIPARM_CLAW_NEUTRAL 90

int GripArmElbowTarget = 90, GripArmClawTarget = 90;

// Definitions for the servos

#define MAX_GRIPSERVOS 2
//#define NUM_GRIPSERVOS ((Dialmode == DIALMODE_RC_GRIPARM)?2:0)  // if we're in griparm mode there are 2 griparm servos, else there are none
#define NUM_GRIPSERVOS 0  // this version does not have GRIP servos

short ServoPos[2 * NUM_LEGS + MAX_GRIPSERVOS]; // the last commanded position of each servo
long ServoTime[2 * NUM_LEGS + MAX_GRIPSERVOS]; // the time that each servo was last commanded to a new position
byte ServoTrim[2 * NUM_LEGS + MAX_GRIPSERVOS]; // trim values for fine adjustments to servo horn positions
long startedStanding = 0;   // the last time we started standing, or reset to -1 if we didn't stand recently
long LastReceiveTime = 0;   // last time we got a bluetooth packet
unsigned long LastValidReceiveTime = 0;  // last time we got a completely valid packet including correct checksum

#define DIALMODE_STAND 0
#define DIALMODE_ADJUST 1
#define DIALMODE_TEST 2
#define DIALMODE_DEMO 3
#define DIALMODE_RC_GRIPARM 4
#define DIALMODE_RC 5

int Dialmode;   // What's the robot potentiometer set to?
//

#define Console Serial
/*
  #define BlueTooth Serial
  #define BLUETOOTH_BAUD CONSOLE_BAUD
*/
#ifdef __DEBUG__
SoftwareSerial BlueTooth(BT_RX, BT_TX);
#else
#define BlueTooth Serial
#endif

#define LEG_DOF (2 * NUM_LEGS)
Servo *servos[LEG_DOF] = {
  nullptr, nullptr,  nullptr, nullptr,
  nullptr,  nullptr,  nullptr,  nullptr,
  nullptr,  nullptr,  nullptr, nullptr
};

byte pins[LEG_DOF] = {
  10,
  7,
  13,
  3,
  6,
  9,
  11,
  5,
  12,
  2,
  4,
  8,
};

void beep(int f, int t) {
  if (f > 0 && t > 0) {
    tone(BeeperPin, f, t);
  } else {
    noTone(BeeperPin);
  }
}

void beep(int f) {  // if no second param is given we'll default to 250 milliseconds for the beep
  beep(f, 250);
}

///////////////////////////////////////////////////////////////
// Trim functions
///////////////////////////////////////////////////////////////
byte TrimInEffect = 1;
byte TrimCurLeg = 0;
byte TrimPose = 0;
#define TRIM_ZERO 127   // this value is the midpoint of the trim range (a byte)

void save_trims() {
#ifdef __DEBUG__
  Console.print("SAVE TRIMS:");
#endif
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    EEPROM.update(i + 1, ServoTrim[i]);
#ifdef __DEBUG__
    Console.print(ServoTrim[i]); Console.print(" ");
#endif
  }
#ifdef __DEBUG__
  Console.println("");
#endif
  EEPROM.update(0, 'V');

}
void erase_trims() {
#ifdef __DEBUG__
  Console.println("ERASE TRIMS");
#endif
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    ServoTrim[i] = TRIM_ZERO;
  }
}

// This function sets the positions of both the knee and hip in
// a single command.  For hip, the left side is reversed so
// forward direction is consistent.

// This function takes a bitmask to specify legs to move, note that
// the basic setHip and setKnee functions take leg numbers, not masks

// if a position is -1 then that means don't change that item

void setLeg(int legmask, int hip_pos, int knee_pos, int adj) {
  setLeg(legmask, hip_pos, knee_pos, adj, 0, 0);  // use the non-raw version with leanangle=0
}

// version with leanangle = 0
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw) {
  setLeg(legmask, hip_pos, knee_pos, adj, raw, 0);
}

void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw, int leanangle) {
  for (int i = 0; i < NUM_LEGS; i++) {
    if (legmask & 0b1) {  // if the lowest bit is ON
      if (hip_pos != NOMOVE) {
        if (!raw) {
          setHip(i, hip_pos, adj);
        } else {
          setHipRaw(i, hip_pos);
        }
      }
      if (knee_pos != NOMOVE) {
        int pos = knee_pos;
        if (leanangle != 0) {
          switch (i) {
            case 0: case 6: case 5: case 11:
              if (leanangle < 0) pos -= leanangle;
              break;
            case 1: case 7: case 4: case 10:
              pos += abs(leanangle / 2);
              break;
            case 2: case 8: case 3: case 9:
              if (leanangle > 0) pos += leanangle;
              break;
          }
          //Console.print("Lean:"); Console.print(leanangle); Console.print("pos="); Console.println(pos);
        }

        setKnee(i, pos);
      }
    }
    legmask = (legmask >> 1); // shift down one bit position
  }
}

// this version of setHip does no processing at all (for example
// to distinguish left from right sides)
void setHipRaw(int leg, int pos) {
  setServo(leg, pos);
}

// this version of setHip adjusts for left and right legs so
// that 0 degrees moves "forward" i.e. toward legs 5-0 which is
// nominally the front of the robot

void setHip(int leg, int pos) {
  // reverse the left side for consistent forward motion
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }
  setHipRaw(leg, pos);
}

// this version of setHip adjusts not only for left and right,
// but also shifts the front legs a little back and the back legs
// forward to make a better balance for certain gaits like tripod or quadruped

void setHip(int leg, int pos, int adj) {
  if (ISFRONTLEG(leg)) {
    pos -= adj;
  } else if (ISBACKLEG(leg)) {
    pos += adj;
  }
  // reverse the left side for consistent forward motion
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }

  setHipRaw(leg, pos);
}

void setKnee(int leg, int pos) {
  // find the knee associated with leg if this is not already a knee
  if (leg < KNEE_OFFSET) {
    leg += KNEE_OFFSET;
  }
  setServo(leg, pos);
}

void setGrip(int elbow, int claw) {
  setServo(GRIPARM_ELBOW_SERVO, elbow);
  setServo(GRIPARM_CLAW_SERVO, claw);
}

void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod) {
  turn(ccw, hipforward, hipbackward, kneeup, kneedown, timeperiod, 0);
}

void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // use tripod groups to turn in place
  if (ccw) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_TURN_PHASES 6
#define FBSHIFT_TURN    40   // shift front legs back, back legs forward, this much

  long t = millis() % timeperiod;
  long phase = (NUM_TURN_PHASES * t) / timeperiod;

  //Console.print("PHASE: ");
  //Console.println(phase);

  switch (phase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move clockwise
      // at the hips, while the rest of the legs move CCW at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0);
      break;

    case 4:
      // similar to phase 1, move raised legs CW and lowered legs CCW
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0);
      break;
  }

}


void stand() {
  transactServos();
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
  commitServos();
}

void stand_90_degrees() {  // used to install servos, sets all servos to 90 degrees
  transactServos();
  setLeg(ALL_LEGS, 90, 90, 0);
  setGrip(90, 90);
  commitServos();
}

void laydown() {
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_UP, 0);
}

void tiptoes() {
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
}

void wave(int dpad) {

#define NUM_WAVE_PHASES 12
#define WAVE_CYCLE_TIME 900
#define KNEE_WAVE  60
  long t = millis() % WAVE_CYCLE_TIME;
  long phase = (NUM_WAVE_PHASES * t) / WAVE_CYCLE_TIME;

  if (dpad == 'b') {
    phase = 11 - phase; // go backwards
  }

  switch (dpad) {
    case 'f':
    case 'b':
      // swirl around
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0); // keep hips stable at 90 degrees
      if (phase < NUM_LEGS) {
        setKnee(phase, KNEE_WAVE);
      } else {
        setKnee(phase - NUM_LEGS, KNEE_STAND);
      }
      break;
    case 'l':
      // teeter totter around font/back legs

      if (phase < NUM_WAVE_PHASES / 2) {
        setKnee(0, KNEE_TIPTOES);
        setKnee(5, KNEE_STAND);
        setHipRaw(0, HIP_FORWARD);
        setHipRaw(5, HIP_BACKWARD - 40);
        setKnee(2, KNEE_TIPTOES);
        setKnee(3, KNEE_STAND);
        setHipRaw(2, HIP_BACKWARD);
        setHipRaw(3, HIP_FORWARD + 40);

        setLeg(LEG1, HIP_NEUTRAL, KNEE_TIPTOES, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
      } else {
        setKnee(0, KNEE_STAND);
        setKnee(5, KNEE_TIPTOES);
        setHipRaw(0, HIP_FORWARD + 40);
        setHipRaw(5, HIP_BACKWARD);
        setKnee(2, KNEE_STAND);
        setKnee(3, KNEE_TIPTOES);
        setHipRaw(2, HIP_BACKWARD - 40);
        setHipRaw(3, HIP_FORWARD);

        setLeg(LEG1, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_TIPTOES, 0);
      }
      break;
    case 'r':
      // teeter totter around middle legs
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
      if (phase < NUM_LEGS) {
        setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
        setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
      } else {
        setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
        setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
      }
      break;
    case 'w':
      // lay on ground and make legs go around in a wave
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0);
      int p = phase / 2;
      for (int i = 0; i < NUM_LEGS; i++) {
        if (i == p) {
          setKnee(i, KNEE_UP_MAX);
        } else {
          setKnee(i, KNEE_NEUTRAL);
        }
      }
      return;
      if (phase < NUM_LEGS) {
        setKnee(phase / 2, KNEE_UP);
      } else {
        int p = phase - NUM_LEGS;
        if (p < 0) p += NUM_LEGS;
        setKnee(p / 2, KNEE_NEUTRAL + 10);
      }
      break;
  }
}

void gait_sidestep(int left, long timeperiod) {

  // the gait consists of 6 phases and uses tripod definitions

#define NUM_SIDESTEP_PHASES 6
#define FBSHIFTSS    50   // shift front legs back, back legs forward, this much

  long t = millis() % timeperiod;
  long phase = (NUM_SIDESTEP_PHASES * t) / timeperiod;
  int side1 = LEFT_LEGS;
  int side2 = RIGHT_LEGS;

  if (left == 0) {
    side1 = RIGHT_LEGS;
    side2 = LEFT_LEGS;
  }

  //Console.print("PHASE: ");
  //Console.println(phase);

  transactServos();

  switch (phase) {
    case 0:
      // Lift up tripod group 1 while group 2 goes to neutral setting
      setLeg(TRIPOD1_LEGS, HIP_NEUTRAL, KNEE_UP, FBSHIFTSS);
      setLeg(TRIPOD2_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      break;

    case 1:
      // slide over by curling one side under the body while extending the other side
      setLeg(TRIPOD2_LEGS & side1, HIP_NEUTRAL, KNEE_DOWN, FBSHIFTSS);
      setLeg(TRIPOD2_LEGS & side2, HIP_NEUTRAL, KNEE_RELAX, FBSHIFTSS);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      // and at the sametime put the curled legs into neutral position
      setLeg(TRIPOD2_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      setLeg(TRIPOD1_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      break;

    case 3:
      // Lift up tripod group 2 while group 2 goes to neutral setting
      setLeg(TRIPOD2_LEGS, HIP_NEUTRAL, KNEE_UP, FBSHIFTSS);
      setLeg(TRIPOD1_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      break;

    case 4:
      // slide over by curling one side under the body while extending the other side
      setLeg(TRIPOD1_LEGS & side1, HIP_NEUTRAL, KNEE_DOWN, FBSHIFTSS);
      setLeg(TRIPOD1_LEGS & side2, HIP_NEUTRAL, KNEE_RELAX, FBSHIFTSS);
      break;

    case 5:
      // now put all the legs back down on the ground, then the cycle repeats
      setLeg(TRIPOD1_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      setLeg(TRIPOD2_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, FBSHIFTSS);
      break;
  }
  commitServos();
}

int GripArmElbowDestination = 90;
short GripArmElbowIncrement = 0;

void griparm_mode(char dpad) {
  // this mode retains state and moves slowly

  //Console.print("Grip:"); Console.print(dpad);
#ifdef __DEBUG__
  Console.println();
#endif
  switch (dpad) {
    case 's':
      // do nothing in stop mode, just hold current position
      GripArmElbowTarget = ServoPos[GRIPARM_ELBOW_SERVO];
      GripArmClawTarget = ServoPos[GRIPARM_CLAW_SERVO];
      break;
    case 'w':  // reset to standard grip arm position, arm raised to mid-level and grip open a medium amount
      GripArmElbowTarget = GRIPARM_ELBOW_DEFAULT;
      GripArmClawTarget = GRIPARM_CLAW_DEFAULT;
      break;
    case 'f': // Elbow up
      GripArmElbowTarget = GRIPARM_ELBOW_MIN;
      break;
    case 'b': // Elbow down
      GripArmElbowTarget = GRIPARM_ELBOW_MAX;
      break;
    case 'l':  // Claw closed
      GripArmClawTarget = GRIPARM_CLAW_MAX;
      break;
    case 'r': // Claw open
      GripArmClawTarget = GRIPARM_CLAW_MIN;
      break;
  }

  // Now that all the targets are adjusted, move the servos toward their targets, slowly

#define GRIPMOVEINCREMENT 10  // degrees per transmission time delay

  int h, k;
  h = ServoPos[GRIPARM_ELBOW_SERVO];
  k = ServoPos[GRIPARM_CLAW_SERVO];
  int diff = GripArmClawTarget - k;

  if (diff <= -GRIPMOVEINCREMENT) {
    // the claw has a greater value than the target
    k -= GRIPMOVEINCREMENT;
  } else if (diff >= GRIPMOVEINCREMENT) {
    // the claw has a smaller value than the target
    k += GRIPMOVEINCREMENT;
  } else {
    // the claw is within MOVEINCREMENT of the target so just go to target
    k = GripArmClawTarget;
  }

  setServo(GRIPARM_CLAW_SERVO, k);

  diff = GripArmElbowTarget - h;

  // smooth move mode on elbow
  if (diff < -GRIPMOVEINCREMENT) {
    // the elbow has a greater value than the target
    h -= GRIPMOVEINCREMENT;
  } else if (diff >= GRIPMOVEINCREMENT) {
    // the elbow has a smaller value than the target
    h += GRIPMOVEINCREMENT;
  } else {
    // the elbow is within MOVEINCREMENT of the target so just go to target
    h = GripArmElbowTarget;
  }
  GripArmElbowDestination = h;
  GripArmElbowIncrement = (h < ServoPos[GRIPARM_ELBOW_SERVO]) ? -2 : 2;
  //Console.print("GADest="); Console.print(h); Console.print(" GAinc="); Console.println(GripArmElbowIncrement);

}

unsigned short KneeTarget[NUM_LEGS];
unsigned short HipTarget[NUM_LEGS];

void fight_mode(char dpad, int mode, long timeperiod) {

#define HIP_FISTS_FORWARD 130

  if (Dialmode == DIALMODE_RC_GRIPARM && mode == SUBMODE_2) {
    // we're really not fighting, we're controlling the grip arm if GRIPARM is nonzero
    griparm_mode(dpad);
    return;
  }

  if (mode == SUBMODE_3) {
    // in this mode the robot leans forward, left, or right by adjusting hips only

    // this mode retains state and moves slowly, it's for getting somethign like the joust or
    // capture the flag accessories in position

    switch (dpad) {
      case 's':
        // do nothing in stop mode, just hold current position
        for (int i = 0; i < NUM_LEGS; i++) {
          KneeTarget[i] = ServoPos[i + NUM_LEGS];
          HipTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          KneeTarget[i] = KNEE_STAND;
          HipTarget[i] = 90;
        }
        break;
      case 'f': // swing hips forward, mirrored
        HipTarget[5] = HipTarget[4] = HipTarget[3] = 125;
        HipTarget[0] = HipTarget[1] = HipTarget[2] = 55;
        break;
      case 'b': // move the knees back up to standing position, leave hips alone
        HipTarget[5] = HipTarget[4] = HipTarget[3] = 55;
        HipTarget[0] = HipTarget[1] = HipTarget[2] = 125;
        break;
      case 'l':
        for (int i = 0; i < NUM_LEGS; i++) {
          HipTarget[i] = 170;
        }
        break;
      case 'r':
        for (int i = 0; i < NUM_LEGS; i++) {
          HipTarget[i] = 10;
        }
        break;
    }

  } else if (mode == SUBMODE_4) {
    // in this mode the entire robot leans in the direction of the pushbuttons
    // and the weapon button makes the robot return to standing position.

    // Only knees are altered by this, not hips (other than the reset action for
    // the special D-PAD button)

    // this mode does not immediately set servos to final positions, instead it
    // moves them toward targets slowly.

    switch (dpad) {
      case 's':
        // do nothing in stop mode, just hold current position
        for (int i = 0; i < NUM_LEGS; i++) {
          KneeTarget[i] = ServoPos[i + NUM_LEGS];
          HipTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          KneeTarget[i] = KNEE_STAND;
          HipTarget[i] = 90;
        }
        break;
      case 'f': // move knees into forward crouch, leave hips alone

        if (ServoPos[8] == KNEE_STAND) { // the back legs are standing, so crouch the front legs
          KneeTarget[0] = KneeTarget[5] = KNEE_CROUCH;
          KneeTarget[1] = KneeTarget[4] = KNEE_HALF_CROUCH;
          KneeTarget[2] = KneeTarget[3] = KNEE_STAND;
        } else { // bring the back legs up first
          for (int i = 0; i < NUM_LEGS; i++) {
            KneeTarget[i] = KNEE_STAND;
          }
        }
        break;
      case 'b': // move back legs down so robot tips backwards
        if (ServoPos[6] == KNEE_STAND) { // move the back legs down
          KneeTarget[0] = KneeTarget[5] = KNEE_STAND;
          KneeTarget[1] = KneeTarget[4] = KNEE_HALF_CROUCH;
          KneeTarget[2] = KneeTarget[3] = KNEE_CROUCH;
        } else { // front legs are down, return to stand first
          for (int i = 0; i < NUM_LEGS; i++) {
            KneeTarget[i] = KNEE_STAND;
          }
        }
        break;
      case 'l':
        if (ServoPos[9] == KNEE_STAND) {
          KneeTarget[0] = KneeTarget[2] = KNEE_HALF_CROUCH;
          KneeTarget[1] = KNEE_CROUCH;
          KneeTarget[3] = KneeTarget[4] = KneeTarget[5] = KNEE_STAND;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            KneeTarget[i] = KNEE_STAND;
          }
        }
        break;
      case 'r':
        if (ServoPos[6] == KNEE_STAND) {
          KneeTarget[0] = KneeTarget[1] = KneeTarget[2] = KNEE_STAND;
          KneeTarget[3] = KneeTarget[5] = KNEE_HALF_CROUCH;
          KneeTarget[4] = KNEE_CROUCH;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            KneeTarget[i] = KNEE_STAND;
          }
        }
        break;

    }
  }

  if (mode == SUBMODE_4 || mode == SUBMODE_3) { // incremental moves

    // move servos toward their targets
#define MOVEINCREMENT 10  // degrees per transmission time delay

    for (int i = 0; i < NUM_LEGS; i++) {
      int h, k;
      h = ServoPos[i];
      k = ServoPos[i + KNEE_OFFSET];
      int diff = KneeTarget[i] - k;

      if (diff <= -MOVEINCREMENT) {
        // the knee has a greater value than the target
        k -= MOVEINCREMENT;
      } else if (diff >= MOVEINCREMENT) {
        // the knee has a smaller value than the target
        k += MOVEINCREMENT;
      } else {
        // the knee is within MOVEINCREMENT of the target so just go to target
        k = KneeTarget[i];
      }

      setKnee(i, k);

      diff = HipTarget[i] - h;

      if (diff <= -MOVEINCREMENT) {
        // the hip has a greater value than the target
        h -= MOVEINCREMENT;
      } else if (diff >= MOVEINCREMENT) {
        // the hip has a smaller value than the target
        h += MOVEINCREMENT;
      } else {
        // the knee is within MOVEINCREMENT of the target so just go to target
        h = HipTarget[i];
      }

      setHipRaw(i, h);
      //Console.print("RAW "); Console.print(i); Console.print(" "); Console.println(h);

    }
    return;  // /this mode does not execute the rest of the actions
  }

  // If we get here, we are in either submode A or B
  //
  // submode A: fight with two front legs, individual movement
  // submode B: fight with two front legs, in unison

  setLeg(MIDDLE_LEGS, HIP_FORWARD + 10, KNEE_STAND, 0);
  setLeg(BACK_LEGS, HIP_BACKWARD, KNEE_STAND, 0);

  switch (dpad) {
    case 's':  // stop mode: both legs straight out forward
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_NEUTRAL, 0);

      break;

    case 'f':  // both front legs move up in unison
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
      break;

    case 'b':  // both front legs move down in unison
      setLeg(FRONT_LEGS, HIP_FORWARD, KNEE_STAND, 0);
      break;

    case 'l':  // left front leg moves left, right stays forward
      if (mode == SUBMODE_1) {
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else {
        // both legs move in unison in submode B
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;

    case 'r':  // right front leg moves right, left stays forward
      if (mode == SUBMODE_1) {
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else { // submode B
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;

    case 'w':  // automatic ninja motion mode with both legs swinging left/right/up/down furiously!

#define NUM_PUGIL_PHASES 8
      { // we need a new scope for this because there are local variables

        long t = millis() % timeperiod;
        long phase = (NUM_PUGIL_PHASES * t) / timeperiod;

        //Console.print("PHASE: ");
        //Console.println(phase);

        switch (phase) {
          case 0:
            // Knees down, hips forward
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_RELAX, 0);
            break;

          case 1:
            // Knees up, hips forward
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;

          case 2:
            // Knees neutral, hips neutral
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_NEUTRAL, 0);
            break;

          case 3:
            // Knees up, hips neutral
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_UP, 0);
            break;

          case 4:
            // hips forward, kick
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
            break;

          case 5:
            // kick phase 2
            // hips forward, kick
            setLeg(LEG0, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;

          case 6:
            // hips forward, kick
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
            break;

          case 7:
            // kick phase 2
            // hips forward, kick
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;
        }
      }
  }

}

void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod) {

  // this version makes leanangle zero
  gait_tripod(reverse, hipforward, hipbackward,
              kneeup, kneedown, timeperiod, 0);
}

void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod, int leanangle) {

  // the gait consists of 6 phases. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all six  phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_TRIPOD_PHASES 6
#define FBSHIFT    15   // shift front legs back, back legs forward, this much

  long t = millis() % timeperiod;
  long phase = (NUM_TRIPOD_PHASES * t) / timeperiod;

  //Console.print("PHASE: ");
  //Console.println(phase);

  transactServos(); // defer leg motions until after checking for crashes
  switch (phase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move forward
      // at the hips, while the rest of the legs move backward at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;

    case 4:
      // similar to phase 1, move raised legs forward and lowered legs backward
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;
  }
  commitServos(); // implement all leg motions
}

int ScamperPhase = 0;
unsigned long NextScamperPhaseTime = 0;

long ScamperTracker = 0;

void gait_tripod_scamper(int reverse, int turn) {

  ScamperTracker += 2;  // for tracking if the user is over-doing it with scamper

  // this is a tripod gait that tries to go as fast as possible by not waiting
  // for knee motions to complete before beginning the next hip motion

  // this was experimentally determined and assumes the battery is maintaining
  // +5v to the servos and they are MG90S or equivalent speed. There is very
  // little room left for slower servo motion. If the battery voltage drops below
  // 6.5V then the BEC may not be able to maintain 5.0V to the servos and they may
  // not complete motions fast enough for this to work.

  int hipforward, hipbackward;

  if (reverse) {
    hipforward = HIP_BACKWARD;
    hipbackward = HIP_FORWARD;
  } else {
    hipforward = HIP_FORWARD;
    hipbackward = HIP_BACKWARD;
  }

#define FBSHIFT    15   // shift front legs back, back legs forward, this much
#define SCAMPERPHASES 6

#define KNEEDELAY 35   //30
#define HIPDELAY 100   //90

  if (millis() >= NextScamperPhaseTime) {
    ScamperPhase++;
    if (ScamperPhase >= SCAMPERPHASES) {
      ScamperPhase = 0;
    }
    switch (ScamperPhase) {
      case 0: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 1: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 2: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 3: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 4: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 5: NextScamperPhaseTime = millis() + KNEEDELAY; break;
    }

  }

  //Console.print("ScamperPhase: "); Console.println(ScamperPhase);

  transactServos();
  switch (ScamperPhase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_SCAMPER, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move forward
      // at the hips, while the rest of the legs move backward at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_SCAMPER, 0, turn);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0, turn);
      break;

    case 4:
      // similar to phase 1, move raised legs forward and lowered legs backward
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;
  }
  commitServos();
}

// call gait_ripple with leanangle = 0
void gait_ripple(int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod) {
  gait_ripple(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, 0);
}

void gait_ripple(int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // the gait consists of 10 phases. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_RIPPLE_PHASES 19

  long t = millis() % timeperiod;
  long phase = (NUM_RIPPLE_PHASES * t) / timeperiod;

  //Console.print("PHASE: ");
  //Console.println(phase);

  transactServos();

  if (phase == 18) {
    setLeg(ALL_LEGS, hipbackward, NOMOVE, FBSHIFT);
  } else {
    int leg = phase / 3; // this will be a number between 0 and 2
    leg = 1 << leg;
    int subphase = phase % 3;

    switch (subphase) {
      case 0:
        setLeg(leg, NOMOVE, kneeup, 0);
        break;
      case 1:
        setLeg(leg, hipforward, NOMOVE, FBSHIFT);
        break;
      case 2:
        setLeg(leg, NOMOVE, kneedown, 0);
        break;
    }
  }
  commitServos();
}

#define G_STAND 0
#define G_TURN  1
#define G_TRIPOD 2
#define G_SCAMPER 3
#define G_DANCE 4
#define G_BOOGIE 5
#define G_FIGHT 6
#define G_TEETER 7
#define G_BALLET 8

#define G_NUMGATES 9

int curGait = G_STAND;
int curReverse = 0;
unsigned long nextGaitTime = 0;

void random_gait(int timingfactor) {

#define GATETIME 3500  // number of milliseconds for each demo

  if (millis() > nextGaitTime) {
    curGait++;
    if (curGait >= G_NUMGATES) {
      curGait = 0;
    }
    nextGaitTime = millis() + GATETIME;

    // when switching demo modes, briefly go into a standing position so
    // we're starting at the same position every time.
    setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
    delay(600);
  }

  switch (curGait) {
    case G_STAND:
      stand();
      break;
    case G_TURN:
      turn(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); // 700
      break;
    case G_TRIPOD:
      gait_tripod(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); // 900
      break;
    case G_SCAMPER:
      gait_tripod_scamper((nextGaitTime - (millis()) < GATETIME / 2), 0); // reverse direction halfway through
      break;
    case G_DANCE:
      stand();
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 145);
      delay(350);
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 35);
      delay(350);
      break;
    case G_BOOGIE:
      boogie_woogie(NO_LEGS, SUBMODE_1, 2);
      break;
    case G_FIGHT:
      fight_mode('w', SUBMODE_1, FIGHT_CYCLE_TIME);
      break;

    case G_TEETER:
      wave('r');
      break;

    case G_BALLET:
      flutter();
      break;

  }

}

void foldup() {
  setLeg(ALL_LEGS, NOMOVE, KNEE_FOLD, 0);
  for (int i = 0; i < NUM_LEGS; i++)
    setHipRaw(i, HIP_FOLD);
}

void dance_dab(int timingfactor) {
#define NUM_DAB_PHASES 3

  long t = millis() % (1100 * timingfactor);
  long phase = (NUM_DAB_PHASES * t) / (1100 * timingfactor);

  switch (phase) {
    case 0:
      stand(); break;

    case 1:
      setKnee(6, KNEE_UP); break;

    case 2:
      for (int i = 0; i < NUM_LEGS; i++)
        if (i != 0) setHipRaw(i, 40);
      setHipRaw(0, 140);
      break;
  }
}

void flutter() {   // ballet flutter legs on pointe
#define NUM_FLUTTER_PHASES 4
#define FLUTTER_TIME 200
#define KNEE_FLUTTER (KNEE_TIPTOES+20)

  long t = millis() % (FLUTTER_TIME);
  long phase = (NUM_FLUTTER_PHASES * t) / (FLUTTER_TIME);

  setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);

  switch (phase) {
    case 0:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_FLUTTER, 0, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 1:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 2:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_FLUTTER, 0, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 3:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
  }

}

void dance_ballet(int dpad) {   // ballet flutter legs on pointe

#define BALLET_TIME 250

  switch (dpad) {

    default:
    case 's': tiptoes(); return;

    case 'w': flutter(); return;

    case 'l':
      turn(1, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;

    case 'r':
      turn(0, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;

    case 'f':
      gait_tripod(0, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;

    case 'b':
      gait_tripod(1, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;
  }
}

void dance_hands(int dpad) {

  setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_STAND, 0, 0);
  setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_STAND, 0, 0);

  switch (dpad) {
    case 's':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_UP, 0, 0);
      break;
    case 'f':
      setLeg(MIDDLE_LEGS, HIP_FORWARD_MAX, KNEE_UP_MAX, 0, 0);
      break;
    case 'b':
      setLeg(MIDDLE_LEGS, HIP_BACKWARD_MAX, KNEE_UP_MAX, 0, 0);
      break;
    case 'l':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
      setLeg(LEG1, NOMOVE, KNEE_NEUTRAL, 0, 0);
      setLeg(LEG4, NOMOVE, KNEE_UP_MAX, 0, 0);
      break;
    case 'r':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
      setLeg(LEG1, NOMOVE, KNEE_UP_MAX, 0, 0);
      setLeg(LEG4, NOMOVE, KNEE_NEUTRAL, 0, 0);
      break;
    case 'w':
      // AUTOMATIC MODE
#define NUM_HANDS_PHASES 2
#define HANDS_TIME_PERIOD 400
      { // we need a new scope for this because there are local variables

        long t = millis() % HANDS_TIME_PERIOD;
        long phase = (NUM_HANDS_PHASES * t) / HANDS_TIME_PERIOD;


        switch (phase) {
          case 0:
            setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
            setLeg(LEG1, NOMOVE, KNEE_NEUTRAL, 0, 0);
            setLeg(LEG4, NOMOVE, KNEE_UP_MAX, 0, 0);
            break;

          case 1:
            setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
            setLeg(LEG1, NOMOVE, KNEE_UP_MAX, 0, 0);
            setLeg(LEG4, NOMOVE, KNEE_NEUTRAL, 0, 0);
            break;

        }
      }
      break;
  }
}

void dance(int legs_up, int submode, int timingfactor) {
  setLeg(legs_up, NOMOVE, KNEE_UP, 0, 0);
  setLeg((legs_up ^ 0b111111), NOMOVE, ((submode == SUBMODE_1) ? KNEE_STAND : KNEE_TIPTOES), 0, 0);

#define NUM_DANCE_PHASES 2

  long t = millis() % (600 * timingfactor);
  long phase = (NUM_DANCE_PHASES * t) / (600 * timingfactor);

  switch (phase) {
    case 0:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 140);
      break;
    case 1:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 40);
      break;
  }
}


void boogie_woogie(int legs_flat, int submode, int timingfactor) {

  setLeg(ALL_LEGS, NOMOVE, KNEE_UP, 0);
  //setLeg(legs_flat, NOMOVE, KNEE_RELAX, 0, 0);

#define NUM_BOOGIE_PHASES 2

  long t = millis() % (400 * timingfactor);
  long phase = (NUM_BOOGIE_PHASES * t) / (400 * timingfactor);

  switch (phase) {
    case 0:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 140);
      break;

    case 1:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 40);
      break;
  }
}

int ServosDetached = 0;

void attach_all_servos() {
#ifdef __DEBUG__
  Console.print("ATTACH");
#endif
  for (int i = 0; i < 2 * NUM_LEGS; i++) {
    if (servos[i] == nullptr) {
      servos[i] = new Servo();
      servos[i]->attach(pins[i], SERVOMIN, SERVOMAX);
    } else {
      servos[i]->attach(pins[i], SERVOMIN, SERVOMAX);
    }
    setServo(i, ServoPos[i]);
#ifdef __DEBUG__
    Console.print(ServoPos[i]); Console.print(":");
#endif
  }
#ifdef __DEBUG__
  Console.println("");
#endif
  ServosDetached = 0;
  return;
}

void detach_all_servos() {
  //Console.println("DETACH");
  for (int i = 0; i < 2 * NUM_LEGS; i++) {
    //servoDriver.setPin(i, 0, false); // stop pulses which will quickly detach the servo
    if (servos[i] != nullptr) {
      servos[i]->detach();
    }
  }
  ServosDetached = 1;
}

void setup() {
  Serial.begin(CONSOLE_BAUD);
  Serial.println("");
  Serial.println(Version);

  pinMode(BeeperPin, OUTPUT);
  beep(200);

  // read in trim values from eeprom if available
  if (EEPROM.read(0) == 'V') {
    // if the first byte in the eeprom is a capital letter V that means there are trim values
    // available. Note that eeprom from the factory is set to all 255 values.
#ifdef __DEBUG__
    Console.print("TRIMS: ");
#endif
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = EEPROM.read(i + 1);
#ifdef __DEBUG__
      Console.print(ServoTrim[i] - TRIM_ZERO); Console.print(" ");
#endif
    }
#ifdef __DEBUG__
    Console.println("");
#endif
  } else {
#ifdef __DEBUG__
    Console.println("TRIMS:unset");
#endif
    // init trim values to zero, no trim
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = TRIM_ZERO;   // this is the middle of the trim range and will result in no trim
    }
  }


  // make a characteristic flashing pattern to indicate the robot code is loaded (as opposed to the gamepad)
  // There will be a brief flash after hitting the RESET button, then a long flash followed by a short flash.
  // The gamepaid is brief flash on reset, short flash, long flash.
#ifndef __DEBUG__
  pinMode(LED_INDICATOR, OUTPUT);
  digitalWrite(LED_INDICATOR, HIGH);
  delay(300);
  digitalWrite(LED_INDICATOR, LOW);
  delay(150);
  digitalWrite(LED_INDICATOR, HIGH);
  delay(150);
  digitalWrite(LED_INDICATOR, LOW);
#endif
  ///////////////////// end of indicator flashing
  delay(300); // give hardware a chance to come up and stabalize
#ifdef __DEBUG__
  // in debug mode uses SoftwareSerial for RC control, in normal run uses Serial.
  Console.println("SoftwareSerial setup for bluetooth.");
  BlueTooth.begin(BLUETOOTH_BAUD);
  delay(250);
#endif
  BlueTooth.println(Version);

  delay(250);
  stand();
  setGrip(90, 90);  // neutral grip arm (if installed)

  delay(300);

  //CmuCam5.init();   // we're still working out some issues with CmuCam5

  beep(400); // Signals end of startup sequence
  attach_all_servos();
  yield();
}

// setServo is the lowest level function for setting servo positions.
// It handles trims too.

byte deferServoSet = 0;

void setServo(int servonum, int position) {
  if (position != ServoPos[servonum]) {
    ServoTime[servonum] = millis();
  }
  ServoPos[servonum] = position;  // keep data on where the servo was last commanded to go

  if (TrimInEffect && servonum < LEG_DOF) {
    //Console.print("Trim leg "); Console.print(servonum); Console.print(" "); Console.println(ServoTrim[servonum] - TRIM_ZERO);
    int p = map(position, 0, 180, SERVOMIN, SERVOMAX);
    p += ServoTrim[servonum] - TRIM_ZERO; // adjust microseconds by trim value which is renormalized to the range -127 to 128
    position = map(p, SERVOMIN, SERVOMAX, 0, 180);
  }
  if (!deferServoSet && servos[servonum]->attached()) {


#ifdef __VORPAL_FRAME__
    servos[servonum]->write(position);
#else
    switch (servonum) {
      case 6:
      case 7:
      case 9: servos[servonum]->write(position);
        break;
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 8:
      case 10:
      case 11: servos[servonum]->write(180 - position);
        break;
    }
#endif
  }
  // DEBUG: Uncomment the next line to debug setservo problems. It causes some lagginess due to all the printing
  //Console.print("SS:");Console.print(servonum);Console.print(":");Console.println(position);
}

void transactServos() {
  deferServoSet = 1;
}

void commitServos() {
  checkForCrashingHips();
  deferServoSet = 0;
  for (int servo = 0; servo < 2 * NUM_LEGS + NUM_GRIPSERVOS; servo++) {
    setServo(servo, ServoPos[servo]);
  }
}

// checkForCrashingHips takes a look at the leg angles and tries to figure out if the commanded
// positions might cause servo stall.  Now the correct way to do this would be to do fairly extensive
// trig computations to see if the edges of the hips touch. However, Arduino isn't really set up to
// do complicated trig stuff. It would take a lot of code space and a lot of time. So we're just using
// a simple approximation. In practice it stops very hard stall situations. Very minor stalls (where the
// motor is commanded a few degress farther than it can physically go) may still occur, but those won't
// draw much power (the current draw is proportional to how far off the mark the servo is).

void checkForCrashingHips() {

  for (int leg = 0; leg < NUM_LEGS; leg++) {
    if (ServoPos[leg] > 85) {
      continue; // it's not possible to crash into the next leg in line unless the angle is 85 or less
    }
    int nextleg = ((leg + 1) % NUM_LEGS);
    if (ServoPos[nextleg] < 100) {
      continue;   // it's not possible for there to be a crash if the next leg is less than 100 degrees
      // there is a slight assymmetry due to the way the servo shafts are positioned, that's why
      // this number does not match the 85 number above
    }
    int diff = ServoPos[nextleg] - ServoPos[leg];
    // There's a fairly linear relationship
    if (diff <= 85) {
      // if the difference between the two leg positions is less than about 85 then there
      // is not going to be a crash (or maybe just a slight touch that won't really cause issues)
      continue;
    }
    // if we get here then the legs are touching, we will adjust them so that the difference is less than 85
    int adjust = (diff - 85) / 2 + 1; // each leg will get adjusted half the amount needed to avoid the crash

    // to debug crash detection, make the following line #if 1, else make it #if 0
#ifdef __DEBUG__
    Console.print("#CRASH:");
    Console.print(leg); Console.print("="); Console.print(ServoPos[leg]);
    Console.print("/"); Console.print(nextleg); Console.print("="); Console.print(ServoPos[nextleg]);
    Console.print(" Diff="); Console.print(diff); Console.print(" ADJ="); Console.println(adjust);
#endif

    setServo(leg, ServoPos[leg] + adjust);
    setServo(nextleg, ServoPos[nextleg] - adjust);

  }
}

#ifdef __ULTRA_SND__
#define ULTRAOUTPUTPIN A4      // TRIG
#define ULTRAINPUTPIN  A5     // ECHO

unsigned int readUltrasonic() {  // returns number of centimeters from ultrasonic rangefinder

  pinMode(ULTRAOUTPUTPIN, OUTPUT);
  digitalWrite(ULTRAOUTPUTPIN, LOW);
  delayMicroseconds(5);
  digitalWrite(ULTRAOUTPUTPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRAOUTPUTPIN, LOW);

  unsigned int duration = pulseIn(ULTRAINPUTPIN, HIGH, 18000);  // maximum 18 milliseconds which would be about 10 feet distance from object

  //Console.print("ultra cm:"); Console.println(duration/58);

  if (duration < 100) { // Either 0 means timed out, or less than 2cm is out of range as well
    return 1000;   // we will use this large value to mean out of range, since 400 cm is the manufacturer's max range published
  }
  return (duration) / 58;  // this converts microseconds of sound travel time to centimeters. Remember the sound has to go back and forth
  // so it's traveling twice as far as the object's distance
}
#endif

// write out a word, high byte first, and return checksum of two individual bytes
unsigned int
bluewriteword(int w) {
  unsigned int h = highByte(w);
  BlueTooth.write(h);
  unsigned int l = lowByte(w);
  BlueTooth.write(l);
  return h + l;
}

void
sendSensorData() {

#ifdef __ULTRA_SND__
  unsigned int ultra = readUltrasonic(); // this delays us 20 milliseconds but we should still be well within timing constraints
#endif
  //uint16_t blocks = CmuCam5.getBlocks(1); // just return the largest object for now
  int blocks = 0; // comment out cmucam for now

  BlueTooth.print("V");
  BlueTooth.print("1");
  int length = 8;  //+blocks?10:0; // if there is a cmucam block, we need to add 10 more bytes of data
  unsigned int checksum = length;
  BlueTooth.write(length);
  //////////////////for testing only////////////////////////////////
  //int testword = 567; // for testing we will for now hard code the first sensor to a fixed value
  //checksum += bluewriteword(testword);
  //checksum += bluewriteword(testword);
  //checksum += bluewriteword(testword);
  /////////////////////////////////////////////////////////////////
  //checksum += bluewriteword(analogRead(A3));
  checksum += bluewriteword(analogRead(A6));
  checksum += bluewriteword(analogRead(A7));
#ifdef __ULTRA_SND__
  checksum += bluewriteword(ultra);
#endif
  if (blocks > 0) {
    //checksum += bluewriteword(CmuCam5.blocks[0].signature);
    //checksum += bluewriteword(CmuCam5.blocks[0].x);
    //checksum += bluewriteword(CmuCam5.blocks[0].y);
    //checksum += bluewriteword(CmuCam5.blocks[0].width);
    //checksum += bluewriteword(CmuCam5.blocks[0].height);
  }

  checksum = (checksum % 256);
  BlueTooth.write(checksum); // end with checksum of data and length
  //Console.println("Sens");

  startedStanding = millis(); // sensor commands are coming from scratch so suppress sleep mode if this comes in

}

// states for processing incoming bluetooth data

#define P_WAITING_FOR_HEADER      0
#define P_WAITING_FOR_VERSION     1
#define P_WAITING_FOR_LENGTH      2
#define P_READING_DATA            3
#define P_WAITING_FOR_CHECKSUM    4
#define P_SIMPLE_WAITING_FOR_DATA 5

int pulselen = SERVOMIN;

#define MAXPACKETDATA 48
unsigned char packetData[MAXPACKETDATA];
unsigned int packetLength = 0;
unsigned int packetLengthReceived = 0;
int packetState = P_WAITING_FOR_HEADER;

void packetErrorChirp(char c) {
  //beep(70, 8);
#ifdef __DEBUG__
  Console.print(" BTER:"); Console.print(packetState); Console.print(c);
  //Console.print("("); Console.print(c,DEC); Console.print(")");
  Console.print("A"); Console.println(BlueTooth.available());
#endif
  packetState = P_WAITING_FOR_HEADER; // reset to initial state if any error occurs
}

byte lastCmd = 's';
byte priorCmd = 0;
byte mode = MODE_WALK; // default
byte submode = SUBMODE_1;     // standard submode.
byte timingfactor = 1;   // default is full speed. If this is greater than 1 it multiplies the cycle time making the robot slower
short priorDialMode = -1;

#ifdef __GO_BLE__
volatile bool stopBluetooth = false;
int receiveDataHandler() {
  int rc = 0;

  if (Goble.available()) {
    BlueTooth.end();
    rc = stopBluetooth = 1;
    int joystickX, joystickY;
    joystickY = Goble.readJoystickY();
    joystickX = Goble.readJoystickX();

    if (joystickX > 190)
      lastCmd = 'f'; // FORWARD;
    else if (joystickX < 80)
      lastCmd = 'b'; // BACKWARD;
    else if (joystickY > 190)
      lastCmd = 'r'; // RIGHT;
    else if (joystickY < 80)
      lastCmd = 'l'; // LEFT;
    else
      lastCmd = 's';
    //
    if (Goble.readSwitchUp() == PRESSED)
      submode = SUBMODE_1;
    else if (Goble.readSwitchDown() == PRESSED)
      submode = SUBMODE_2;
    else if (Goble.readSwitchLeft() == PRESSED)
      submode = SUBMODE_3;
    else if (Goble.readSwitchRight() == PRESSED)
      submode = SUBMODE_4;;
    //
    if (Goble.readSwitchSelect() == PRESSED) {
      mode = MODE_WALK;
      submode = SUBMODE_1;
      lastCmd = 's';
    } else if (Goble.readSwitchStart() == PRESSED) {
      mode = MODE_DANCE;
      submode = SUBMODE_1;
      lastCmd = 's';
    }
    LastValidReceiveTime = millis();
  }
  return rc;
}

#else //  not __GO_BLE__
int receiveDataHandler() {

  while (BlueTooth.available() > 0) {
    unsigned int c = BlueTooth.read();

    // uncomment the following lines if you're doing some serious packet debugging, but be aware this will take up so
    // much time you will drop some data. I would suggest slowing the gamepad/scratch sending rate to 4 packets per
    // second or slower if you want to use this.
#if 0
    unsigned long m = millis();
    //Console.print(m);
    Console.print("'"); Console.write(c); Console.print("' ("); Console.print((int)c);
    //Console.print(")S="); Console.print(packetState); Console.print(" a="); Console.print(BlueTooth.available()); Console.println("");
    //Console.print(m);
    Console.println("");
#endif

    switch (packetState) {
      case P_WAITING_FOR_HEADER:
        if (c == 'V') {
          packetState = P_WAITING_FOR_VERSION;
          //Console.print("GOT V ");
        } else if (c == '@') {  // simplified mode, makes it easier for people to write simple apps to control the robot
          packetState = P_SIMPLE_WAITING_FOR_DATA;
          packetLengthReceived = 0; // we'll be waiting for exactly 3 bytes like 'D1b' or 'F3s'
          //Console.print("GOT @");
        } else {
          // may as well flush up to the next header
          int flushcount = 0;
          while (BlueTooth.available() > 0 && (BlueTooth.peek() != 'V') && (BlueTooth.peek() != '@')) {
            BlueTooth.read(); // toss up to next possible header start
            flushcount++;
          }
#ifdef __DEBUG__
          Console.print("F:"); Console.print(flushcount);
#endif
          packetErrorChirp(c);
        }
        break;
      case P_WAITING_FOR_VERSION:
        if (c == '1') {
          packetState = P_WAITING_FOR_LENGTH;
          //Console.print("GOT 1 ");
        } else if (c == 'V') {
          // this can actually happen if the checksum was a 'V' and some noise caused a
          // flush up to the checksum's V, that V would be consumed by state WAITING FOR HEADER
          // leaving the real 'V' header in position 2. To avoid an endless loop of this situation
          // we'll simply stay in this state (WAITING FOR VERSION) if we see a 'V' in this state.

          // do nothing here
        } else {
          packetErrorChirp(c);
          packetState = P_WAITING_FOR_HEADER; // go back to looking for a 'V' again
        }
        break;
      case P_WAITING_FOR_LENGTH:
        { // need scope for local variables
          packetLength = c;
          if (packetLength > MAXPACKETDATA) {
            // this can happen if there's either a bug in the gamepad/scratch code, or if a burst of
            // static happened to hit right when the length was being transmitted. In either case, this
            // packet is toast so abandon it.
            packetErrorChirp(c);
#ifdef __DEBUG__
            Console.print("Bad Length="); Console.println(c);
#endif
            packetState = P_WAITING_FOR_HEADER;
            return 0;
          }
          packetLengthReceived = 0;
          packetState = P_READING_DATA;

          //Console.print("L="); Console.println(packetLength);
        }
        break;
      case P_READING_DATA:
        if (packetLengthReceived >= MAXPACKETDATA) {
          // well this should never, ever happen but I'm being paranoid here.
#ifdef __DEBUG__
          Console.println("ERROR: PacketDataLen out of bounds!");
#endif
          packetState = P_WAITING_FOR_HEADER;  // abandon this packet
          packetLengthReceived = 0;
          return 0;
        }
        packetData[packetLengthReceived++] = c;
        if (packetLengthReceived == packetLength) {
          packetState = P_WAITING_FOR_CHECKSUM;
        }
        //Console.print("CHAR("); Console.print(c); Console.print("/"); Console.write(c); Console.println(")");
        break;

      case P_WAITING_FOR_CHECKSUM:

        {
          unsigned int sum = packetLength;  // the length byte is part of the checksum
          for (unsigned int i = 0; i < packetLength; i++) {
            // uncomment the next line if you need to see the packet bytes
            //Console.print(packetData[i]);Console.print("-");
            sum += packetData[i];
          }
          sum = (sum % 256);

          if (sum != c) {
            packetErrorChirp(c);
#ifdef __DEBUG__
            Console.print("CHECKSUM FAIL "); Console.print(sum); Console.print("!="); Console.print((int)c);
            Console.print(" len="); Console.println(packetLength);
#endif
            packetState = P_WAITING_FOR_HEADER;  // giving up on this packet, let's wait for another
          } else {
            LastValidReceiveTime = millis();  // set the time we received a valid packet
            processPacketData();
            packetState = P_WAITING_FOR_HEADER;
            //dumpPacket();   // comment this line out unless you are debugging packet transmissions
            return 1; // new data arrived!
          }
        }
        break;

      case P_SIMPLE_WAITING_FOR_DATA:
        packetData[packetLengthReceived++] = c;
        if (packetLengthReceived == 3) {
          // at this point, we're done no matter whether the packet is good or not
          // so might as well set the new state right up front
          packetState = P_WAITING_FOR_HEADER;

          // this simple mode consists of an at-sign followed by three letters that indicate the
          // button and mode, such as: @W2f means walk mode two forward. As such, there is no
          // checksum, but we can be pretty sure it's valid because there are strong limits on what
          // each letter can be. The following large conditional tests these constraints
          if ( (packetData[0] != 'W' && packetData[0] != 'D' && packetData[0] != 'F') ||
               (packetData[1] != '1' && packetData[1] != '2' && packetData[1] != '3' && packetData[1] != '4') ||
               (packetData[2] != 'f' && packetData[2] != 'b' && packetData[2] != 'l' && packetData[2] != 'r' &&
                packetData[2] != 'w' && packetData[2] != 's')) {

            // packet is bad, just toss it.
            return 0;
          } else {
            // we got a good combo of letters in simplified mode
            processPacketData();
            return 1;
          }
        }
        //Console.print("CHAR("); Console.print(c); Console.print("/"); Console.write(c); Console.println(")");
        break;
    }
  }

  return 0; // no new data arrived
}
#endif // __GO_BLE__

unsigned int LastGgaittype;
unsigned int LastGreverse;
unsigned int LastGhipforward;
unsigned int LastGhipbackward;
unsigned int LastGkneeup;
unsigned int LastGkneedown;
unsigned int LastGtimeperiod;
int LastGleanangle;   // this can be negative so don't make it unsigned

void gait_command(int gaittype, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, int leanangle, int timeperiod) {

  if (ServosDetached) { // wake up any sleeping servos
    attach_all_servos();
  }

  switch (gaittype) {
    case 0:
    default:
      gait_tripod(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 1:
      turn(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 2:
      gait_ripple(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 3:
      gait_sidestep(reverse, timeperiod);
      break;
  }

#if 0
  Console.print("GAIT: style="); Console.print(gaittype); Console.print(" dir="); Console.print(reverse, DEC); Console.print(" angles="); Console.print(hipforward);
  Console.print("/"); Console.print(hipbackward); Console.print("/"); Console.print(kneeup, DEC); Console.print("/"); Console.print(kneedown);
  Console.print("/"); Console.println(leanangle);
#endif

  mode = MODE_GAIT;   // this stops auto-repeat of gamepad mode commands
}

inline void dumpPacket() { // this is purely for debugging, it can cause timing problems so only use it for debugging
#ifdef __DEBUG__
  Console.print("DMP:");
  for (unsigned int i = 0; i < packetLengthReceived; i++) {
    Console.write(packetData[i]); Console.print("("); Console.print(packetData[i]); Console.print(")");
  }
  Console.println("");
#endif
}


void processPacketData() {
  unsigned int i = 0;
  while (i < packetLengthReceived) {
    switch (packetData[i]) {
      case 'W':
      case 'F':
      case 'D':
        // gamepad mode change
        if (i <= packetLengthReceived - 3) {

          mode = packetData[i];
          submode = packetData[i + 1];
          lastCmd = packetData[i + 2];
          //Console.print("GP="); Console.write(mode);Console.write(submode);Console.write(lastCmd);Console.println("");
          i += 3; // length of mode command is 3 bytes
          continue;
        } else {
          // this is an error, we got a command that was not complete
          // so the safest thing to do is toss the entire packet and give an error
          // beep
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.println("PKERR:M:Short");
#endif
          return;  // stop processing because we can't trust this packet anymore
        }
        break;
      case 'B':   // beep
        if (i <= packetLengthReceived - 5) {
          int honkfreq = word(packetData[i + 1], packetData[i + 2]);
          int honkdur = word(packetData[i + 3], packetData[i + 4]);
          // eventually we should queue beeps so scratch can issue multiple tones
          // to be played over time.
          if (honkfreq > 0 && honkdur > 0) {
#ifdef __DEBUG__
            Console.println("Beep Command");
#endif
            beep(honkfreq, honkdur);
          }
          i += 5; // length of beep command is 5 bytes
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.print("PKERR:B:Short:"); Console.print(i); Console.print(":"); Console.println(packetLengthReceived);
#endif
          return;  // toss the rest of the packet
        }
        break;

      case 'R': // Raw Servo Move Command (from Scratch most likely)

#define RAWSERVOPOS 0
#define RAWSERVOADD 1
#define RAWSERVOSUB 2
#define RAWSERVONOMOVE 255
#define RAWSERVODETACH 254
        // Raw servo command is 18 bytes, command R, second byte is type of move, next 16 are all the servo ports positions
        // note: this can move more than just leg servos, it can also access the four ports beyond the legs so
        // you could make active attachments with servo motors, or you could control LED light brightness, etc.
        // move types are: 0=set to position, 1=add to position, 2=subtract from position
        // the 16 bytes of movement data is either a number from 1 to 180 meaning a position, or the
        // number 255 meaning "no move, stay at prior value", or 254 meaning "cut power to servo"
        if (i <= packetLengthReceived - 18) {
          //Console.println("Got Raw Servo with enough bytes left");
          int movetype = packetData[i + 1];
          //Console.print(" Movetype="); Console.println(movetype);
          for (int servo = 0; servo < 16; servo++) {
            int pos = packetData[i + 2 + servo];
            if (pos == RAWSERVONOMOVE) {
              //Console.print("Port "); Console.print(servo); Console.println(" NOMOVE");
              continue;
            }
            if (pos == RAWSERVODETACH) {
              servos[servo]->detach(); //servoDriver.setPin(servo, 0, false); // stop pulses which will quickly detach the servo
              //Console.print("Port "); Console.print(servo); Console.println(" detached");
              continue;
            }
            if (movetype == RAWSERVOADD) {
              pos += ServoPos[servo];
            } else if (movetype == RAWSERVOSUB) {
              pos = ServoPos[servo] - pos;
            }
            pos = constrain(pos, 0, 180);
            //Console.print("Servo "); Console.print(servo); Console.print(" pos "); Console.println(pos);
            ServoPos[servo] = pos;
          }
          checkForCrashingHips();  // make sure the user didn't do something silly
          for (int servo = 0; servo < LEG_DOF; servo++) {
            setServo(servo, ServoPos[servo]);
          }
          i += 18; // length of raw servo move is 18 bytes
          mode = MODE_LEG;  // suppress auto-repeat of gamepad commands when this is in progress
          startedStanding = -1; // don't allow sleep mode while this is running
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.print("PKERR:R:Short:"); Console.print(i); Console.print(":"); Console.println(packetLengthReceived);
#endif
          return;  // toss the rest of the packet
        }
        break;

      case 'G': // Gait command (coming from Scratch most likely). This command is always 10 bytes long
        // params: literal 'G',
        //         Gait type: 0=tripod, 1=turn in place CW from top, 2=ripple, 3=sidestep
        //         reverse direction(0 or 1)
        //         hipforward (angle)
        //         hipbackward (angle),
        //         kneeup (angle)
        //         kneedown(angle)
        //         lean (angle)    make the robot body lean forward or backward during gait, adjusts front and back legs
        //         cycle time (2 byte unsigned long)  length of time a complete gait cycle should take, in milliseconds
        if (i <= packetLengthReceived - 10) {
          LastGgaittype = packetData[i + 1];
          LastGreverse = packetData[i + 2];
          LastGhipforward = packetData[i + 3];
          LastGhipbackward = packetData[i + 4];
          LastGkneeup = packetData[i + 5];
          LastGkneedown = packetData[i + 6];
          int lean = packetData[i + 7];
          LastGtimeperiod = word(packetData[i + 8], packetData[i + 9]);

          LastGleanangle = constrain(lean - 70, -70, 70); // lean comes in from 0 to 60, but we need to bring it down to the range -30 to 30

          gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward, LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);

          i += 10;  // length of command
          startedStanding = -1; // don't sleep the legs during this command
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.println("PKERR:G:Short");
#endif
          return;  // toss the rest of the packet
        }
        break;

      case 'L': // leg motion command (coming from Scratch most likely). This command is always 5 bytes long
        if (i <= packetLengthReceived - 5) {
          unsigned int knee = packetData[i + 2];
          unsigned int hip = packetData[i + 3];
          if (knee == 255) {
            knee = NOMOVE;
#ifdef __DEBUG__
            Console.println("KNEE NOMOVE");
#endif
          }
          if (hip == 255) {
            hip = NOMOVE;
#ifdef __DEBUG__
            Console.println("HIP NOMOVE");
#endif
          }
          unsigned int legmask = packetData[i + 1];
          int raw = packetData[i + 4];
#ifdef __DEBUG__
          Console.print("SETLEG:"); Console.print(legmask, DEC); Console.print("/"); Console.print(knee);
          Console.print("/"); Console.print(hip); Console.print("/"); Console.println(raw, DEC);
#endif
          setLeg(legmask, knee, hip, 0, raw);
          mode = MODE_LEG;   // this stops auto-repeat of gamepad mode commands
          i += 5;  // length of leg command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received
          if (ServosDetached) { // wake up any sleeping servos
            attach_all_servos();
          }
          break;
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.println("PKERR:L:Short");
#endif
          return;  // toss the rest of the packet
        }
        break;

      case 'T': // Trim command

        // The trim command is always just a single operator, either a DPAD button (f, b, l, r, s, w) or the
        // special values S (save), E (erase), P (toggle pose), or R (reset temporarily to untrimmed stance).
        // The meanings are:
        // f    Raise current knee 1 microsecond
        // b    Lower current knee 1 microsecond
        // l    Move current hip clockwise
        // r    Move current hip counterclockwise
        // w    Move to next leg, the leg will twitch to indicate
        // s    Do nothing, just hold steady
        // S    Save all the current trim values
        // P    Toggle the pose between standing and adjust mode
        // R    Show untrimmed stance in the current pose
        // E    Erase all the current trim values
        if (i <= packetLengthReceived - 2) {

          unsigned int command = packetData[i + 1];
#ifdef __DEBUG__
          Console.print("Trim Cmd: "); Console.write(command); Console.println("");
#endif
          i += 2;  // length of trim command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received
          mode = MODE_LEG;
          if (ServosDetached) { // wake up any sleeping servos
            attach_all_servos();
          }

          TrimInEffect = 1;   // by default we'll show trims in effect

          // Interpret the command received
          switch (command) {
            case 'f':
            case 'b':
              ServoTrim[TrimCurLeg + NUM_LEGS] = constrain(ServoTrim[TrimCurLeg + NUM_LEGS] + ((command == 'b') ? -1 : 1), 0, 255);
              beep(300, 30);
              break;

            case 'l':
            case 'r':
              ServoTrim[TrimCurLeg] = constrain(ServoTrim[TrimCurLeg] + ((command == 'r') ? -1 : 1), 0, 255);
              beep(500, 30);
              break;

            case 'w':
              TrimCurLeg = (TrimCurLeg + 1) % NUM_LEGS;
              setKnee(TrimCurLeg, 120);
              beep(100, 30);
              delay(500);  // twitch the leg up to give the user feedback on what the new leg being trimmed is
              // this delay also naturally debounces this command a bit
              break;
            case 'R':
              TrimInEffect = 0;
              beep(100, 30);
              break;
            case 'S':
              save_trims();
              beep(800, 1000);
              delay(500);
              break;
            case 'P':
              TrimPose = 1 - TrimPose;  // toggle between standing (0) and adjust mode (1)
              beep(500, 30);
              break;
            case 'E':
              erase_trims();
              beep(1500, 1000);
              break;

            default:
            case 's':
              // do nothing.
              break;
          }

          // now go ahead and implement the trim settings to display the result
          for (int i = 0; i < NUM_LEGS; i++) {
            setHip(i, HIP_NEUTRAL);
            if (TrimPose == 0) {
              setKnee(i, KNEE_STAND);
            } else {
              setKnee(i, KNEE_NEUTRAL);
            }
          }
          break;
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.println("PKERR:T:Short");
#endif
          return;  // toss the rest of the packet
        }
        break;

      case 'P': // Pose command (from Scratch) sets all 12 robot leg servos in a single command
        // special value of 255 means no change from prior commands, 254 means power down the servo
        // This command is 13 bytes long: "P" then 12 values to set servo positions, in order from servo 0 to 11

        if (ServosDetached) { // wake up any sleeping servos
          attach_all_servos();
        }
        if (i <= packetLengthReceived - 13) {
          for (int servo = 0; servo < LEG_DOF; servo++) {
            unsigned int position = packetData[i + 1 + servo];
            if (position < 0) {
              position = 0;
            } else if (position > 180 && position < 254) {
              position = 180;
            }
            if (position < 254) {
              ServoPos[servo] = position;

              //Console.print("POSE:servo="); Console.print(servo); Console.print(":pos="); Console.println(position);
            } else if (position == 254) {
              // power down this servo
              servos[servo]->detach(); //servoDriver.setPin(servo, 0, false); // stop pulses which will quickly detach the servo
              //Console.print("POSE:servo="); Console.print(servo); Console.println(":DETACHED");
            } else {
              //Console.print("POSE:servo="); Console.print(servo); Console.println(":pos=unchanged");
            }
          }
          checkForCrashingHips();
          for (int servo = 0; servo < LEG_DOF; servo++) {
            setServo(servo, ServoPos[servo]);
          }

          mode = MODE_LEG;   // this stops auto-repeat of gamepad mode commands

          i += 13;  // length of pose command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received

          break;
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
#ifdef __DEBUG__
          Console.println("PKERR:P:Short");
#endif
          return;  // toss the rest of the packet
        }
        break;  // I don't think we can actually get here.

      case 'S':   // sensor request
        // CMUCam seems to require it's own power supply so for now we're not doing that, will get it
        // figured out by the time KS shipping starts.
        i++;  // right now this is a single byte command, later we will take options for which sensors to send
        sendSensorData();
        //////////////// TEMPORARY CODE ////////////////////
        // chirp at most once per second if sending sensor data, this is helpful for debugging
        if (0) {
          unsigned long t = millis() % 1000;
          if (t < 110) {
            beep(2000, 20);
          }
        }
        ////////////////////////////////////////////////////
        break;
      default:
#ifdef __DEBUG__
        Console.print("PKERR:BadSW:"); Console.print(packetData[i]);
        Console.print("i="); Console.print(i); Console.print(" RCV="); Console.println(packetLengthReceived);
#endif
        beep(BF_ERROR, BD_MED);
        return;  // something is wrong, so toss the rest of the packet
    }
  }
}


int flash(unsigned long t) {
  // the following code will return HIGH for t milliseconds
  // followed by LOW for t milliseconds. Useful for flashing
  // the LED on pin 13 without blocking
  return (millis() % (2 * t)) > t;
}

//
// Short power dips can cause the servo driver to put itself to sleep
// the checkForServoSleep() function uses IIC protocol to ask the servo
// driver if it's asleep. If it is, this function wakes it back up.
// You'll see the robot stutter step for about half a second and a chirp
// is output to indicate what happened.
// This happens more often on low battery conditions. When the battery gets low
// enough, however, this code will not be able to wake it up again.
// If your robot constantly resets even though the battery is fully charged, you
// may have too much friction on the leg hinges, or you may have a bad servo that's
// drawing more power than usual. A bad BEC can also cause the issue.
//
unsigned long freqWatchDog = 0;
unsigned long SuppressScamperUntil = 0;  // if we had to wake up the servos, suppress the power hunger scamper mode for a while

inline void checkForServoSleep() {
  /*
    if (millis() > freqWatchDog) {

      // See if the servo driver module went to sleep, probably due to a short power dip
      Wire.beginTransmission(SERVO_IIC_ADDR);
      Wire.write(0);  // address 0 is the MODE1 location of the servo driver, see documentation on the PCA9685 chip for more info
      Wire.endTransmission();
      Wire.requestFrom((uint8_t)SERVO_IIC_ADDR, (uint8_t)1);
      int mode1 = Wire.read();
      if (mode1 & 16) { // the fifth bit up from the bottom is 1 if controller was asleep
        // wake it up!
        resetservoDriver();
        beep(1200, 100); // chirp to warn user of brown out on servo controller
        SuppressScamperUntil = millis() + 10000;  // no scamper for you! (for 10 seconds because we ran out of power, give the battery
        // a bit of time for charge migration and let the servos cool down)
      }
      freqWatchDog = millis() + 100;
    }
  */
}

void checkLegStressSituation() {
  return; // This is experimental and for now we're disabling it by immediately returning

#if 0

  // ok we got new data. Awesome! If it's not the same mode as the old data and would result in the robot
  // attempting to lift off the ground with less than all six legs, then insert a 200 millisecond
  // attempt to get the robot lifted back off the ground. For now we're just hard coding the
  // major cases that would cause this in practice. A better solution would be to have a watchdog
  // that models the robot's ground-to-standing state at a low level in the servo subroutines.
  // We will do that in a future release, but for now this will save a lot of stress on the servos and
  // is easy.

  // first, let's see if all six legs are plausibly on the ground which we'll define as the hips all
  // having been commanded to a standing angle at least 200 milliseconds ago
  long now = millis();
  byte alldown = 1;
  for (int i = 0; i < NUM_LEGS; i++) {
    if ( (ServoTime[i + KNEE_OFFSET] <= now - 200) && ServoPos[i + KNEE_OFFSET] <= KNEE_STAND) {
      continue;  // this leg meets the criteria
    }
    // if we get here we found a leg that's possibly not all the way down
    alldown = 0;
    break;
  }
  if (alldown) {
    return;   // no need to continue, all the legs are down
  }

  // ok, we're in a dangerous situation. Not every leg is down and we're switching to a new mode.
  // for safety, if the new mode doesn't have all the legs down, we're going to insert a short
  // extra move to bring the legs all down to the ground.
  // the modes that don't have all legs down are: W2*, F1*, F2* if not with GRIPARM, D1l, D1r, D3*, D4*
  // while technically some walking modes may not have all the legs down at certain times, only W2 (high step)
  // would have the legs so far off the ground that it would be a major issue.

  if (
    (mode == 'W' && submode == '2' && lastCmd != 's') ||
    (mode == 'F' && (submode == '1' || (submode == '2' && Dialmode != DIALMODE_RC_GRIPARM) )) ||
    (mode == 'D' && (submode == '3' || submode == '4')) ||
    (mode == 'D' && submode == '1' && (lastCmd == 'r' || lastCmd == 'l'))
  ) {
    // if we get here, we do in fact have a danger situation so command all the hips down
    // to a standing position momentarily
    stand();
    delay(200);
  }

#endif
}

void checkForSmoothMoves() {
  // This is kind of a hack right now for making the grip arm move smoothly. Really there should be a
  // general mechanism that would apply to all servos for things like lean and twist mode as well as
  // servos added by the user for use by Scratch. We'll kind of use this hack as a prototype for a
  // more general mechanism to be implemented in a major revision later

  if (abs(ServoPos[GRIPARM_ELBOW_SERVO] - GripArmElbowDestination) <= 1) {
    // uncomment the following line to debug grip elbow movement
    //Console.print("GA close pos="); Console.print(ServoPos[GRIPARM_ELBOW_SERVO]); Console.print(" dest="); Console.println(GripArmElbowDestination);
    return; // we're close enough to the intended destination already
  }

#define SMOOTHINCREMENTTIME 20    // number of milliseconds between interpolated moves
  static long LastSmoothMoveTime = 0;

  long now = millis();
  if (now >= LastSmoothMoveTime + SMOOTHINCREMENTTIME) {
    LastSmoothMoveTime = now;
    //Console.print("Set GAE="); Console.println(ServoPos[GRIPARM_ELBOW_SERVO]+GripArmElbowIncrement);
    deferServoSet = 0;
    setServo(GRIPARM_ELBOW_SERVO, ServoPos[GRIPARM_ELBOW_SERVO] + GripArmElbowIncrement);

  } else {
    //Console.println("not time");
  }
}

unsigned long ReportTime = 0;
unsigned long SuppressModesUntil = 0;

void loop() {

  checkForServoSleep();
  checkForCrashingHips();
  checkForSmoothMoves();

  ////////////////////
  int p = analogRead(DIAL_PIN);
  int factor = 1;

  if (p < 50) {
    Dialmode = DIALMODE_STAND;
  } else if (p < 150) {
    Dialmode = DIALMODE_ADJUST;
  } else if (p < 300) {
    Dialmode = DIALMODE_TEST;
  } else if (p < 750) {
    Dialmode = DIALMODE_DEMO;
  } else {
    Dialmode = DIALMODE_RC;
  }

  // for test
  //Dialmode = DIALMODE_TEST;

  if (Dialmode != priorDialMode && priorDialMode != -1) {
    beep(100 + 100 * Dialmode, 60); // audio feedback that a new mode has been entered
    SuppressModesUntil = millis() + 1000;
  }
  priorDialMode = Dialmode;

  if (millis() < SuppressModesUntil) {
    return;
  }

  //Console.print("Analog0="); Console.println(p);

  if (Dialmode == DIALMODE_STAND) { // STAND STILL MODE
#ifndef __DEBUG__
    digitalWrite(LED_INDICATOR, LOW);  // turn off LED in stand mode
#endif
    delay(250);
    stand();
    // setGrip(90, 90);  // in stand mode set the grip arms to neutral positions
    // in Stand mode we will also dump out all sensor values once per second to aid in debugging hardware issues
    if (millis() > ReportTime) {
      ReportTime = millis() + 1000;
#ifdef __DEBUG__
      Console.println("Stand Mode, Sensors:");
      Console.print(" A3=0");
      Console.print(" A6="); Console.print(analogRead(A6));
      Console.print(" A7="); Console.print(analogRead(A7));
#ifdef __ULTRA_SND__
      Console.print(" Dist="); Console.print(readUltrasonic());
#endif
      Console.println("");
#endif //  __DEBUG__    
    }

  } else if (Dialmode == DIALMODE_ADJUST) {  // Servo adjust mode, put all servos at 90 degrees
#ifndef __DEBUG__
    digitalWrite(LED_INDICATOR, flash(100));  // Flash LED13 rapidly in adjust mode
#endif
    stand_90_degrees();

    if (millis() > ReportTime) {
      ReportTime = millis() + 1000;
#ifdef __DEBUG__
      Console.println("AdjustMode");
#endif
    }

  } else if (Dialmode == DIALMODE_TEST) {   // Test each servo one by one
#ifndef __DEBUG__
    pinMode(LED_INDICATOR, flash(500));      // flash LED moderately fast in servo test mode
#endif
    for (int i = 0; i < 2 * NUM_LEGS + NUM_GRIPSERVOS; i++) {
      p = analogRead(DIAL_PIN);
      if (p > 300 || p < 150) {
        break;
      }
      setServo(i, 140);
      delay(500);
      if (p > 300 || p < 150) {
        break;
      }
      setServo(i, 40);
      delay(500);
      setServo(i, 90);
      delay(100);
#ifdef __DEBUG__
      Console.print("SERVO: "); Console.println(i);
#endif
    }

  } else if (Dialmode == DIALMODE_DEMO) {  // demo mode
#ifndef __DEBUG__
    digitalWrite(LED_INDICATOR, flash(2000));  // flash LED very slowly in demo mode
#endif
    random_gait(timingfactor);
    if (millis() > ReportTime) {
      ReportTime = millis() + 1000;
#ifdef __DEBUG__
      Console.println("Demo Mode");
#endif
    }
    return;

  } else { // bluetooth mode (regardless of whether it's with or without the grip arm)
#ifndef __DEBUG__
    digitalWrite(LED_INDICATOR, HIGH);   // LED is set to steady on in bluetooth mode
#endif
    if (millis() > ReportTime) {
      ReportTime = millis() + 2000;
#ifdef __DEBUG__
      Console.print("RC Mode:");
      Console.print(ServosDetached);
      Console.write(mode);
      Console.write(submode);
      Console.write(lastCmd);
      Console.println("");
#endif
    }
    
    int gotnewdata = receiveDataHandler();  // handle any new incoming data first
    //Console.print(gotnewdata); Console.print(" ");

    // if its been more than 1 second since we got a valid bluetooth command
    // then for safety just stand still.

    if (millis() > LastValidReceiveTime + 1000) {
      if (millis() > LastValidReceiveTime + 15000) {
        // after 15 full seconds of not receiving a valid command, reset the bluetooth connection

#ifdef __DEBUG__
        Console.println("Loss of Signal: not receiving any commands from gamepad.");
#endif

        // Make a three tone chirp to indicate reset
        beep(200, 40); // loss of connection test
        delay(100);
        beep(400, 40);
        delay(100);
        beep(600, 40);
        LastReceiveTime = LastValidReceiveTime = millis();
#ifndef __GO_BLE__
        lastCmd = 's';  // for safety put it in stop of walk mode
        mode = 'W';
        submode = '1';
#endif
      }
      long losstime = millis() - LastValidReceiveTime;
#ifdef __DEBUG__
      //Console.print("LOS "); Console.println(losstime);  // LOS stands for "Loss of Signal"
#endif
#ifndef __GO_BLE__
      return;  // don't repeat commands if we haven't seen valid data in a while
#endif
    }

    if (gotnewdata == 0) {
      // we didn't receive any new instructions so repeat the last command unless it was binary
      // or unless we're in fight adjust mode
      if (lastCmd == -1) {
        //Console.println("REP");
        return;
      }


      // fight submodes 3 and 4 should not be repeated without receiving
      // a packet because otherwise they'll zoom right to the end state instead
      // of giving the user a chance to make fine adjustments to position
      if (mode == MODE_FIGHT && (submode == SUBMODE_3 || submode == SUBMODE_4)) {
        //Console.print("f");
        return;
      }

      // If the griparm in enabled then fight mode 2 really is grip control mode and
      // this mode is incremental in nature so the user can adjust the grip up/down/open/closed
      // bit by bit
      if (Dialmode == DIALMODE_RC_GRIPARM && mode == MODE_FIGHT && (submode == SUBMODE_2)) {
        return;
      }

    } else {
      LastReceiveTime = millis();

      checkLegStressSituation();

    }
    // Leg set mode should also not be repeated
    if (mode == MODE_LEG) {
      //Console.print("l");
      return;
    } else if (mode == MODE_GAIT) {
      // repeat the last Gait command (from scratch typically)
      gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward,
                   LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);
      return;
    }
    //
    // Now we're either repeating the last command, or reading the new bluetooth command
    //
    ScamperTracker -= 1;
    if (ScamperTracker < 0) {
      ScamperTracker = 0;
    } else {
      //Console.println(ScamperTracker);
    }

    switch (lastCmd) {
      case '?': BlueTooth.println("#Vorpal Hexapod");
        break;
      case 'W':
        mode = MODE_WALK;
        break;
      case 'F':
        mode = MODE_FIGHT; startedStanding = -1;
        break;
      case 'D':
        mode = MODE_DANCE; startedStanding = -1;
        break;
      case '1':
      case '2':
      case '3':
      case '4':
        submode = lastCmd;
        break;
      case 'w':  // weapon mode, special depending on mode
        startedStanding = -1;
        switch (mode) {
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, 660 * timingfactor);
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance_dab(timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_WALK: {
              beep(400);
              // stomp in place while beeping horn
              if (submode == SUBMODE_2) { // high step
                factor = 2;
              }
              int cyc = TRIPOD_CYCLE_TIME * factor;
              if (submode == SUBMODE_4) {
                cyc = TRIPOD_CYCLE_TIME / 2; // faster stomp in scamper mode
              }
              gait_tripod(1, 90, 90,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN,
                          cyc);
            }
            break;
          default:     // for any other mode implement a "horn"
            beep(400);
            break;
        }
        break;

      case 'f':  // forward
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(0, 0);
            } else {
              if (submode == SUBMODE_2) { // high step
                factor = 2;
              }
              gait_tripod(1, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                          (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN,
                          TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(NO_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
        }
        break;

      case 'b':  // backward
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(1, 0);
            } else {
              if (submode == SUBMODE_2) {
                factor = 2;
              }
              gait_tripod(0, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                          (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              boogie_woogie(NO_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
        }
        break;

      case 'l': // left
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_2) {
              factor = 2;
            }
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(1, 1);
            } else {
              turn(0, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                   (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                   KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(TRIPOD1_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
        }
        break;

      case 'r':  // right
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_2) {
              factor = 2;
            }
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(0, 1);
            } else {
              turn(1, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                   (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                   KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ,
                   KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(TRIPOD2_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
        }
        break;

      case 's':  // stop and just stand there
        if (startedStanding == -1) {
          startedStanding = millis();
        }
        if (mode == MODE_FIGHT) {
          startedStanding = millis();  // reset in fight mode, never sleep the legs
          fight_mode(lastCmd, submode, 660 * timingfactor);
        } else if (mode == MODE_DANCE && submode == SUBMODE_2) { // ballet
          tiptoes();
        } else if (mode == MODE_DANCE && submode == SUBMODE_4) {
          dance_hands(lastCmd);
        } else {
          //          if (millis() - startedStanding > BATTERYSAVER) {
          //            //Console.print("DET LC=");Console.write(lastCmd); Console.println("");
          //            detach_all_servos();
          //            return;
          //          }
          stand();
        }
        break;

      case 'a': // adjust mode
        stand_90_degrees();
        break;

      default:
#ifdef __DEBUG__
        //Console.print("BAD CHAR:"); Console.write(lastCmd); Console.println("");
#endif
        beep(100, 20);
    }  // end of switch
  }  // end of main if statement

  //
#ifdef __GO_BLE__
  if (stopBluetooth && millis() > LastValidReceiveTime + 100  ) {
    BlueTooth.begin(BLUETOOTH_BAUD);
    stopBluetooth = false;
  }
#endif
}
