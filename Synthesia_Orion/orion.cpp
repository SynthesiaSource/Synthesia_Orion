#include "orion.h"
#include "gamma.h"
#include "LPD8806.h"

byte stripBufferA[PIXEL_COUNT];
byte stripBufferB[PIXEL_COUNT];

uint32_t pixelBuffer[PIXEL_COUNT];

int animationStep; // Used for incrementing animations (0-384)
int frameStep;     // Used to increment frame counts.
int mode;          // System mode
int speed;         // System animation speed control
int brightness;    // System brightness control

LPD8806 strip = LPD8806(PIXEL_COUNT);

void stepMode(void) {
  mode++;
  delay(100);
   
  if(mode > NUMBER_OF_MODES)
    mode = 0;
     
  frameStep = 0;
  animationStep = 0;
} // stepMode()

void stepSpeed(void) {
   speed++;
  delay(100);

   if(speed > NUMBER_SPEED_SETTINGS)
     speed = 0;
} // stepSpeed()

void stepBrightness(void) {
   brightness++;
   delay(100);
     
   if(brightness > NUMBER_BRIGHTNESS_LEVELS)
     brightness = 1;
} // stepBrightness()

void enable(boolean setBegun) {
  strip.enable(setBegun);
} // enable()


void disable(void) {
  strip.disable();
}


boolean isEnabled(void) {
  return strip.isEnabled();
} // isEnabled()


boolean isDisabled(void) {
  return strip.isDisabled();
} // isDisabled()


// Set pixel at globalBrightness
void setPixelAtBrightness(int i, uint32_t c)
{
   int  r, g, b;
  
//  g = ((c >> 16) & 0x7f)*(brightnessFraction);
//  r = ((c >>  8) & 0x7f)*(brightnessFraction);
//  b = (c        & 0x7f)*(brightnessFraction); 

  // Extract RGB channels
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
   // Fast bitshift calculation
   // Using floating point calulations in this constantly called method
   // results in really low frame rates
   switch(brightness) 
  {
    case 1:
    {
      break;
    }
    case 2:
    {
      g = (g << 1)+g;
      r = (r << 1)+r;
      b = (b << 1)+b;
      
      g >>= 2;
      r >>= 2;
      b >>= 2;   
      break;
    }
    case 3:
    { 
      g >>= 1;
      r >>= 1;
      b >>= 1;
      break;
    }
    case 4:
    {
      g >>= 2;
      r >>= 2;
      b >>= 2;   
      break;
    }
    case 5:
    {
      g >>= 3;
      r >>= 3;
      b >>= 3;   
      break;
    }
     default: break;
  }
  
  strip.setPixelColor(i, r, g, b); 
}


void setupOrion() {
  for(int u = 0; u<PIXEL_COUNT; u++)
    pixelBuffer[u] = 0;
  
  // globalSpeed controls the delays in the animations. Starts low. Range is 1-5. 
  // Each animation is responsible for calibrating its own speed relative to the globalSpeed.
  speed = 0;

  animationStep = 0;
  frameStep = 0;
  mode = 0;
  speed = 0;
  brightness = 1;
} // setupOrion()


