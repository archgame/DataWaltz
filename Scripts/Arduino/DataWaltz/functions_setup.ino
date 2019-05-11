//WIFI CONNECT
void connect(const char *subscription) {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "288b70b1", "12b466c6da113704")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");  

  client.subscribe(subscription);
}
