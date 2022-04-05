int LED = 5;
int ON_LED = 2;
void setup() {
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(ON_LED, OUTPUT); 
}

void loop() {
  digitalWrite(LED, HIGH);   
  digitalWrite(ON_LED, HIGH); // Turn the LED off by making the voltage HIGH (it's active low) 
  delay(1000);                      // Wait for a second
  digitalWrite(LED, LOW);  // Turn the LED off by making the voltage LOW 
  digitalWrite(ON_LED, LOW);// Turn the LED on by making the voltage LOW (it's active low) 
  delay(2000);                      // Wait for two seconds
} 
