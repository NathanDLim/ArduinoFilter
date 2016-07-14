/*
 * This class is to filter the incoming data from the heart rate monitor and remove unneccesary noise
 */
class DynamicFilter
{
  private:
    int oldx[5];
    int oldy[5];
    float a[5];
    float b[5];
    int arrLen;

    
  public:  
  /*
   * Constructor for the Filter
   * Assigns the a and b values, a and b must have 3 values each. a[0] must be 1.
   */
  DynamicFilter(float aValues[],float bValues[],int baSize)
  {    
    arrLen = baSize-1;
    if(arrLen > 5)
      arrLen = 5;
    
    for(int i = 0;i<arrLen;i++)
    {
      a[i] = aValues[i];
      b[i] = bValues[i];
      oldx[i] = 0;
      oldy[i] = 0;
    }


    a[arrLen] = aValues[arrLen];
    b[arrLen] = bValues[arrLen];
  }


  /*
   * When a new value is added, it performs the IIR filter
   */
 void addValue(int in)
  {    
    
    float newY = in*b[0];// + oldx[0]*b[1] + oldx[1]*b[2] - oldy[0]*a[1] - oldy[1]*a[2];
    
    for(int i = arrLen;i>=1;i--)
    {
      newY += oldx[i-1]*b[i] - oldy[i-1]*a[i];
    }



    for(int i = arrLen-1;i>=1;i--)
    {
      oldy[i] = oldy[i-1];
      oldx[i] = oldx[i-1];
    }

    oldy[0] = newY;
    oldx[0] = in;

  }

  /*
   * @return y_1 and y_2 as a string
   */
  String getLastTwoValues()
  {
    return String(oldy[0]) + " " + String(oldy[1]); 
  }

  String getLastValue()
  {
    return String(oldy[0]);
  }
  
};



//keeps track of if the data should be sent or not
bool sendData;
//stores the reset value for the timer
int timer1_counter;

//the Filter constants
//float a[3] = {1.0f,0.4425,0.1584}; //butter first order 40-80Hz notch filter
//float b[3] = {0.5792,0.4425,0.5792};

//float a[3] = {1.0f,0.4905,0.5095}; //butter first order 50-70Hz notch filter
//float b[3] = {0.7548,0.4905,0.7548};

//float a[3] = {1.0f,0.f,0.f}; //No Filter
//float b[3] = {1.f,0.f,0.f};

//float a[] = {1.0f, 1.0212, 1.4128, 0.6396, 0.4128}; //butter second order 55-65Hz Notch filter with 200 Hz sampling
//float b[] = {0.6389, 0.8304, 1.5477, 0.8304, 0.6389};

//float a[] = {1.0f, -0.2308, 1.6609, -0.1930, 0.7009}; //butter second order 55-65Hz Notch filter with 250 Hz sampling
//float b[] = {0.8371, -0.2119, 1.6876, -0.2119, 0.8371};

float a[] = {1.0f, -1.412, 1.1228, -0.4081, 0.0632}; //butter second order 40z low filter with 250 Hz sampling
float b[] = {0.0229, 0.0915, 0.1372, 0.0915, 0.0229};


//Filter notchF(a,b);
DynamicFilter notchF(a,b,5);

void setup() {
  // initialize the serial communication:
  Serial.begin(9600);
  
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT); // Setup for leads off detection LO -
  
  sendData = false;
  
  //Timer
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  // Set timer1_counter to the correct value for our interrupt interval
//  timer1_counter = 65223;   // preload timer 65536-16MHz/256/200Hz
  timer1_counter = 65286;   // preload timer 65536-16MHz/256/250Hz
//  timer1_counter = 65380;   // preload timer 65536-16MHz/256/400Hz
  
  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << 2);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  TCNT1 = timer1_counter;   // preload timer
  notchF.addValue(analogRead(A0));
  
}

void loop() 
{
  //Start sending data if a '1' is sent. Stop sending data if a '0' is sent
  if(Serial.available())
  {
     int incoming = Serial.read();
     if(incoming == 49)//49 is '1'
      sendData = true;
     else if (incoming == 48)//48 is '0'
      sendData = false;
  }
  
  //Wait for a bit to keep serial data from saturating
  delay(10);

  if(sendData)
  {
    if((digitalRead(10) == 0) && (digitalRead(11) == 0))
      Serial.println(notchF.getLastValue());
    else
      Serial.println("!");
  }
}



