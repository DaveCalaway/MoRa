void dht(byte &temperature, byte &humidity) {
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");

  // read without samples.
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(pinDHT11, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err);
    mydata[0] = 0x00;
    mydata[1] = 0x00;
    return;
  }

  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" H");

  mydata[0] = (byte)temperature;
  mydata[1] = (byte)humidity;

  //static uint8_t mydata[] = "Hello world!";
  //static uint8_t mydata[] = {0x19, 0x33, 0x00};
}
