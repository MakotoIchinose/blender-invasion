uniform int color_type;
uniform int mode;
uniform sampler2D myTexture;

uniform float gradient_f;
uniform vec2 gradient_s;

in vec4 mColor;
in vec2 mTexCoord;
out vec4 fragColor;

#define texture2D texture

#define GPENCIL_MODE_LINE   0
#define GPENCIL_MODE_DOTS   1
#define GPENCIL_MODE_BOX    2

/* keep this list synchronized with list in gpencil_engine.h */
#define GPENCIL_COLOR_SOLID   0
#define GPENCIL_COLOR_TEXTURE 1
#define GPENCIL_COLOR_PATTERN 2

/* Function to check the point inside ellipse */
float checkpoint(vec2 pt, vec2 radius) 
{ 
    float p = (pow(pt.x, 2) / pow(radius.x, 2)) + (pow(pt.y, 2) / pow(radius.y, 2)); 
  
    return p; 
} 

void main()
{
	vec2 centered = mTexCoord - vec2(0.5);
	float dist_squared = dot(centered, centered);
	const float rad_squared = 0.25;
	float ellip = checkpoint(centered, vec2(gradient_s / 2.0));

	if (mode != GPENCIL_MODE_BOX) {
		if (ellip > 1.0) {
			discard;
		}
	}

	vec4 tmp_color = texture2D(myTexture, mTexCoord);

	/* Solid */
	if (color_type == GPENCIL_COLOR_SOLID) {
		fragColor = mColor;
	}
	/* texture */
	if (color_type == GPENCIL_COLOR_TEXTURE) {
		fragColor =  texture2D(myTexture, mTexCoord);
		/* mult both alpha factor to use strength factor with texture */
		fragColor.a = min(fragColor.a * mColor.a, fragColor.a);
	}
	/* pattern */
	if (color_type == GPENCIL_COLOR_PATTERN) {
		vec4 text_color = texture2D(myTexture, mTexCoord);
		fragColor = mColor;
		/* mult both alpha factor to use strength factor with color alpha limit */
		fragColor.a = min(text_color.a * mColor.a, mColor.a);
	}
	
	if ((mode == GPENCIL_MODE_DOTS) && (gradient_f < 1.0)) {
		float dist = length(centered) * 2.0;
		float ex = pow(dist, (-gradient_f * 2.0f));
		float alpha = clamp(1.0 - abs((1.0f - ex) / 10.0f), 0.0f, 1.0f) * ellip;
		fragColor.a = clamp(smoothstep(fragColor.a, 0.0, alpha), 0.01, 1.0);
	}
	
	if(fragColor.a < 0.0035) {
		discard;
	}
}