// All animations are controlled by a delay method. Range of delay is 0-5;
// All animations must be totally non-blocking. That is, draw only one frame at a time.
void updateOrion() {
    
  // Used to store a current color for modes which cycle through colors.
  static uint32_t currentColor;

  // This is used to calibrate the speed range for different modes
  // Slow modes require a low frameDelayTimer (1-5). Fast modes require a high frameDelayTimer (5+).
  static int frameDelayTimer = 5;
  static long previousMillis = 0;
  unsigned long currentMillis = millis();

  // If insufficient time has elapsed since the last call just return and do nothing.
  if(currentMillis - previousMillis < frameDelayTimer*speed)    
    return;
    
  previousMillis = currentMillis;

  switch(mode) {
    case 0:
      rainbow(); // Smooth rainbow animation.
      frameDelayTimer = 1;
      break;
    case 1:
      rainbowBreathing(10);
      frameDelayTimer = 1;
      break;
    case 2:
      plasma();
      frameDelayTimer = 20;    
      break;
    case 3:
      splitColorBuilder();
      frameDelayTimer = 5;
      break;
    case 4:
      smoothColors();
      frameDelayTimer = 5;
      break;
    case 5:
      // Single pixel random color pixel chase.
      if(animationStep == 0)
        currentColor =Wheel(random(0, 384));
      colorChase(currentColor, speed);
      frameDelayTimer = 5;    
      break;
    case 6:
      // Random color wipe.
      if(frameStep == 0)
        currentColor = Wheel(random(0, 384));
      colorWipe(currentColor, speed);
      frameDelayTimer = 5;    
      break;
    case 7:
      // Random color dither. This is a color to color dither (does not clear between colors).
      {
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        dither(c, speed);
      }  
      frameDelayTimer = 8;    
      break;
    case 8:
      if(animationStep == 0)
        currentColor = Wheel(random(0, 384));
      scanner(currentColor, speed);  
      frameDelayTimer = 5;    
      break;
    case 9:
      // Sin wave effect. New color every cycle.
      if(animationStep == 0)
        currentColor = Wheel(random(0, 384));
      wave(currentColor, speed);  
      frameDelayTimer = 5;    
      break;
    case 10:
      // Random color noise animation.
      randomSparkle(speed);
      frameDelayTimer = 3;    
      break;
    case 11:
      // Color fade-in fade-out effect
      if(animationStep==0)
        currentColor =Wheel(random(0, 384));
      
      if(animationStep<192)
        fadeIn(currentColor, 10); 
      else
        fadeOut(currentColor, 10);
    
      frameDelayTimer = 1;
      break;
    case 12:
      sparkler();
      frameDelayTimer = 10;
      break;
    default:
      ; // This should never happen. 
  } // switch()
  
  // Global animation frame limit of 384 (for full color wheel range).
  // Large animationSteps slow down the driver.
  animationStep++;
  if(animationStep>384)
    animationStep = 0;
  
  // Ensure that only as many pixels are drawn as there are in the strip.
  frameStep++;
  if(frameStep > PIXEL_COUNT)
    frameStep = 0;
} // updateOrion()


void solidColor()
{
    for (int i=0; i < strip.numPixels(); i++) {
     strip.setPixelColor(i, strip.Color(127, 127, 127));
    }
    strip.show();

}

void plasma() {
  
    double time = animationStep;
            
    for(int y = 0; y < PIXEL_COUNT; y++)
    {
        double value = sin(dist(frameStep + time, y, 64.0, 64.0) / 4.0)
                   + sin(dist(frameStep, y, 32.0, 32.0) / 4.0);
                   + sin(dist(frameStep, y + time / 7, 95.0, 32) / 3.5);
                   + sin(dist(frameStep, y, 95.0, 50.0) / 4.0);
  
      int color = int((4 + value)*384)%384;
      setPixelAtBrightness(y, Wheel(color));
      //strip.setPixelColor(y, Wheel(color)); 
    }    
  strip.show();
}

void sparkler() {
  
  stripBufferA[random(0,PIXEL_COUNT)] = random(0, 255);

  for(int x = 0; x < PIXEL_COUNT; x++) 
    {
      byte newPoint = (stripBufferA[x] + stripBufferA[x+1]) / 2 - 15;
      stripBufferB[x] = newPoint;
      if(newPoint>50)
        setPixelAtBrightness(x, Wheel(((newPoint/5)+animationStep)%384));
//         strip.setPixelColor(x, Wheel(((newPoint/5)+animationStep)%384));      
      if(newPoint<50)
        setPixelAtBrightness(x, strip.Color(0, 0, 0));
//         strip.setPixelColor(x, strip.Color(0,0,0)); 
    }
   
   strip.show();
    
    for(int x = 0; x < PIXEL_COUNT; x++) 
    {
      stripBufferA[x] = stripBufferB[x];
   }
  
}


