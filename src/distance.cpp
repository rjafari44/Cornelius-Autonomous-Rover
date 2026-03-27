#include "common.h"

// function to calculate distance to an object, returns an integer
int getDistance() {
  long sum{};         // variable of integer type for storing total duration from each for loop iteration
  int count{};        // variable of integer type for counting number of times for loop runs for averaging
  long duration{};    // variable of long integer type for getting time it takes to and from an object
  long avgDuration{}; // variable of long integer type for getting the average of different durations
  int distance{};     // variable of integer type for storing distance to an object

  for (int i{}; i < 3; i++) {     // run loop three times to get three values to average for more consistency
    digitalWrite(trigPin, LOW);  // send out low trigger pulse
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); // send out high trigger pulse
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);  // send out low trigger pulse

    duration = pulseIn(echoPin, HIGH, 25000); // returns total time it takes to go out and come back in microseconds, also has a added 25 ms timeout

    if (duration > 0) { // if there is a echo time received. 
      sum += duration;
      count++;
    } // other wise keep going if echo is received

    delay(5);
  }

  if (count == 0) { // if no pulses are gathered, return
    return 999;
  }

  avgDuration = (sum / count); // calculate average of three durations
  distance = (avgDuration * 0.034 / 2 + 0.5); // calculate the distance to an object in cm, round to nearest integer

  return distance; // return distance to object
}


// function for gathering average distances of two angles on left side, returns an integer
int lookLeft() {
  int distance1{};   // variable of integer type for first distance at angle one
  int distance2{};   // variable of integer type for second distance at angle two
  int avgDistance{}; // variable of integer type for average distance of two angles

  // first angle
  myServo.write(135); 
  delay(500);
  distance1 = getDistance();

  // second angle
  myServo.write(180);
  delay(500);
  distance2 = getDistance();

  // recenter
  myServo.write(90);
  delay(300);

  avgDistance = (distance1 + distance2)/2; // calculate average distance of two angles

  return avgDistance; // return average distance
}

// function for gathering average distances of two angles on left side, returns an integer
int lookRight() {
    int distance1{};   // variable of integer type for first distance at angle one
    int distance2{};   // variable of integer type for second distance at angle two
    int avgDistance{}; // variable of integer type for average distance of two angles

    // first angle
    myServo.write(45);
    delay(500);
    distance1 = getDistance();

    // second angle
    myServo.write(0);
    delay(500);
    distance2 = getDistance();

    // recenter
    myServo.write(90);
    delay(300);

    avgDistance = (distance1 + distance2)/2; // calculate average distance of two angles

    return avgDistance; // return average distance
}