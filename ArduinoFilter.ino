/*
 * This class is to filter the incoming data from the heart rate monitor and remove unneccesary noise
 */
class DynamicFilter
{
  private:
    int *oldx;
    int *oldy;
    float *a;
    float *b;
    int arrLen;

    
  public:  
  /*
   * Constructor for the Filter
   * Assigns the a and b values, a and b must have 3 values each. a[0] must be 1.
   */
  DynamicFilter(float aValues[],float bValues[],int baSize)
  {    
    arrLen = baSize-1;
    oldx = new int[arrLen];
    oldy = new int[arrLen];
    a = new float[baSize];
    b = new float[baSize];
    
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

  ~DynamicFilter()
  {
    delete[] a;
    delete[] b;
    delete[] oldx;
    delete[] oldy;
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


//    float newy = in*b[0] + oldx[0]*b[1] + oldx[1]*b[2] - oldy[0]*a[1] - oldy[1]*a[2];
//    oldy[1] = oldy[0];
//    oldy[0] = newy;
//    oldx[1] = oldx[0];
//    oldx[0] = in;
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


/*
 * This class is to filter the incoming data from the heart rate monitor and remove unneccesary noise
 */
class Filter
{
  private:
    int y_1,y_2,x_1,x_2;
    float a[3];
    float b[3];

    
  public:  
  /*
   * Constructor for the Filter
   * Assigns the a and b values, a and b must have 3 values each. a[0] must be 1.
   */
  Filter(float aValues[],float bValues[])
  {
    
    a[0] = aValues[0];
    a[1] = aValues[1];
    a[2] = aValues[2];
    b[0] = bValues[0];
    b[1] = bValues[1];
    b[2] = bValues[2];

    y_2 = 0;
    y_1 = 0;
    x_1 = 0;
    x_2 = 0;
    
  }

  /*
   * When a new value is added, it performs the IIR filter
   */
 void addValue(int in)
  {
    float newy = in*b[0] + x_1*b[1] + x_2*b[2] - y_1*a[1] - y_2*a[2];  
    y_2 = y_1;
    y_1 = newy;
    x_2 = x_1;
    x_1 = in;
  }

  /*
   * @return y_1 and y_2 as a string
   */
  String getLastTwoValues()
  {
    return String(y_1) + " " + String(y_2); 
  }

  String getLastValue()
  {
    return String(y_1);
  }
  
};

//keeps track of if the data should be sent or not
bool sendData;

//sends data every other timeout
int oddTimeout;

//stores the reset value for the timer
int timer1_counter;

//the Filter constants
//float a[3] = {1.0f,0.4425,0.1584}; //butter first order 40-80Hz notch filter
//float b[3] = {0.5792,0.4425,0.5792};

//float a[3] = {1.0f,0.4905,0.5095}; //butter first order 50-70Hz notch filter
//float b[3] = {0.7548,0.4905,0.7548};

//float a[3] = {1.0f,0.f,0.f}; //No Filter
//float b[3] = {1.f,0.f,0.f};

float a[] = {1.0f, 1.0212, 1.4128, 0.6396, 0.4128}; //better second order 55-65Hz Notch filter
float b[] = {0.6389, 0.8304, 1.5477, 0.8304, 0.6389};

//Filter notchF(a,b);
DynamicFilter notchF(a,b,5);

//    int x_1=0;
//    int x_2=0;
//    int y_1=0;
//    int y_2=0;
    
void setup() {
  // initialize the serial communication:
  Serial.begin(9600);
  
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT); // Setup for leads off detection LO -
  
  sendData = false;
  oddTimeout = 0;

  //Timer
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  // Set timer1_counter to the correct value for our interrupt interval
  timer1_counter = 65223;   // preload timer 65536-16MHz/256/200Hz
  
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

void loop() {

  //Start sending data if a '1' is sent. Stop sending data if a '0' is sent
  if(Serial.available())
  {
     int incoming = Serial.read();
     if(incoming == 49)//49 is '1'
     { 
      sendData = true;
     }
     else if (incoming == 48)//48 is '0'
     { 
      sendData = false;
     }
     Serial.println(incoming);
  }
  
  //Wait for a bit to keep serial data from saturating
  delay(40);

  if(sendData)
  {
    if((digitalRead(10) == 0) && (digitalRead(11) == 0))
    {
      Serial.println(notchF.getLastTwoValues());
    }
    else
    {
      Serial.println("0 0");
    }
  }
}


// void addValue(int in)
//  {
//    y_2 = y_1;
//    y_1 = in*b[0] + x_1*b[1] + x_2*b[2] - y_1*a[1] - y_2*a[2];  
//    x_2 = x_1;
//    x_1 = in;
//  }



