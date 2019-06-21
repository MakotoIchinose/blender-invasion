/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Noise Bases */

float safe_noise(point p, string type)
{
  float f = 0.0;

  /* Perlin noise in range -1..1 */
  if (type == "signed")
    f = noise("perlin", p);

  /* Perlin noise in range 0..1 */
  else
    f = noise(p);

  /* can happen for big coordinates, things even out to 0.5 then anyway */
  if (!isfinite(f))
    return 0.5;

  return f;
}

/* Turbulence */

float noise_turbulence(point p, float details, int hard)
{
  float fscale = 1.0;
  float amp = 1.0;
  float sum = 0.0;
  int i, n;

  float octaves = clamp(details, 0.0, 16.0);
  n = (int)octaves;

  for (i = 0; i <= n; i++) {
    float t = safe_noise(fscale * p, "unsigned");

    if (hard)
      t = fabs(2.0 * t - 1.0);

    sum += t * amp;
    amp *= 0.5;
    fscale *= 2.0;
  }

  float rmd = octaves - floor(octaves);

  if (rmd != 0.0) {
    float t = safe_noise(fscale * p, "unsigned");

    if (hard)
      t = fabs(2.0 * t - 1.0);

    float sum2 = sum + t * amp;

    sum *= ((float)(1 << n) / (float)((1 << (n + 1)) - 1));
    sum2 *= ((float)(1 << (n + 1)) / (float)((1 << (n + 2)) - 1));

    return (1.0 - rmd) * sum + rmd * sum2;
  }
  else {
    sum *= ((float)(1 << n) / (float)((1 << (n + 1)) - 1));
    return sum;
  }
}

/* Utility */

float nonzero(float f, float eps)
{
  float r;

  if (abs(f) < eps)
    r = sign(f) * eps;
  else
    r = f;

  return r;
}
