#version 460 core

// Ground truth-based ambient occlusion
// Implementation based on:
// Practical Realtime Strategies for Accurate Indirect Occlusion, Siggraph 2016
// Jorge Jimenez, Xianchun Wu, Angelo Pesce, Adrian Jarabo

// Implementation by /u/Kvaleya
// 2018-08-11

const float PI = 3.141592653589;
const float PIHalf = 1.570796326794;

// Depth buffer
uniform sampler2D gDepth;

uniform float far;
uniform float near;

uniform int samples = 4;
uniform float limit = 100;
uniform float radius = 4.0;
uniform float falloff = 7.5;
uniform float thicknessMix = 0.2;
uniform float maxStride = 8;

// Used to get vector from camera to pixel
float aspect = 1.0;

// The normalized coordinates of the current pixel, range 0.0 .. 1.0
in vec2 coord;

out vec4 fragColor;

float LinearizeDepth(float d)
{
    float z = 2.0 * d - 1.0;
    return 2.0 * near * far / (far + near - z * (far - near));
}

// [Eberly2014] GPGPU Programming for Games and Science
float GTAOFastAcos(float x)
{
	float res = -0.156583 * abs(x) + PIHalf;
	res *= sqrt(1.0 - abs(x));
	return x >= 0 ? res : PI - res;
}

float IntegrateArc(float h1, float h2, float n)
{
	float cosN = cos(n);
	float sinN = sin(n);
	return 0.25 * (-cos(2.0 * h1 - n) + cosN + 2.0 * h1 * sinN - cos(2.0 * h2 - n) + cosN + 2.0 * h2 * sinN);
}

vec3 Visualize_0_3(float x)
{
	const vec3 color0 = vec3(1.0, 0.0, 0.0);
	const vec3 color1 = vec3(1.0, 1.0, 0.0);
	const vec3 color2 = vec3(0.0, 1.0, 0.0);
	const vec3 color3 = vec3(0.0, 1.0, 1.0);

	vec3 color = mix(color0, color1, clamp(x - 0.0, 0.0, 1.0));
         color = mix(color,  color2, clamp(x - 1.0, 0.0, 1.0));
         color = mix(color,  color3, clamp(x - 2.0, 0.0, 1.0));
	
    return color;
}

vec3 GetCameraVec(vec2 uv)
{	
	// Returns the vector from camera to the specified position on the camera plane (uv argument), located one unit away from the camera
	// This vector is not normalized.
	// The nice thing about this setup is that the returned vector from this function can be simply multiplied with the linear depth to get pixel's position relative to camera position.
	// This particular function does not account for camera rotation or position or FOV at all (since we don't need it for AO)
	// TODO: AO is dependent on FOV, this function is not!
	// The outcome of using this simplified function is that the effective AO range is larger when using larger FOV
	// Use something more accurate to get proper FOV-independent world-space range, however you will likely also have to adjust the SSAO constants below
	return vec3(uv.x * -2.0 + 1.0, uv.y * 2.0 * aspect - aspect, 1.0);
}

void SliceSample(vec2 texCoordsBase, vec2 aoDir, int i, float targetMip, vec3 ray, vec3 v, inout float closest)
{
	vec2 uv = texCoordsBase + aoDir * i;
	float depth = LinearizeDepth(textureLod(gDepth, uv, targetMip).x);
	// Vector from current pixel to current slice sample
	vec3 p = GetCameraVec(uv) * depth - ray;
	// Cosine of the horizon angle of the current sample
	float current = dot(v, normalize(p));
	// Linear falloff for samples that are too far away from current pixel
	float falloff = clamp((radius - length(p)) / falloff, 0.0, 1.0);
	if(current > closest)
		closest = mix(closest, current, falloff);
	// Helps avoid overdarkening from thin objects
	closest = mix(closest, current, thicknessMix * falloff);
}

