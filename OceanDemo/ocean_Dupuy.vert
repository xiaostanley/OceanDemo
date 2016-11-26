// 2016-11-18 17:11:36

uniform mat4 screenToCamera;	// screen space to camera space
uniform mat4 cameraToWorld;		// camera space to world space
uniform mat4 worldToScreen;		// world space to screen space
uniform vec3 worldCamera;		// camera position in world space

varying vec2 u; 	// horizontal coordinates in world space used to compute P(u)
varying vec3 P; 	// wave point P(u) in world space

vec2 oceanPos(vec4 vertex) 
{
    vec3 cameraDir = normalize((screenToCamera * vertex).xyz);
    vec3 worldDir = (cameraToWorld * vec4(cameraDir, 0.0)).xyz;
    float t = -worldCamera.z / worldDir.z;
    return worldCamera.xy + t * worldDir.xy;
}

void main() 
{
	u = oceanPos(gl_Vertex);

	P = vec3(u, 15.0);

	gl_Position = worldToScreen * vec4(P, 1.0);
}