void rainbowBreathing(uint16_t wait)
{
  static int shifter = 0;
  uint16_t i, j;
  if(animationStep<192)
  {
  float modifier = 0.25+0.004*(float)animationStep;

    for (i=0; i < strip.numPixels(); i++) 
      {
        
      uint32_t c =  Wheel(((i * 384 / PIXEL_COUNT)) % 384);
 
      byte  r, g, b;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      setPixelAtBrightness(i, strip.Color(r*modifier, g*modifier, b*modifier));
      //strip.setPixelColor(i, r*modifier, g*modifier, b*modifier);
//      strip.setPixelColor(i, gamma(r*modifier), gamma(g*modifier), gamma(b*modifier));
    }
  strip.show();   // write all the pixels out
  } else {
  float modifier = 1.75-0.004*(float)animationStep;

    for (i=0; i < strip.numPixels(); i++) 
      {
        
      uint32_t c =  Wheel(((i * 384 / PIXEL_COUNT)+shifter) % 384);
 
      byte  r, g, b;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      setPixelAtBrightness(i, strip.Color( r*modifier, g*modifier, b*modifier));

     // strip.setPixelColor(i, r*modifier, g*modifier, b*modifier);
//      strip.setPixelColor(i, gamma(r*modifier), gamma(g*modifier), gamma(b*modifier));
    }
  strip.show();   // write all the pixels out   
  }
} 
  
  
void rainbow() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();

      for (i=0; i < strip.numPixels(); i++) 
      {
        //strip.setPixelColor(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
        setPixelAtBrightness(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
      }
      strip.show();   // write all the pixels out
}

void splitColorBuilder() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();
//  pixelBuffer[0] = Wheel((sin(animationStep)+1)*384);
//  pixelBuffer[1] = Wheel((sin(animationStep)+1)*384);
  
  uint32_t c = Wheel(animationStep);
  float y = (sin(PI * 1 * (float)(animationStep) / (float)strip.numPixels()/4))+1;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  
  r2 = 127 - (byte)((float)(127 - r) * y);
  g2 = 127 - (byte)((float)(127 - g) * y);
  b2 = 127 - (byte)((float)(127 - b) * y);
  
  pixelBuffer[0] = strip.Color(r2, g2, b2);

  for (i=0; i < strip.numPixels()/2; i++) 
  {
    setPixelAtBrightness(PIXEL_COUNT/2-i, pixelBuffer[i]);
    setPixelAtBrightness(PIXEL_COUNT/2+i, pixelBuffer[i]); 
  }
  
  strip.show();   // write all the pixels out
  
  for(int k = PIXEL_COUNT-1; k>0; k--)
  {
   pixelBuffer[k] = pixelBuffer[k-1]; 
  }


}

//void splitColorBuilder() {
//  uint16_t i, j;
//  int pixelCount = strip.numPixels();
////  pixelBuffer[0] = Wheel((sin(animationStep)+1)*384);
////  pixelBuffer[1] = Wheel((sin(animationStep)+1)*384);
//  
//  uint32_t c = Wheel(animationStep);
//  float y = (sin(PI * 1 * (float)(animationStep) / (float)strip.numPixels()/4))+1;
//  byte  r, g, b, r2, g2, b2;
//
//  // Need to decompose color into its r, g, b elements
//  g = (c >> 16) & 0x7f;
//  r = (c >>  8) & 0x7f;
//  b =  c        & 0x7f; 
//  
//  r2 = 127 - (byte)((float)(127 - r) * y);
//  g2 = 127 - (byte)((float)(127 - g) * y);
//  b2 = 127 - (byte)((float)(127 - b) * y);
//  
//  pixelBuffer[0] = strip.Color(r2, g2, b2);
//
//  for (i=0; i < strip.numPixels()/2; i++) 
//  {
//    setPixelAtBrightness(PIXEL_COUNT/2-i, pixelBuffer[i]);
//    setPixelAtBrightness(PIXEL_COUNT/2+i, pixelBuffer[i]); 
//  }
//  
//  strip.show();   // write all the pixels out
//  
//  for(int k = PIXEL_COUNT-1; k>0; k--)
//  {
//   pixelBuffer[k] = pixelBuffer[k-1]; 
//  }
//
//
//}

