// Easing functions for animation

#define PI 3.14159265359

// ------------ In ------------

float easeInCubic(float x) 
{
    return x * x * x;
}

float easeInSine(float x) 
{
  return 1 - cos((x * PI) / 2);
}

// ------------ In-Out ------------

float easeInOutQuart(float t) 
{
  float p = 2.0 * t * t;
  return t < 0.5 ? p : -p + (4.0 * t) - 1.0;
}