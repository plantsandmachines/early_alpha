extern DHT dht;

float get_airHumid ()
{
  return dht.readHumidity();
}

float get_airTemp ()
{
  return dht.readTemperature();
}