void main()
{
    vec2 depthSize = textureSize(gDepth, 0);
    vec2 texelSize = 1.0 / depthSize;

    aspect = depthSize.x / depthSize.y;

	vec2 texCoords = coord;
	
	// Depth of the current pixel
	float dhere = LinearizeDepth(textureLod(gDepth, texCoords, 0.0).x);
	// Vector from camera to the current pixel's position
	vec3 ray = GetCameraVec(texCoords) * dhere;
	
	const float normalSampleDist = 1.0;
	
	// Calculate normal from the 4 neighbourhood pixels
	vec2 uv = texCoords + vec2(texelSize.x * normalSampleDist, 0.0);
	vec3 p1 = ray - GetCameraVec(uv) * LinearizeDepth(textureLod(gDepth, uv, 0.0).x);
	
	uv = texCoords + vec2(0.0, texelSize.y * normalSampleDist);
	vec3 p2 = ray - GetCameraVec(uv) * LinearizeDepth(textureLod(gDepth, uv, 0.0).x);
	
	uv = texCoords + vec2(-texelSize.x * normalSampleDist, 0.0);
	vec3 p3 = ray - GetCameraVec(uv) * LinearizeDepth(textureLod(gDepth, uv, 0.0).x);
	
	uv = texCoords + vec2(0.0, -texelSize.y * normalSampleDist);
	vec3 p4 = ray - GetCameraVec(uv) * LinearizeDepth(textureLod(gDepth, uv, 0.0).x);
	
	vec3 normal1 = normalize(cross(p1, p2));
	vec3 normal2 = normalize(cross(p3, p4));
	
	vec3 normal = normalize(normal1 + normal2);
	
	// Calculate the distance between samples (direction vector scale) so that the world space AO radius remains constant but also clamp to avoid cache trashing
	// texelSize = vec2(1.0 / sreenWidth, 1.0 / screenHeight)
	float stride = min((1.0 / length(ray)) * limit, maxStride);
	vec2 dirMult = texelSize.xy * stride;
	// Get the view vector (normalized vector from pixel to camera)
	vec3 v = normalize(-ray);
	
	// Calculate slice direction from pixel's position
	float dirAngle = (PI / 16.0) * (((int(gl_FragCoord.x) + int(gl_FragCoord.y) & 3) << 2) + (int(gl_FragCoord.x) & 3));
	vec2 aoDir = dirMult * vec2(sin(dirAngle), cos(dirAngle));
	
	// Project world space normal to the slice plane
	vec3 toDir = GetCameraVec(texCoords + aoDir);
	vec3 planeNormal = normalize(cross(v, -toDir));
	vec3 projectedNormal = normal - planeNormal * dot(normal, planeNormal);
	
	// Calculate angle n between view vector and projected normal vector
	vec3 projectedDir = normalize(normalize(toDir) + v);
	float n = GTAOFastAcos(dot(-projectedDir, normalize(projectedNormal))) - PIHalf;
	
	// Init variables
	float c1 = -1.0;
	float c2 = -1.0;
	
	vec2 texCoordsBase = texCoords + aoDir * (0.25 * ((int(gl_FragCoord.y) - int(gl_FragCoord.x)) & 3) - 0.375);
	
	const float minMip = 0.0;
	const float maxMip = 3.0;
	const float mipScale = 1.0 / 12.0;
	
	float targetMip = floor(clamp(pow(stride, 1.3) * mipScale, minMip, maxMip));
	
	// Find horizons of the slice
	for(int i = -1; i >= -samples; i--)
	{
		SliceSample(texCoordsBase, aoDir, i, targetMip, ray, v, c1);
	}
	for(int i = 1; i <= samples; i++)
	{
		SliceSample(texCoordsBase, aoDir, i, targetMip, ray, v, c2);
	}
	
	// Finalize
	float h1a = -GTAOFastAcos(c1);
	float h2a = GTAOFastAcos(c2);
	
	// Clamp horizons to the normal hemisphere
	float h1 = n + max(h1a - n, -PIHalf);
	float h2 = n + min(h2a - n, PIHalf);
	
	float visibility = mix(1.0, IntegrateArc(h1, h2, n), length(projectedNormal));
	
	fragColor = vec4(1.0 - vec3(clamp(pow(visibility, 12.0), 0.0, 1.0)), 1.0);
}