void smoothColors() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();
  uint32_t c = Wheel(((i * 384 / pixelCount) + animationStep) % 384);
  pixelBuffer[0] = c;
  
      for (i=0; i < strip.numPixels(); i++) 
      {
        //strip.setPixelColor(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
        setPixelAtBrightness(i, c);
        
      }
      strip.show();   // write all the pixels out
}

void fadeOut(uint32_t c, uint16_t wait)
{  

  byte  r, g, b, r2, g2, b2, r8, g8,b8;
  
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
 
  r8 = r;
  g8 = g;
  b8 = b;

  byte highColorByte = 0;
  if(g8>highColorByte)
  {highColorByte= g8; }  
  if(r8>highColorByte)
  {highColorByte= r8; }
  if(b8>highColorByte)
  {highColorByte= b8; }  

  byte lowColorByte = 255;
  if(g8 < lowColorByte)
  {lowColorByte= g8; }  
  if(r8 < lowColorByte)
  {lowColorByte= r8; }
  if(b8 < lowColorByte)
  {lowColorByte= b8; }   
  
  int y = (float)highColorByte-(float)highColorByte*(float)(2-0.005*animationStep);

//  int y = (float)highColorByte-(float)highColorByte*(2.0-(0.005*(float)animationStep));

      if(g8>y)
      {
        g2 =  gamma(g8-y);
      } else {
        g2 = gamma(0); 
      }
      if(r8>y)
      {
        r2 =  gamma(r8-y);
      } else {
        r2 = gamma(0); 
      }
      if(b8>y)
      {
        b2 =  gamma(b8-y);
      } else {
        b2 = gamma(0); 
      }
      
    for (int i=0; i < strip.numPixels(); i++) 
    {
      //strip.setPixelColor(i, r2, g2, b2);
      setPixelAtBrightness(i, strip.Color(r2, g2, b2));  
    }
    
    strip.show();   // write all the pixels out
}

void fadeIn(uint32_t c, uint16_t wait)
{
  byte  r, g, b, r2, g2, b2, r8, g8,b8;
  
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
 
  r8 = r;
  g8 = g;
  b8 = b;

  byte highColorByte = 0;
  if(g8>highColorByte)
  {highColorByte= g8; }  
  if(r8>highColorByte)
  {highColorByte= r8; }
  if(b8>highColorByte)
  {highColorByte= b8; }  

  byte lowColorByte = 255;
  if(g8 < lowColorByte)
  {lowColorByte= g8; }  
  if(r8 < lowColorByte)
  {lowColorByte= r8; }
  if(b8 < lowColorByte)
  {lowColorByte= b8; }   

  int y = (float)highColorByte - (float)highColorByte*((float)animationStep/192);

      if(g8>y)
      {
        g2 =  gamma(g8-y);
      } else {
        g2 = gamma(0); 
      }
      if(r8>y)
      {
        r2 =  gamma(r8-y);
      } else {
        r2 = gamma(0); 
      }
      if(b8>y)
      {
        b2 =  gamma(b8-y);
      } else {
        b2 = gamma(0); 
      }
    for (int i=0; i < strip.numPixels(); i++) 
    {
      setPixelAtBrightness(i, strip.Color(r2, g2, b2));
//      strip.setPixelColor(i, r2, g2, b2);
    }
    
    strip.show();   // write all the pixels out
}


