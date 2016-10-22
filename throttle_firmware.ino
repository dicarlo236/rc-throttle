#include <Servo.h>
Servo throttle_out;
#define THROTTLE_IN A6
#define LED 10
#define THROTTLE_OUT 9
#define DISABLED 2
#define THROTTLE_MIN_IN 0
#define THROTTLE_MAX_IN 1023
#define THROTTLE_MIN_OUT 1000
#define THROTTLE_MAX_OUT 2000
#define THROTTLE_LOW_ENABLE 200
#define THROTTLE_LOW_SAMPLES 10
#define SAMPLE_PERIOD_MS 10
#define THROTTLE_RANGE_LOW 100
#define THROTTLE_RANGE_HIGH 1000
#define THROTTLE_MAX_CHANGE 500

bool enabled = false;
bool fault = false;
int zero_count = 0;
int fault_code = -1;
char* fault_messages[] = {"THROTTLE OUT OF RANGE: TOO LOW", 
                          "THROTTLE OUT OF RANGE: TOO HIGH", 
                          "THROTTLE RATE TOO HIGH"};
int fault_print_delay_counter = 0;

int flash_code_counter = 0;
int throttle_last = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(10, OUTPUT);
  
}

bool check_faults(int pot_val)
{
  if(pot_val < THROTTLE_RANGE_LOW)
  {
    go_fault(0);
    return false;
  }
  if(pot_val > THROTTLE_RANGE_HIGH)
  {
    go_fault(1);
    return false;
  }
  if(abs(pot_val - throttle_last) > THROTTLE_MAX_CHANGE)
  {
    go_fault(2);
    return false;
  }
  throttle_last = pot_val;
  return true;
}

void go_fault(int code)
{
  throttle_out.detach();
  fault_code = code;
  fault = true;
  enabled = false;
}
void flash_enabled(float percent_throttle)
{
  flash_code_counter++;
  if(flash_code_counter > 1000/percent_throttle) digitalWrite(10, LOW);
  else digitalWrite(10, HIGH);

  if(flash_code_counter > 1250/percent_throttle) flash_code_counter = 0;
  
}

void flash_fault()
{
  flash_code_counter++;
  if(flash_code_counter > 50 && flash_code_counter < 65) digitalWrite(10, HIGH);
  else if(flash_code_counter > 80 && flash_code_counter < 95) digitalWrite(10, HIGH);
  else digitalWrite(10, LOW);
  if(flash_code_counter > 100) flash_code_counter = 0;
  
}

void flash_disabled()
{
  flash_code_counter++;
  if(flash_code_counter < 150) digitalWrite(10, LOW);
  else digitalWrite(10, HIGH);
  if(flash_code_counter > 300) flash_code_counter = 0;
}

void go_enabled()
{
  zero_count = 0;
  fault_code = -1;
  fault = false;
  enabled = true;
  flash_code_counter = 0;
  Serial.print("ENABLING THROTTLE OUTPUT\n");
  throttle_last = analogRead(THROTTLE_IN);
  throttle_out.attach(THROTTLE_OUT);
}

void loop() {
  int pot_val = analogRead(THROTTLE_IN);
  int usec = map(pot_val, THROTTLE_RANGE_LOW, THROTTLE_RANGE_HIGH, THROTTLE_MIN_OUT, THROTTLE_MAX_OUT);
  if (enabled && check_faults(pot_val))
  {
    
    throttle_out.writeMicroseconds(usec);
    flash_enabled(map(usec, THROTTLE_MIN_OUT, THROTTLE_MAX_OUT, 20, 100));
  }
  else
  {
      //not enabled
      throttle_out.writeMicroseconds(DISABLED);
      if(!fault)
      {
        flash_disabled();
        //not enabled, not fault
        if(pot_val < THROTTLE_LOW_ENABLE) zero_count++;
        if(zero_count > THROTTLE_LOW_SAMPLES) go_enabled();
      }
      else
      {
        flash_fault();
        fault_print_delay_counter++;
        if(fault_print_delay_counter > 60)
        {
            //not enabled, fault
          Serial.print("FAULT: ");
          Serial.print(fault_messages[fault_code]);
          Serial.print("\n");
          fault_print_delay_counter = 0;
        }
        
      }
  }
  delay(SAMPLE_PERIOD_MS);

}
