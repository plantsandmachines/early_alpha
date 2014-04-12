extern const int distPin;


long microsecondsToCentimeters(long microseconds)
{
  // speed of sound is 340 m/s or 29 microseconds per centimeter.
  return microseconds / 29 / 2;
}


long get_distance ()
{
  long duration;
 
  cli(); // disable global interrupts to get an acurate reading 

  // PING is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse
  
  pinMode(distPin, OUTPUT);
  digitalWrite(distPin, LOW);
  delayMicroseconds(2);
  digitalWrite(distPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(distPin, LOW);
 
  // how long does it take till the ping retruns
  
  pinMode(distPin, INPUT);
  duration = pulseIn(distPin, HIGH);

  sei(); // enable global interrupts again
 
  // convert the time into a distance
  return microsecondsToCentimeters(duration);
}