void pulseStrobe(uint32_t c, uint16_t wait)
{
    for (int i=0; i < strip.numPixels(); i++) 
    {
      if(animationStep%2)
        {
          setPixelAtBrightness(i, dampenBrightness(c, animationStep));
//        strip.setPixelColor(i, dampenBrightness(c, animationStep));
        } else {
          setPixelAtBrightness(i, dampenBrightness(c,10));
//        strip.setPixelColor(i, dampenBrightness(c, 10)); 
        }
    
    strip.show();   // write all the pixels out  
    }
}


// Cycle through the color wheel, equally spaced around the belt
void rainbowCycle(uint16_t wait) {
  uint16_t i, j;
  for (i=0; i < strip.numPixels(); i++) 
  {
    setPixelAtBrightness(i, Wheel(((i * 384 / strip.numPixels()) + animationStep) % 384));
//    strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + animationStep) % 384));
  }
  strip.show();   // write all the pixels out
  delay(wait);
  animationStep++;
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint16_t wait) {
  int i;
 
  for (i=0; i < frameStep; i++) 
    {
    setPixelAtBrightness(i, c);
    }
   
    strip.show(); 
}


// Chase a dot down the strip
// Random color for each chase
void colorChase(uint32_t c, uint16_t wait) {
  int i;

    setPixelAtBrightness(frameStep-1, 0); // Erase pixel, but don't refresh!
    setPixelAtBrightness(frameStep, c); // Set new pixel 'on'
    strip.show(); // Refresh LED states
}


// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither(uint32_t c, uint16_t wait) {
 
  // Determine highest bit needed to represent pixel index
  int hiBit = 0;
  int n = strip.numPixels() - 1;
  for(int bit=1; bit < 0x8000; bit <<= 1) {
    if(n & bit) hiBit = bit;
  }

  int bit, reverse;
  for(int i=0; i<(hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for(bit=1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    setPixelAtBrightness(reverse, c);
    //strip.setPixelColor(reverse, c);
    strip.show();
    delay(wait);
  }
  delay(wait);  
}


void randomSparkle(uint16_t wait) {

  long randNumber;
  
  // Determine highest bit needed to represent pixel index
  uint16_t i, j;

  randNumber = random(0, strip.numPixels()-1);
  setPixelAtBrightness(randNumber, Wheel(random(0,384)));
  //strip.setPixelColor(randNumber, Wheel(((frameStep * 384 / strip.numPixels()) + animationStep) % 384));
  strip.show();


}

void canada2() {

  int i, j, pos, dir;

  if(frameStep == 0)
  {
    pos = 0;
    dir = 2;
  }
  
  pos = frameStep*2;
  
   if(pos >= strip.numPixels()) 
   {
     pos = 32-((frameStep-16)*2);
    }
  
 // pos = frameStep;
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 2, strip.Color(127, 127, 127));
    strip.setPixelColor(pos - 1, strip.Color(127, 127, 127));
    strip.setPixelColor(pos, strip.Color(127, 127, 127));
    strip.setPixelColor(pos + 1, strip.Color(127, 127, 127));
    strip.setPixelColor(pos + 2, strip.Color(127, 127, 127));
   
   

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(127,0,0));
        
    strip.setPixelColor(pos - 1, strip.Color(127, 127, 127));
    strip.setPixelColor(pos - 0, strip.Color(127, 127, 127));
    strip.setPixelColor(pos+1, strip.Color(127, 127, 127));
    strip.setPixelColor(pos + 2, strip.Color(127, 127, 127));
    strip.setPixelColor(pos + 3, strip.Color(127, 127, 127));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-1; j<= 3; j++) 
        strip.setPixelColor(pos+j, strip.Color(127,0,0));
}
void canada() {

  int i, j, pos, dir;

  if(frameStep == 0)
  {
    pos = 0;
    dir = 2;
  }
  
  pos = frameStep*2;
  
   if(pos >= strip.numPixels()) 
   {
     pos = 32-((frameStep-16)*2);
    }
  
 // pos = frameStep;
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 4, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 3, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 2, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 1, strip.Color(127, 0, 0));
    strip.setPixelColor(pos, strip.Color(127, 0, 0));
    strip.setPixelColor(pos + 1, strip.Color(127, 0, 0));
    strip.setPixelColor(pos + 2, strip.Color(127, 0, 0));
     strip.setPixelColor(pos + 3, strip.Color(127, 0, 0));
    strip.setPixelColor(pos + 4, strip.Color(127, 0, 0));
   
   

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-4; j<= 4; j++) 
        strip.setPixelColor(pos+j, strip.Color(127,127,127));
        
            strip.setPixelColor(pos - 3, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 2, strip.Color(127, 0, 0));

    strip.setPixelColor(pos - 1, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 0, strip.Color(127, 0, 0));
    strip.setPixelColor(pos+1, strip.Color(127, 0, 0));
    strip.setPixelColor(pos + 2, strip.Color(127, 0, 0));
    strip.setPixelColor(pos + 3, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 4, strip.Color(127, 0, 0));
    strip.setPixelColor(pos - 5, strip.Color(127, 0, 0));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-3; j<= 5; j++) 
        strip.setPixelColor(pos+j, strip.Color(127,127,127));
}

// "Larson scanner" = Cylon/KITT bouncing light effect
// Draws two sets of frames per call to double the resolution range
void scanner(uint32_t c, uint16_t wait) {

  byte  r, g, b;
  
  // Decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  
  int i, j, pos, dir;

  if(frameStep == 0)
  {
    pos = 0;
    dir = 2;
  }
  
  pos = frameStep*2;
  
   if(pos >= strip.numPixels()) 
   {
     pos = 32-((frameStep-16)*2);
    }
  
 // pos = frameStep;
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    
    setPixelAtBrightness(pos - 2, strip.Color(r/4, g/4, b/4));
    setPixelAtBrightness(pos - 1, strip.Color(r/2, g/2, b/2));
    setPixelAtBrightness(pos, strip.Color(r, g, b));
    setPixelAtBrightness(pos + 1, strip.Color(r/2, g/2, b/2));
    setPixelAtBrightness(pos + 2, strip.Color(r/4, g/4, b/4));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
        
    setPixelAtBrightness(pos - 1, strip.Color(r/4, g/4, b/4));
    setPixelAtBrightness(pos - 0, strip.Color(r/2, g/2, b/2));
    setPixelAtBrightness(pos+1, strip.Color(r, g, b));
    setPixelAtBrightness(pos + 2, strip.Color(r/2, g/2, b/2));
    setPixelAtBrightness(pos + 3, strip.Color(r/4, g/4, b/4));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-1; j<= 3; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
}

// Sine wave effect.
// Self calibrating for pixel run length.
void wave(uint32_t c, uint16_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

    for(int i=0; i<strip.numPixels(); i++) 
    {
      y = sin(PI * 1 * (float)(animationStep + i) / (float)strip.numPixels());
      if(y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      setPixelAtBrightness(i, strip.Color(r2, g2, b2));
//      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
}


//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g - b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128; // red down
      g = WheelPos % 128;       // green up
      b = 0;                    // blue off
      break;
    case 1:
      g = 127 - WheelPos % 128; // green down
      b = WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      b = 127 - WheelPos % 128; // blue down
      r = WheelPos % 128;       // red up
      g = 0;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}


uint32_t dampenBrightness(uint32_t c, int brightness) {

 byte  r, g, b;
  
  g = ((c >> 16) & 0x7f)/brightness;
  r = ((c >>  8) & 0x7f)/brightness;
  b = (c        & 0x7f)/brightness; 

  return(strip.Color(r,g,b));

}



